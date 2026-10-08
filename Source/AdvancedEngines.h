// ==============================================================================
// mettavoxad - Source/AdvancedEngines.h
// Motores avancados: Afinacao Automatica (Pitch), Vocoder/Hematron e Masterizacao.
// Implementacao 100% local (sem dependencias externas), C++17 / JUCE 8.
// ==============================================================================
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <vector>

namespace mettavoxad_eng
{
    using IirF  = juce::dsp::IIR::Filter<float>;
    using IirC  = juce::dsp::IIR::Coefficients<float>;

    inline float clamp01 (float v) noexcept { return juce::jlimit (0.0f, 1.0f, v); }

    // ==========================================================================
    // 1. DETECTOR DE PITCH (autocorrelacao normalizada, downsampling 2x)
    // ==========================================================================
    class PitchDetector
    {
    public:
        void prepare(double sampleRate) {
            decimation=juce::jmax(1,static_cast<int>(sampleRate/24000.0));
            rate=sampleRate/decimation;
            hist.fill(0); wpos=filled=dsCnt=counter=0; dsAcc=0; lastF0=0;
        }
        void push(float sample) noexcept {
            dsAcc+=sample;
            if(++dsCnt<decimation) return;
            hist[wpos]=dsAcc/decimation; dsCnt=0; dsAcc=0;
            wpos=(wpos+1)%1024; filled=juce::jmin(1024,filled+1);
            if(++counter>=128) {counter=0; if(filled==1024) detect();}
        }
        float lastF0=0;
    private:
        void detect() noexcept {
            const int minLag=juce::jlimit(2,499,static_cast<int>(rate/1000.0));
            const int maxLag=juce::jlimit(minLag+1,500,static_cast<int>(rate/50.0));
            std::array<float,1024> ordered;
            double energy=0;
            for(int n=0;n<1024;++n) {ordered[n]=hist[(wpos+n)%1024]; energy+=ordered[n]*ordered[n];}
            if(energy<1.0e-5) {lastF0=0;return;}
            // Cumulative mean normalised difference (YIN). Chronological ring read
            // prevents the discontinuity caused by correlating unordered storage.
            std::array<double,501> difference{};
            double running=0;
            int selected=0;
            for(int lag=1;lag<=maxLag;++lag) {
                double d=0;
                for(int n=0;n<512;++n) {const double delta=ordered[n]-ordered[n+lag]; d+=delta*delta;}
                running+=d;
                difference[lag]=running>1.0e-12 ? d*lag/running : 1.0;
            }
            for(int lag=minLag;lag<maxLag;++lag) {
                if(difference[lag]<0.15) {
                    while(lag<maxLag && difference[lag+1]<difference[lag]) ++lag;
                    selected=lag;break;
                }
            }
            if(!selected) {lastF0=0;return;}
            double refined=selected;
            if(selected>1 && selected<maxLag) {
                const double a=difference[selected-1],b=difference[selected],c=difference[selected+1];
                const double denominator=a-2*b+c;
                if(std::abs(denominator)>1.0e-12) refined+=juce::jlimit(-0.5,0.5,0.5*(a-c)/denominator);
            }
            lastF0=static_cast<float>(rate/refined);
        }
        std::array<float,1024> hist{};
        double rate=24000;
        int wpos=0,filled=0,dsCnt=0,decimation=2,counter=0;
        float dsAcc=0;
    };

    // ==========================================================================
    // 2. PITCH SHIFTER granular com duas cabeças em crossfade (Hann)
    // ==========================================================================
    class GrainShifter
    {
    public:
        void init (int lineSize)
        {
            size = lineSize;
            line.assign ((size_t) lineSize, 0.0f);
            wpos = 0; phase = 0.0f;
        }

        float process (float input, float ratio) noexcept
        {
            line[(size_t) wpos] = input;
            const float grain = (float) size * 0.5f;
            phase += (1.0f - ratio) / grain;
            if (phase >= 1.0f) phase -= 1.0f;
            if (phase < 0.0f)  phase += 1.0f;

            float p2 = phase + 0.5f;
            if (p2 >= 1.0f) p2 -= 1.0f;

            float w1 = std::sin (juce::MathConstants<float>::pi * phase); w1 *= w1;
            float w2 = std::sin (juce::MathConstants<float>::pi * p2);    w2 *= w2;

            const float s1 = readAt (grain * phase) * w1;
            const float s2 = readAt (grain * p2) * w2;

            wpos = (wpos + 1) % size;
            return s1 + s2;
        }

    private:
        float readAt (float delay) noexcept
        {
            float rp = (float) wpos - delay;
            while (rp < 0.0f) rp += (float) size;
            const int i0 = ((int) rp) % size;
            const int i1 = (i0 + 1) % size;
            const float frac = rp - std::floor (rp);
            return line[(size_t) i0] * (1.0f - frac) + line[(size_t) i1] * frac;
        }

        std::vector<float> line;
        int size = 8192, wpos = 0;
        float phase = 0.0f;
    };

    // ==========================================================================
    // 3. MOTOR DE AFINACAO AUTOMATICA (estilo Autotune)
    // ==========================================================================
    class PitchCorrectionEngine
    {
    public:
        void prepare (double sampleRate)
        {
            det.prepare (sampleRate);
            rate=sampleRate;
            const int length=juce::jmax(2048,static_cast<int>(sampleRate*0.085));
            shL.init (length); shR.init (length);
            smoothRatio = 1.0f; ratioTarget = 1.0f;
            counter = 0; smoothCoef = 0.02f;
            updCounter = 0;
            lastDetectedHz = 0.0f;
        }

        void setParams (bool on, float amountPct, float speedMs, int keyIdx, int scaleIdx,
                        float transposeSemitones=0,float humanisePct=0,float mixPct=100,float outputDb=0)
        {
            active = on;
            transpose=juce::jlimit(-12.0f,12.0f,transposeSemitones);humanise=clamp01(humanisePct/100);
            wet=clamp01(mixPct/100);outputGain=juce::Decibels::decibelsToGain(outputDb);
            amount = clamp01 (amountPct / 100.0f);
            key = juce::jlimit (0, 11, keyIdx);
            scale = juce::jlimit (0, 2, scaleIdx);
            smoothCoef = static_cast<float>(1.0-std::exp(-1.0/(rate*juce::jmax(5.0f,speedMs)*0.001)));
        }

        void processStereo (float& l, float& r) noexcept
        {
            if (! active) { ratioTarget = 1.0f; smoothRatio = 1.0f; lastDetectedHz=0; return; }

            det.push (0.5f * (l + r));
            lastDetectedHz = det.lastF0;

            if (++updCounter >= 16)
            {
                updCounter = 0;
                updateRatio();
            }

            smoothRatio += (ratioTarget - smoothRatio) * smoothCoef;
            if(wet<=0 || (amount<=0.005f && std::abs(transpose)<0.01f)) {l*=outputGain;r*=outputGain;return;}
            const float dryL=l,dryR=r;
            l=(dryL*(1-wet)+shL.process(dryL,smoothRatio)*wet)*outputGain;
            r=(dryR*(1-wet)+shR.process(dryR,smoothRatio)*wet)*outputGain;
        }

        float lastDetectedHz = 0.0f;

    private:
        static int snapToScale(float note,int keyIdx,int scaleIdx) noexcept {
            static const int major[7]={0,2,4,5,7,9,11},minor[7]={0,2,3,5,7,8,10};
            int best=static_cast<int>(std::round(note)); float distance=100;
            for(int candidate=static_cast<int>(std::floor(note))-12;candidate<=static_cast<int>(std::ceil(note))+12;++candidate) {
                bool allowed=scaleIdx==0;
                const int relative=((candidate-keyIdx)%12+12)%12;
                if(scaleIdx) for(int n=0;n<7;++n) if(relative==(scaleIdx==1?major[n]:minor[n])) allowed=true;
                const float d=std::abs(candidate-note);
                if(allowed && d<distance) {distance=d;best=candidate;}
            }
            return best;
        }

        void updateRatio() noexcept
        {
            const float f0 = det.lastF0;
            if (f0 <= 0.0f) { ratioTarget = std::pow(2.0f,transpose/12.0f); return; }
            const float midi = 69.0f + 12.0f * std::log2 (f0 / 440.0f);
            const int target = snapToScale (midi, key, scale);
            float cents = ((float) target - midi) * 100.0f;
            cents = juce::jlimit (-600.0f, 600.0f, cents) * amount*(1-humanise*0.5f*(1-juce::jmin(1.0f,std::abs(cents)/100.0f)))+transpose*100.0f;
            ratioTarget = std::pow (2.0f, cents / 1200.0f);
        }

        float transpose=0,humanise=0,wet=1,outputGain=1;
        double rate=48000;
        PitchDetector det;
        GrainShifter shL, shR;
        bool active = false;
        float amount = 0.5f, smoothRatio = 1.0f, ratioTarget = 1.0f, smoothCoef = 0.02f;
        int key = 0, scale = 1, counter = 0, updCounter = 0;
    };

    // ==========================================================================
    // 4. MOTOR VOCODER / HEMATRON (banco de 24 bandas + portadora interna)
    // ==========================================================================
    // 24-band spectral voice synthesis, band-limited multi-oscillator carrier,
    // envelope/formant remapping, consonant preservation and stereo depth.
    class VocoderEngine
    {
    public:
        static constexpr int numBands=24;
        void prepare(double sampleRate) {
            sr=sampleRate;
            for(int b=0;b<numBands;++b) {
                const double hz=90.0*std::pow(11000.0/90.0,b/23.0);
                const auto coefficients=juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass(sr,juce::jmin(sr*0.43,hz),4.0f);
                for(int c=0;c<2;++c) {
                    *analysis[b][c].coefficients=coefficients;*synthesis[b][c].coefficients=coefficients;
                    analysis[b][c].reset();synthesis[b][c].reset();env[b][c]=0;
                }
            }
            const auto low=juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(sr,juce::jmin(sr*0.43,9000.0));
            const auto high=juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(sr,juce::jmin(sr*0.43,3500.0));
            for(int c=0;c<2;++c) {
                *toneFilter[c].coefficients=low;*consonants[c].coefficients=high;
                toneFilter[c].reset();consonants[c].reset();
                lower[c].init(juce::jmax(2048,static_cast<int>(sr*0.085)));
                phase[c].fill(0);
            }
            inputEnv=outputEnv=0;gain=1;lastTone=-1;
            attack=static_cast<float>(1.0-std::exp(-1.0/(sr*0.003)));
            release=static_cast<float>(1.0-std::exp(-1.0/(sr*0.040)));
            levelSmoothing=static_cast<float>(1.0-std::exp(-1.0/(sr*0.020)));
            reverb.setSampleRate(sr);reverb.reset();
            setParams(1,85,70,4);
        }
        void setParams(int modeIdx,float mixPct,float tonePct,int carrierNoteIdx,
                       float texturePct=70,float formantSemitones=0,float widthPct=40,
                       float depthPct=10,float outputDb=0) {
            mode=juce::jlimit(0,3,modeIdx);mix=clamp01(mixPct/100);
            texture=clamp01(texturePct/100);width=clamp01(widthPct/100);
            formantBands=juce::jlimit(-8.0f,8.0f,formantSemitones)*(23.0f/(12.0f*std::log2(11000.0f/90.0f)));
            outputGain=juce::Decibels::decibelsToGain(juce::jlimit(-12.0f,6.0f,outputDb));
            const float tone=clamp01(tonePct/100);
            if(tone!=lastTone) {
                const auto coefficients=juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(sr,juce::jmin(sr*0.43,1200.0+tone*10800.0));
                for(auto& filter:toneFilter) *filter.coefficients=coefficients;
                lastTone=tone;
            }
            const float base=130.8128f*std::pow(2.0f,juce::jlimit(0,11,carrierNoteIdx)/12.0f)*(mode==2?0.75f:1.0f);
            for(int c=0;c<2;++c) for(int v=0;v<3;++v) {
                const float spread=(c==0?-1.0f:1.0f)*width*(v+1)*4.0f;
                const float interval=mode==3?(v==0?1.0f:v==1?1.498307f:2.0f):(v==2?2.0f:1.0f);
                increment[c][v]=base*interval*std::pow(2.0f,spread/1200.0f)/static_cast<float>(sr);
            }
            juce::Reverb::Parameters p;
            p.roomSize=0.45f;p.damping=0.6f;p.width=width;p.wetLevel=clamp01(depthPct/100)*0.35f;p.dryLevel=0.5f;p.freezeMode=0;
            reverb.setParameters(p);
        }
        void processStereo(float& l,float& r) noexcept {
            if(mode==0 || mix<=0) return;
            const float dry[]{l,r};float carrier[2]{};
            for(int c=0;c<2;++c) for(int v=0;v<3;++v) {
                const float t=phase[c][v],dt=increment[c][v];
                const float saw=2*t-1-polyBlep(t,dt);
                const float pulse=(t<0.5f?1.0f:-1.0f)+polyBlep(t,dt)-polyBlep(t<0.5f?t+0.5f:t-0.5f,dt);
                const float sine=std::sin(juce::MathConstants<float>::twoPi*t);
                const float weight=v==0?0.55f:v==1?0.30f:0.15f;
                carrier[c]+=(saw*(0.45f+0.35f*texture)+pulse*texture*0.20f+sine*(1-texture)*0.35f)*weight;
                phase[c][v]+=dt;if(phase[c][v]>=1) phase[c][v]-=1;
            }
            for(int b=0;b<numBands;++b) for(int c=0;c<2;++c) {
                const float amplitude=std::abs(analysis[b][c].processSample(dry[c]));
                env[b][c]+=(amplitude-env[b][c])*(amplitude>env[b][c]?attack:release);
            }
            float wet[2]{};
            for(int b=0;b<numBands;++b) for(int c=0;c<2;++c) {
                const float position=b-formantBands;
                float envelope=0;
                if(position>=0 && position<=numBands-1) {
                    const int k=static_cast<int>(position);const float fraction=position-k;
                    envelope=env[k][c]*(1-fraction)+env[juce::jmin(k+1,numBands-1)][c]*fraction;
                }
                wet[c]+=synthesis[b][c].processSample(carrier[c])*envelope*24.0f;
            }
            for(int c=0;c<2;++c) {
                if(mode==2) wet[c]=wet[c]*0.75f+lower[c].process(dry[c],0.75f)*0.28f;
                wet[c]=toneFilter[c].processSample(wet[c]);
                wet[c]+=consonants[c].processSample(dry[c])*(0.12f+0.08f*(1-texture));
            }
            const float in=0.5f*(std::abs(l)+std::abs(r)),out=0.5f*(std::abs(wet[0])+std::abs(wet[1]));
            inputEnv+=(in-inputEnv)*levelSmoothing;outputEnv+=(out-outputEnv)*levelSmoothing;
            const float desired=inputEnv>0.00001f?juce::jlimit(0.02f,4.0f,inputEnv/(outputEnv+0.00001f)):1.0f;
            gain+=(desired-gain)*levelSmoothing;
            for(auto& x:wet) x=std::tanh(x*gain*1.2f)/1.2f;
            reverb.processStereo(&wet[0],&wet[1],1);
            l=(dry[0]*(1-mix)+wet[0]*mix)*outputGain;
            r=(dry[1]*(1-mix)+wet[1]*mix)*outputGain;
        }
    private:
        static float polyBlep(float t,float dt) noexcept {
            if(t<dt) {t/=dt;return t+t-t*t-1;}
            if(t>1-dt) {t=(t-1)/dt;return t*t+t+t+1;}
            return 0;
        }
        double sr=48000;
        std::array<std::array<IirF,2>,numBands> analysis{},synthesis{};
        std::array<std::array<float,2>,numBands> env{};
        std::array<IirF,2> toneFilter{},consonants{};
        std::array<GrainShifter,2> lower;
        std::array<std::array<float,3>,2> phase{},increment{};
        juce::Reverb reverb;
        float attack=0.01f,release=0.001f,levelSmoothing=0.001f;
        float mix=0.85f,texture=0.7f,width=0.4f,formantBands=0,outputGain=1,lastTone=-1;
        float inputEnv=0,outputEnv=0,gain=1;
        int mode=1;
    };

    // ==========================================================================
    // 5. MOTOR DE MASTERIZACAO (EQ + Multibanda + Exciter + Width + Limiter)
    // ==========================================================================
    class MasteringEngine
    {
    public:
        void prepare (double sampleRate)
        {
            sr = sampleRate;
            applyCoefficients (true);
            resetAll();
            limEnv = 0.0f; limSmooth=1.0f;
        }

        void setParams (bool on, float lowDb, float midDb, float highDb,
                        float mbPct, float excPct, float widthPct,
                        float loudPct, float ceilDb)
        {
            active = on;
            const bool changed = lowDb != lastLow || midDb != lastMid || highDb != lastHigh;
            lastLow = lowDb; lastMid = midDb; lastHigh = highDb;
            if (changed || !coeffDone) applyCoefficients (false);

            mb   = clamp01 (mbPct / 100.0f);
            exc  = clamp01 (excPct / 100.0f);
            sideGain = juce::jlimit (0.0f, 2.0f, widthPct / 100.0f);
            loudGain = 1.0f + clamp01 (loudPct / 100.0f) * 1.4f;
            ceilGain = juce::Decibels::decibelsToGain (juce::jlimit (-12.0f, 0.0f, ceilDb));
        }

        void processStereo (float& l, float& r) noexcept
        {
            if (! active) return;

            // EQ tonal
            l = eqHigh[0].processSample (eqMid[0].processSample (eqLow[0].processSample (l)));
            r = eqHigh[1].processSample (eqMid[1].processSample (eqLow[1].processSample (r)));

            // Largura estereo (Mid/Side)
            const float m = 0.5f * (l + r);
            float s = 0.5f * (l - r) * sideGain;
            l = m + s;
            r = m - s;

            // Exciter harmonico nas altas
            if (exc > 0.001f)
            {
                float eL = excBp[0].processSample (l);
                float eR = excBp[1].processSample (r);
                l += std::tanh (eL * 3.0f) * 0.22f * exc;
                r += std::tanh (eR * 3.0f) * 0.22f * exc;
            }

            // Compressao multibanda (3 bandas, LR4 200 Hz / 3.5 kHz)
            if (mb > 0.001f)
            {
                for (int c = 0; c < 2; ++c)
                {
                    float& x = (c == 0) ? l : r;
                    const float lo = lp2[c].processSample (lp1[c].processSample (x));
                    const float hi = hp2[c].processSample (hp1[c].processSample (x));
                    const float mi = x - lo - hi;

                    const float lo2 = compressBand (envLo[c], lo);
                    const float mi2 = compressBand (envMi[c], mi);
                    const float hi2 = compressBand (envHi[c], hi);

                    x = lo2 + mi2 + hi2;
                }
            }

            // Estagio de loudness (saturacao suave)
            l = std::tanh (l * loudGain) / std::tanh (loudGain);
            r = std::tanh (r * loudGain) / std::tanh (loudGain);

            // Limitador brickwall no teto escolhido
            const float pk = std::max (std::abs (l), std::abs (r));
            limEnv = std::max (pk, limEnv * 0.9997f);
            const float g = (limEnv > ceilGain) ? ceilGain / limEnv : 1.0f;
            limSmooth += (g - limSmooth) * 0.75f;
            l=juce::jlimit(-ceilGain,ceilGain,l*limSmooth);
            r=juce::jlimit(-ceilGain,ceilGain,r*limSmooth);
        }

    private:
        float compressBand (float& env, float x) noexcept
        {
            const float ax = std::abs (x);
            if (ax > env) env += (ax - env) * 0.02f;
            else          env *= 0.9995f;

            const float envDb = juce::Decibels::gainToDecibels (env, -96.0f);
            const float thresh = -14.0f - mb * 8.0f;
            const float ratio  = 1.4f + mb * 2.2f;
            float grDb = 0.0f;
            if (envDb > thresh)
                grDb = (envDb - thresh) * (1.0f - 1.0f / ratio);
            const float gain = juce::Decibels::decibelsToGain (-grDb * 0.55f); // make-up parcial
            return x * gain;
        }

        void resetAll()
        {
            for (int c = 0; c < 2; ++c)
            {
                eqLow[c].reset(); eqMid[c].reset(); eqHigh[c].reset();
                lp1[c].reset(); lp2[c].reset(); hp1[c].reset(); hp2[c].reset();
                excBp[c].reset();
            }
            envLo.fill(0);envMi.fill(0);envHi.fill(0);
        }

        void applyCoefficients (bool /*force*/)
        {
            coeffDone = true;
            const double d = sr;
            for (int c = 0; c < 2; ++c)
            {
                *eqLow[c].coefficients  = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf  (d, 180.0, 0.707, juce::Decibels::decibelsToGain (lastLow));
                *eqMid[c].coefficients  = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (d, 1200.0, 0.9,  juce::Decibels::decibelsToGain (lastMid));
                *eqHigh[c].coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf (d, juce::jmin(d*0.44,6000.0), 0.707, juce::Decibels::decibelsToGain (lastHigh));
                *lp1[c].coefficients = *IirC::makeLowPass  (d, 200.0);
                *lp2[c].coefficients = *IirC::makeLowPass  (d, 200.0);
                *hp1[c].coefficients = *IirC::makeHighPass (d, juce::jmin(d*0.44,3500.0));
                *hp2[c].coefficients = *IirC::makeHighPass (d, juce::jmin(d*0.44,3500.0));
                *excBp[c].coefficients = *IirC::makeBandPass (d, juce::jmin(d*0.44,4200.0), 0.8);
            }
        }

        double sr = 48000.0;
        std::array<IirF, 2> eqLow {}, eqMid {}, eqHigh {}, lp1 {}, lp2 {}, hp1 {}, hp2 {}, excBp {};
        std::array<float,2> envLo{},envMi{},envHi{};
        float limEnv = 0.0f, limSmooth = 1.0f;
        float mb = 0.0f, exc = 0.0f, sideGain = 1.0f, loudGain = 1.0f, ceilGain = 1.0f;
        float lastLow = 0.0f, lastMid = 0.0f, lastHigh = 0.0f;
        bool active = false, coeffDone = false;
    };
    class HarmonyEngine {
    public:
        void prepare(double sr) {left.init(juce::jmax(2048,static_cast<int>(sr*0.085)));right.init(juce::jmax(2048,static_cast<int>(sr*0.085)));}
        void processStereo(float& l,float& r,float amount) noexcept {
            if(amount<=0.0f) return;
            const float mix=clamp01(amount/100.0f)*0.4f;
            const float wetL=left.process(l,1.498307f),wetR=right.process(r,1.498307f);
            l=l*(1-mix)+wetL*mix;r=r*(1-mix)+wetR*mix;
        }
    private:
        GrainShifter left,right;
    };

}
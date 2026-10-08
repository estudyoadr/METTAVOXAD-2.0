// ==============================================================================
// mettavoxad - Source/DspModules.h
// Implementacao modular em tempo real da cadeia vocal de 5 estagios:
// 1. Entrada e Limpeza (Ganho, HPF, Expander/Gate Suave, Reducao de Ruido, Plosivas)
// 2. Corpo e Tonalidade (Peso 135Hz, EQ Dinamico 280Hz, Calor c/ Oversampling, Clareza, Presenca)
// 3. Dinamica (Compressor Optico/Marcante, Paralelo, De-Esser Split-Band, Limitador Brickwall)
// 4. Deep Reverb (Curta/Media/Profunda, Pre-Delay, Difusao, Filtros e Vocal Ducking)
// 5. Efeitos Musicais (Delay Sync, Dobra Vocal, Micro-Pitch, Harmonizador Diatonico e Pitch)
// ==============================================================================
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <array>

namespace mettavoxad_dsp
{
    inline float sanitizeSample (float x) noexcept
    {
        if (! std::isfinite (x) || std::abs (x) < 1.0e-30f)
            return 0.0f;
        return juce::jlimit (-4.0f, 4.0f, x);
    }

    struct BiquadStereo
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        double z1[2] = { 0.0, 0.0 }, z2[2] = { 0.0, 0.0 };

        void reset() noexcept
        {
            z1[0] = z1[1] = z2[0] = z2[1] = 0.0;
        }

        void setHighPass (double sr, double freqHz, double Q = 0.7071) noexcept
        {
            const double w0 = 2.0 * juce::MathConstants<double>::pi * juce::jlimit (20.0, sr * 0.45, freqHz) / sr;
            const double cosw = std::cos (w0), sinw = std::sin (w0);
            const double alpha = sinw / (2.0 * Q);
            const double a0 = 1.0 + alpha;
            b0 = ((1.0 + cosw) * 0.5) / a0;
            b1 = -(1.0 + cosw) / a0;
            b2 = ((1.0 + cosw) * 0.5) / a0;
            a1 = (-2.0 * cosw) / a0;
            a2 = (1.0 - alpha) / a0;
        }

        void setPeaking (double sr, double freqHz, double Q, double gainDb) noexcept
        {
            const double A = std::pow (10.0, gainDb / 40.0);
            const double w0 = 2.0 * juce::MathConstants<double>::pi * juce::jlimit (20.0, sr * 0.45, freqHz) / sr;
            const double cosw = std::cos (w0), sinw = std::sin (w0);
            const double alpha = sinw / (2.0 * Q);
            const double a0 = 1.0 + alpha / A;
            b0 = (1.0 + alpha * A) / a0;
            b1 = (-2.0 * cosw) / a0;
            b2 = (1.0 - alpha * A) / a0;
            a1 = (-2.0 * cosw) / a0;
            a2 = (1.0 - alpha / A) / a0;
        }

        void setHighShelf (double sr, double freqHz, double gainDb) noexcept
        {
            const double A = std::pow (10.0, gainDb / 40.0);
            const double w0 = 2.0 * juce::MathConstants<double>::pi * juce::jlimit (20.0, sr * 0.45, freqHz) / sr;
            const double cosw = std::cos (w0), sinw = std::sin (w0);
            const double alpha = sinw * 0.5 * std::sqrt (2.0);
            const double sqA = 2.0 * std::sqrt (A) * alpha;
            const double a0 = (A + 1.0) - (A - 1.0) * cosw + sqA;
            b0 = (A * ((A + 1.0) + (A - 1.0) * cosw + sqA)) / a0;
            b1 = (-2.0 * A * ((A - 1.0) + (A + 1.0) * cosw)) / a0;
            b2 = (A * ((A + 1.0) + (A - 1.0) * cosw - sqA)) / a0;
            a1 = (2.0 * ((A - 1.0) - (A + 1.0) * cosw)) / a0;
            a2 = ((A + 1.0) - (A - 1.0) * cosw - sqA) / a0;
        }

        inline float processSample (int ch, float x) noexcept
        {
            const double y = b0 * x + z1[ch];
            z1[ch] = b1 * x - a1 * y + z2[ch];
            z2[ch] = b2 * x - a2 * y;
            return sanitizeSample (static_cast<float> (y));
        }
    };

    // Modulo 4: Deep Reverb com Ducking Vocal Dinamico
    class DeepReverbEngine
    {
    public:
        void prepare (double sr)
        {
            sampleRate = sr;
            juce::dsp::ProcessSpec spec { sr, 512, 2 };
            reverb.prepare (spec);
            lowCut.reset();
            highCut.reset();
            preDelayBuffer.assign (static_cast<size_t> (sr * 0.5), 0.0f);
            writePos = 0;
            duckEnv = 0.0f;
        }

        void processStereo (float& left, float& right, float vocalDetectorAbs,
                            float preDelayMs, float decaySec, float sizePct,
                            float dampingPct, float lowCutHz, float highCutHz,
                            float duckingPct, float wetMixPct) noexcept
        {
            if (wetMixPct <= 0.1f)
                return;

            lowCut.setHighPass (sampleRate, lowCutHz, 0.707);
            highCut.setHighShelf (sampleRate, highCutHz, -9.0);

            juce::dsp::Reverb::Parameters rp;
            rp.roomSize = juce::jlimit (0.1f, 0.98f, (sizePct * 0.006f) + (decaySec / 9.0f));
            rp.damping  = juce::jlimit (0.05f, 0.95f, 1.0f - (dampingPct * 0.0085f));
            rp.wetLevel = 1.0f;
            rp.dryLevel = 0.0f;
            rp.width    = 0.92f;
            reverb.setParameters (rp);

            const int delaySamples = juce::jlimit (1, static_cast<int> (preDelayBuffer.size()) - 2,
                                                   static_cast<int> ((preDelayMs * 0.001f) * sampleRate));
            const float monoIn = 0.5f * (left + right);
            preDelayBuffer[static_cast<size_t> (writePos)] = monoIn;
            const int readPos = (writePos - delaySamples + static_cast<int> (preDelayBuffer.size()))
                                % static_cast<int> (preDelayBuffer.size());
            writePos = (writePos + 1) % static_cast<int> (preDelayBuffer.size());

            float wetL = lowCut.processSample (0, preDelayBuffer[static_cast<size_t> (readPos)]);
            float wetR = wetL;
            float* chans[2] = { &wetL, &wetR };
            juce::AudioBuffer<float> tmp (chans, 2, 1);
            juce::dsp::AudioBlock<float> block (tmp);
            juce::dsp::ProcessContextReplacing<float> ctx (block);
            reverb.process (ctx);

            wetL = highCut.processSample (0, wetL);
            wetR = highCut.processSample (1, wetR);

            // Ducking: reduz a reverberacao durante as palavras e libera nas pausas
            duckEnv = duckEnv * 0.995f + vocalDetectorAbs * 0.005f;
            const float duckGain = juce::jlimit (0.18f, 1.0f, 1.0f - (duckEnv * 2.8f * (duckingPct * 0.01f)));
            const float mix = (wetMixPct * 0.01f) * duckGain;

            left  = sanitizeSample (left  + wetL * mix);
            right = sanitizeSample (right + wetR * mix);
        }

    private:
        double sampleRate = 48000.0;
        juce::dsp::Reverb reverb;
        BiquadStereo lowCut, highCut;
        std::vector<float> preDelayBuffer;
        int writePos = 0;
        float duckEnv = 0.0f;
    };
}

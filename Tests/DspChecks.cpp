#include "PluginEditor.h"
#include <iostream>
#include <vector>
#include <functional>
#include <stdexcept>
void require(bool ok,const char* description) {
    if(!ok) throw std::runtime_error(description);
    std::cout<<"PASS: "<<description<<"\n";
}
void set(MettavoxadAudioProcessor& p,const char* id,float value) {
    auto* parameter=p.apvts.getParameter(id);
    require(parameter!=nullptr,id);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
float peakHz(const std::vector<float>& samples,double sr) {
    constexpr int order=15,n=1<<order;
    std::vector<float> data(n*2,0);
    for(int k=0;k<n;++k) data[k]=samples[samples.size()-n+k]*(0.5f-0.5f*std::cos(juce::MathConstants<float>::twoPi*k/(n-1)));
    juce::dsp::FFT fft(order);fft.performFrequencyOnlyForwardTransform(data.data());
    int peak=1;
    for(int k=2;k<n/2;++k) if(data[k]>data[peak]) peak=k;
    return static_cast<float>(peak*sr/n);
}
std::vector<float> run(const std::function<void(float&,float&)>& processing,int frames=96000,double sr=48000,float freq=225) {
    std::vector<float> result(frames);
    for(int n=0;n<frames;++n) {
        float l=0.18f*std::sin(juce::MathConstants<float>::twoPi*freq*n/sr),r=l;
        processing(l,r);
        if(!std::isfinite(l)||!std::isfinite(r)) throw std::runtime_error("Non-finite audio");
        result[n]=l;
    }
    return result;
}
double energy(const std::vector<float>& x) {double sum=0;for(size_t n=x.size()/2;n<x.size();++n) sum+=x[n]*x[n];return std::sqrt(sum/(x.size()-x.size()/2));}
std::vector<float> render(MettavoxadAudioProcessor& p) {
    std::vector<float> result;
    juce::MidiBuffer midi;
    for(int block=0;block<160;++block) {
        juce::AudioBuffer<float> audio(2,512);
        for(int n=0;n<512;++n) for(int c=0;c<2;++c) audio.setSample(c,n,0.18f*std::sin(juce::MathConstants<float>::twoPi*225*(block*512+n)/48000.0));
        p.processBlock(audio,midi);
        for(int n=0;n<512;++n) {auto x=audio.getSample(0,n);if(!std::isfinite(x)) throw std::runtime_error("Processor non-finite audio");result.push_back(x);}
    }
    return result;
}
double differenceRms(const std::vector<float>& a,const std::vector<float>& b) {
    double sum=0;for(size_t n=0;n<a.size();++n) sum+=std::pow(a[n]-b[n],2);
    return std::sqrt(sum/a.size());
}
std::vector<float> speechLike(const std::function<void(float&,float&)>& process,int frames=48000,double rate=48000) {
    std::vector<float> audio(frames);double phase=0;
    for(int n=0;n<frames;++n) {
        const double t=n/rate,f0=205+22*std::sin(t*5)+7*std::sin(t*17);
        phase+=juce::MathConstants<double>::twoPi*f0/rate;
        float sample=0;
        for(int k=1;k<=32;++k) {
            const double hz=k*f0;
            const double formant=0.2+1.7*std::exp(-std::pow((hz-650)/220,2))+1.4*std::exp(-std::pow((hz-1800)/450,2))+0.8*std::exp(-std::pow((hz-2900)/550,2));
            sample+=static_cast<float>(0.12*formant/k*std::sin(phase*k));
        }
        sample*=static_cast<float>(0.5+0.5*std::sin(juce::MathConstants<double>::pi*std::fmod(t*3,1)));
        float r=sample;process(sample,r);
        if(!std::isfinite(sample)||!std::isfinite(r)||std::abs(sample)>1.01f||std::abs(r)>1.01f) throw std::runtime_error("Speech-like audio became invalid");
        audio[n]=sample;
    }
    return audio;
}
void checkNewControls() {
    auto tune=[](const std::array<float,8>& v) {
        mettavoxad_eng::PitchCorrectionEngine engine;engine.prepare(48000);
        engine.setParams(true,v[0],v[1],static_cast<int>(v[6]),static_cast<int>(v[7]),v[2],v[3],v[4],v[5]);
        return speechLike([&](float& l,float& r){engine.processStereo(l,r);});
    };
    const std::array<float,8> normalTune{100,5,0,0,100,0,0,1};
    const std::array<float,8> otherTune{0,200,-4,100,0,-6,5,2};
    auto reference=tune(normalTune);
    for(int k=0;k<8;++k) {
        auto values=normalTune;values[k]=otherTune[k];auto changed=tune(values);
        std::cout<<"Autotune control "<<k<<" difference "<<differenceRms(reference,changed)<<"\n";
        require(differenceRms(reference,changed)>0.0001,"Autotune knob changes voiced audio");
    }
    auto robot=[](const std::array<float,8>& v,int mode=1) {
        mettavoxad_eng::VocoderEngine engine;engine.prepare(48000);
        engine.setParams(mode,v[0],v[1],static_cast<int>(v[4]),v[2],v[3],v[5],v[6],v[7]);
        return speechLike([&](float& l,float& r){engine.processStereo(l,r);});
    };
    const std::array<float,8> normalRobot{100,75,75,0,4,45,10,0};
    const std::array<float,8> otherRobot{0,0,0,-6,0,100,100,-6};
    reference=robot(normalRobot);
    for(int k=0;k<8;++k) {
        auto values=normalRobot;values[k]=otherRobot[k];auto changed=robot(values);
        std::cout<<"Robot control "<<k<<" difference "<<differenceRms(reference,changed)<<"\n";
        require(differenceRms(reference,changed)>0.0001,"Robot knob changes voiced audio");
    }
    const auto hem=robot(normalRobot,2),prism=robot(normalRobot,3);
    require(differenceRms(reference,hem)>0.01 && differenceRms(reference,prism)>0.01 && differenceRms(hem,prism)>0.01,"Three robotic architectures sound different");
    mettavoxad_eng::VocoderEngine silence;silence.prepare(48000);silence.setParams(3,100,100,11);
    for(int n=0;n<48000;++n) {float l=0,r=0;silence.processStereo(l,r);if(l!=0||r!=0) throw std::runtime_error("Robot generates audio without input");}
    require(true,"Robot remains silent without voice input");
}
void checkModulePresets() {
    const char* switches[]{"vocalOn","atOn","masterOn","vocOn"};
    const char* params[4][9]{
     {"macroPeso","macroCalor","macroClareza","macroPresenca","macroCompressao","macroProfundidade","macroHarmonia","macroSaida",nullptr},
     {"atAmount","atSpeed","atTranspose","atHumanise","atMix","atOutput","atKey","atScale",nullptr},
     {"mstLowDb","mstMidDb","mstHighDb","mstMbAmount","mstExciter","mstWidth","mstLoudness","mstCeilingDb",nullptr},
     {"vocMix","vocTone","vocTexture","vocFormant","vocNote","vocWidth","vocDepth","vocOutput","vocMode"}};
    MettavoxadAudioProcessor processor;
    for(int m=0;m<4;++m) {
        require(processor.getModulePresetNames(m).size()==6,"Each module has six specific presets");
        for(int preset=0;preset<6;++preset) {
            std::vector<std::pair<juce::String,float>> others;
            for(int other=0;other<4;++other) if(other!=m) {
                others.emplace_back(switches[other],processor.apvts.getRawParameterValue(switches[other])->load());
                for(auto id:params[other]) if(id) others.emplace_back(id,processor.apvts.getRawParameterValue(id)->load());
            }
            processor.applyModulePreset(m,preset);
            for(const auto& value:others) if(processor.apvts.getRawParameterValue(value.first)->load()!=value.second) throw std::runtime_error("Module preset changed another tab");
            processor.soloModule(m);processor.prepareToPlay(48000,512);
            auto output=render(processor);require(energy(output)>0.002,"Specific preset produces audible finite output in SOLO");
            require(processor.getModulePresetIndex(m)==preset,"Module preset selection retained");
        }
    }
    for(int mask=0;mask<16;++mask) {
        for(int m=0;m<4;++m) processor.apvts.getParameter(switches[m])->setValueNotifyingHost((mask&(1<<m))?1:0);
        processor.prepareToPlay(48000,512);auto output=render(processor);
        require(energy(output)>0.002,"All sixteen module combinations produce finite audio");
    }
    for(int m=0;m<4;++m) {
        processor.soloModule(m);
        for(int k=0;k<4;++k) if(processor.apvts.getRawParameterValue(switches[k])->load()!=(k==m?1:0)) throw std::runtime_error("SOLO failed to isolate module");
    }
    juce::MemoryBlock state;processor.getStateInformation(state);
    processor.applyModulePreset(3,0);processor.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    require(processor.getModulePresetIndex(3)==5,"Module presets persist in saved state");
}
int main(int argc,char** argv) {
    try {
        juce::ScopedJuceInitialiser_GUI gui;
        checkNewControls();checkModulePresets();
        for(double rate:{44100.0,48000.0,96000.0}) {
            mettavoxad_eng::PitchDetector detector;detector.prepare(rate);
            run([&](float& l,float&){detector.push(l);},static_cast<int>(rate),rate);
            require(std::abs(detector.lastF0-225)<1.5f,"Pitch detection 225 Hz at 44.1/48/96 kHz");
        }
        mettavoxad_eng::PitchCorrectionEngine pitch;pitch.prepare(48000);pitch.setParams(true,100,5,0,0);
        auto corrected=run([&](float& l,float& r){pitch.processStereo(l,r);});
        const float frequency=peakHz(corrected,48000);
        std::cout<<"Corrected peak: "<<frequency<<" Hz, target 220 Hz\n";
        require(std::abs(frequency-220)<2,"Autotune corrects detuned A to 220 Hz");
        pitch.prepare(48000);pitch.setParams(true,100,5,0,2);
        auto minor=run([&](float& a,float& b){pitch.processStereo(a,b);},96000,48000,251);
        require(std::abs(peakHz(minor,48000)-261.626f)<2,"Minor scale selects C across the octave boundary");
        pitch.setParams(false,100,5,0,0);
        float l=0.12f,r=0.13f;pitch.processStereo(l,r);
        require(l==0.12f && r==0.13f,"Autotune OFF is transparent");
        for(int mode:{1,2,3}) {
            mettavoxad_eng::VocoderEngine voc;voc.prepare(48000);voc.setParams(mode,100,70,0);
            auto wet=run([&](float& a,float& b){voc.processStereo(a,b);});
            std::cout<<"Robot mode "<<mode<<" RMS "<<energy(wet)<<", peak "<<peakHz(wet,48000)<<" Hz\n";
            require(std::abs(peakHz(wet,48000)-225)>5,"Robotic synthesis changes the source spectral pitch");
            require(energy(wet)>0.005 && energy(wet)<0.22,"Robot output has controlled gain");
            auto reference=run([](float&,float&){});
            double difference=0;for(size_t n=48000;n<wet.size();++n) difference+=std::pow(wet[n]-reference[n],2);
            require(std::sqrt(difference/48000)>0.02,"Robot effect changes the source audio");
            require(energy(wet)>0.005,"Vocoder/Hematron produces audible non-silent audio");
            voc.setParams(mode,0,70,0);l=0.12f;r=0.13f;voc.processStereo(l,r);
            require(l==0.12f && r==0.13f,"Robot zero mix is transparent");
        }
        mettavoxad_eng::HarmonyEngine harmony;harmony.prepare(48000);
        auto harmonic=run([&](float& a,float& b){harmony.processStereo(a,b,100);});
        auto unharmonised=run([](float&,float&){});
        double harmonyDifference=0;for(size_t n=48000;n<harmonic.size();++n) harmonyDifference+=std::pow(harmonic[n]-unharmonised[n],2);
        require(energy(harmonic)>0.03 && std::sqrt(harmonyDifference/48000)>0.02,"Harmony changes the audio");
        mettavoxad_eng::MasteringEngine master;master.prepare(48000);master.setParams(true,0,0,0,100,25,100,100,-6);
        auto limited=run([&](float& a,float& b){a*=5;b*=5;master.processStereo(a,b);});
        float maximum=0;for(auto sample:limited) maximum=std::max(maximum,std::abs(sample));
        require(maximum<=juce::Decibels::decibelsToGain(-6.0f)+0.00001f,"Master respects the selected ceiling");
        MettavoxadAudioProcessor processor;
        processor.prepareToPlay(48000,512);
        set(processor,"vocalOn",0);set(processor,"atOn",0);set(processor,"masterOn",0);set(processor,"vocOn",0);
        auto dry=render(processor);
        auto reference=run([](float&,float&){},static_cast<int>(dry.size()));
        double error=0;for(size_t n=0;n<dry.size();++n) error=std::max(error,static_cast<double>(std::abs(dry[n]-reference[n])));
        require(error<0.00001,"All tab switches OFF bypass all processing");
        set(processor,"vocMode",1);set(processor,"vocMix",100);set(processor,"vocOn",1);
        auto robot=render(processor);require(energy(robot)>0.005,"Vocoder reaches final processor output");
        set(processor,"vocMode",2);
        auto metal=render(processor);require(energy(metal)>0.005,"Hematron reaches final processor output");
        set(processor,"vocOn",0);set(processor,"atOn",1);set(processor,"atAmount",100);set(processor,"atSpeed",5);set(processor,"atScale",0);set(processor,"atHumanise",0);
        auto tuned=render(processor);require(std::abs(peakHz(tuned,48000)-220)<2,"Autotune reaches final processor output");
        set(processor,"atOn",0);set(processor,"vocalOn",1);set(processor,"qualityOversample",1);
        auto hq=render(processor);require(energy(hq)>0.005 && processor.getLatencySamples()>0,"HQ oversampling processes audio and reports latency");
        set(processor,"globalBypass",1);auto bypass=render(processor);
        error=0;for(size_t n=0;n<bypass.size();++n) error=std::max(error,static_cast<double>(std::abs(bypass[n]-reference[n])));
        require(error<0.00001 && processor.getLatencySamples()==0,"Global bypass is transparent and removes processing latency");
        set(processor,"vocalOn",0);set(processor,"vocOn",0);
        juce::MemoryBlock state;processor.getStateInformation(state);
        set(processor,"vocOn",1);processor.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        require(processor.apvts.getRawParameterValue("vocOn")->load()==0,"Tab switch persists in saved state");
        if(argc==2) {
            set(processor,"globalBypass",0);set(processor,"vocalOn",1);set(processor,"atOn",1);set(processor,"masterOn",1);set(processor,"vocOn",1);
            for(int m=0;m<4;++m) processor.applyModulePreset(m,0);
            std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
            editor->setSize(980,700);
            int combinations=0,knobs=0;
            for(auto* child:editor->getChildren()) {
                if(auto* combo=dynamic_cast<juce::ComboBox*>(child)) {
                    require(combo->getNumItems()>0,"Choice menu populated");
                    if(combo->getName().startsWith("preset_")) require(combo->getNumItems()==6,"Tab preset menu has six specific choices");
                    ++combinations;
                }
                if(auto* slider=dynamic_cast<juce::Slider*>(child)) {
                    require(slider->getSliderStyle()==juce::Slider::RotaryHorizontalVerticalDrag,"Numeric control is a rotary knob");++knobs;
                }
            }
            require(combinations>=5 && knobs==32,"All choice controls and 32 knobs exist");
            auto directory=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);directory.createDirectory();
            const char* names[]{"tab_vocal","tab_autotune","tab_master","tab_robot"};
            for(auto name:names) {
                for(auto* child:editor->getChildren()) if(child->getName()==name)
                    if(auto* button=dynamic_cast<juce::TextButton*>(child)) {button->setToggleState(true,juce::dontSendNotification);button->onClick();}
                for(auto* child:editor->getChildren()) if(child->isVisible())
                    require(editor->getLocalBounds().contains(child->getBounds()),"Visible control stays inside editor");
                auto image=editor->createComponentSnapshot(editor->getLocalBounds());
                auto destination=directory.getChildFile(juce::String(name)+".png");destination.deleteFile();
                juce::FileOutputStream output(destination);
                juce::PNGImageFormat png;require(png.writeImageToStream(image,output),"Tab preview rendered");
            }
            set(processor,"atAmount",100);set(processor,"atSpeed",5);set(processor,"atHumanise",0);set(processor,"atScale",0);
            for(auto* child:editor->getChildren()) if(child->getName()=="atTranspose")
                if(auto* knob=dynamic_cast<juce::Slider*>(child)) knob->setValue(4,juce::sendNotificationSync);
            require(processor.apvts.getRawParameterValue("atTranspose")->load()==4,"Moving Autotune GUI knob updates the audio parameter");
            for(auto* child:editor->getChildren()) if(child->getName()=="solo_1")
                if(auto* button=dynamic_cast<juce::TextButton*>(child)) button->onClick();
            require(processor.apvts.getRawParameterValue("atOn")->load()==1 && processor.apvts.getRawParameterValue("vocOn")->load()==0 && processor.apvts.getRawParameterValue("vocalOn")->load()==0 && processor.apvts.getRawParameterValue("masterOn")->load()==0,"GUI SOLO isolates Autotune");
            processor.prepareToPlay(48000,512);auto moved=render(processor);
            require(std::abs(peakHz(moved,48000)-277.183f)<3,"Moving Autotune GUI knob transposes final audio by four semitones");
            for(auto* child:editor->getChildren()) if(child->getName()=="vocMix")
                if(auto* knob=dynamic_cast<juce::Slider*>(child)) knob->setValue(100,juce::sendNotificationSync);
            require(processor.apvts.getRawParameterValue("vocMix")->load()==100,"Moving Robot GUI knob updates the audio parameter");
            for(auto* child:editor->getChildren()) if(child->getName()=="solo_3")
                if(auto* button=dynamic_cast<juce::TextButton*>(child)) button->onClick();
            processor.prepareToPlay(48000,512);auto movedRobot=render(processor);
            require(differenceRms(movedRobot,reference)>0.02,"Moving Robot GUI control and SOLO changes final audio");
            editor->setSize(820,620);
            for(auto name:names) {
                for(auto* child:editor->getChildren()) if(child->getName()==name)
                    if(auto* button=dynamic_cast<juce::TextButton*>(child)) button->onClick();
                for(auto* child:editor->getChildren()) if(child->isVisible())
                    require(editor->getLocalBounds().contains(child->getBounds()),"Control fits the minimum editor size");
            }
        }
        std::cout<<"ALL CHECKS PASSED\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<"\n";return 1;}
}

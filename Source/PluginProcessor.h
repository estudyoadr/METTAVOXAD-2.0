// ==============================================================================
// mettavoxad - Source/PluginProcessor.h
// Processador VST3 64-bit: cadeia vocal + Afinaçao Automatica + Masterizaçao
// + Vocoder/Hematron. 24 Presets. Mono/Stereo, PDC, Oversampling 4x.
// ==============================================================================
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DspModules.h"
#include "VoiceAssistantAnalyzer.h"
#include "AdvancedEngines.h"

class MettavoxadAudioProcessor : public juce::AudioProcessor
{
public:
    MettavoxadAudioProcessor();
    ~MettavoxadAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "METTAVOXAD 2.1"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.5; }

    int getNumPrograms() override { return 25; }
    int getCurrentProgram() override { return currentProgramIndex; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::StringArray getModulePresetNames(int module) const;
    void applyModulePreset(int module,int index);
    void soloModule(int module);
    int getModulePresetIndex(int module) const {return modulePresets[juce::jlimit(0,3,module)].load();}
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> meterInputRmsDb  { -72.0f };
    std::atomic<float> meterOutputRmsDb { -72.0f };
    std::atomic<float> meterCompGrDb    {  0.0f };
    std::atomic<bool>  meterClipFlag    { false };
    std::atomic<float> livePitchHz      {  0.0f };

    mettavoxad_eng::PitchCorrectionEngine pitchEngine;
    mettavoxad_eng::VocoderEngine         vocoderEngine;
    mettavoxad_eng::MasteringEngine       masteringEngine;
    mettavoxad_eng::HarmonyEngine         harmonyEngine;

private:
    std::array<std::atomic<int>,4> modulePresets{{-1,-1,-1,-1}};
    int currentProgramIndex = 0;
    double currentSampleRate = 48000.0;
    bool lastHqOversamplingState = false;

    mettavoxad_dsp::BiquadStereo hpfFilter, pesoFilter, mudDynFilter, clarezaFilter, presencaFilter, deEsserFilter;
    mettavoxad_dsp::DeepReverbEngine deepReverb;
    mettavoxad_assistant::LocalVoiceAnalyzer voiceAnalyzer;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler4x;

    juce::SmoothedValue<float> smoothInGain, smoothOutGain, smoothDrive, smoothCompThresh;
    float compEnvState = 0.0f;
    float gateEnvState = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MettavoxadAudioProcessor)
};
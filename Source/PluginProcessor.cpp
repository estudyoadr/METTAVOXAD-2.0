// ==============================================================================
// mettavoxad - Source/PluginProcessor.cpp
// ==============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================================================================
// 24 Presets — valores reais aplicados a todos os modulos.
// ==============================================================================
namespace
{
    struct MettavoxadPreset
    {
        // Cadeia vocal (8 macros)
        float peso, calor, clareza, presenca, comp, prof, harm, saida;
        // Afinacao automatica
        float atOn, atAmount, atSpeed, atKey, atScale;
        // Vocoder / Hematron
        float vocMode, vocMix, vocTone, vocNote;
        // Masterizacao
        float masterOn, mstLow, mstMid, mstHigh, mstMb, mstExc, mstWidth, mstLoud, mstCeil;
    };

    static const MettavoxadPreset kPresets[25] =
    {
        //        Peso Calor Clar Pres Comp Prof Harm Said || atOn atAm atSp Key Scale || vM vMix vTone vNote || mOn mLo mMi mHi  MB  Exc Wid Loud Ceil
        { 48.0f, 30.0f, 62.0f, 55.0f, 55.0f, 10.0f,  0.0f,  -2.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Radio Clara
        { 40.0f, 28.0f, 70.0f, 65.0f, 60.0f,  8.0f,  0.0f,  -1.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // TV Presente
        { 50.0f, 38.0f, 55.0f, 45.0f, 45.0f, 14.0f,  0.0f,  -3.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Podcast Natural
        { 58.0f, 48.0f, 48.0f, 35.0f, 35.0f, 22.0f,  0.0f,  -4.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Narracao Intima
        { 68.0f, 44.0f, 58.0f, 50.0f, 55.0f, 16.0f,  0.0f,  -2.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Voz Grave com Definicao
        { 55.0f, 35.0f, 60.0f, 48.0f, 40.0f, 18.0f,  0.0f,  -3.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Voz Leve com Corpo
        { 85.0f, 62.0f, 50.0f, 45.0f, 75.0f, 30.0f, 15.0f,  -1.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Trailer Profundo
        { 70.0f, 50.0f, 60.0f, 58.0f, 65.0f, 26.0f, 10.0f,  -2.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Chamada de Cinema
        { 60.0f, 40.0f, 75.0f, 70.0f, 70.0f, 14.0f, 20.0f,   0.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Vinheta Energetica
        { 65.0f, 42.0f, 65.0f, 60.0f, 60.0f, 18.0f,  8.0f,  -2.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Impacto Controlado
        { 45.0f, 30.0f, 55.0f, 40.0f, 30.0f, 12.0f,  0.0f,  -4.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Vocal Natural
        { 48.0f, 32.0f, 68.0f, 68.0f, 55.0f, 12.0f,  6.0f,  -1.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Vocal Pop Presente
        { 62.0f, 50.0f, 52.0f, 40.0f, 48.0f, 34.0f, 12.0f,  -3.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Balada Profunda
        { 50.0f, 40.0f, 50.0f, 42.0f, 35.0f, 28.0f,  5.0f,  -5.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Dobra Suave
        { 52.0f, 38.0f, 62.0f, 55.0f, 45.0f, 24.0f, 35.0f,  -2.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Harmonia Aberta
        { 48.0f, 35.0f, 58.0f, 48.0f, 30.0f, 48.0f, 45.0f,  -2.0f,   0,0,40,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Atmosfera Eterea
        { 45.0f, 30.0f, 60.0f, 52.0f, 50.0f, 10.0f,  0.0f,  -1.5f,   1,45, 60,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Afinaçao Natural
        { 42.0f, 28.0f, 68.0f, 66.0f, 58.0f,  8.0f,  0.0f,  -1.0f,   1,85, 30,0,1,  0,0,55,4,   0, 0,0,0, 30,25,110,35,-0.3f }, // Afinaçao Radio Pop
        { 50.0f, 38.0f, 58.0f, 55.0f, 55.0f, 20.0f, 10.0f,  -2.0f,   1,70, 55,0,2,  0,0,55,4,   1, 1,1,1, 40,30,115,40,-0.2f }, // Voz Completa Studio
        { 45.0f, 30.0f, 55.0f, 45.0f, 40.0f, 10.0f,  0.0f,  -2.0f,   0,0,40,0,1,  1,45,55,4,  0, 0,0,0, 30,25,110,35,-0.3f }, // Vocoder Robotico
        { 60.0f, 45.0f, 60.0f, 60.0f, 55.0f, 16.0f,  8.0f,  -2.0f,   0,0,40,0,1,  1,65,45,7,  0, 0,0,0, 30,25,110,35,-0.3f }, // Vocoder Harmonico
        { 50.0f, 35.0f, 60.0f, 50.0f, 45.0f, 12.0f,  0.0f,  -2.0f,   0,0,40,0,1,  2,50,60,4,  0, 0,0,0, 30,25,110,35,-0.3f }, // Hematron Metal
        { 55.0f, 40.0f, 58.0f, 52.0f, 55.0f, 14.0f,  6.0f,  -1.0f,   0,0,40,0,1,  0,0,55,4,   1, 2,1,2, 45,30,120,55,-0.2f }, // Master Broadcast
        { 58.0f, 48.0f, 55.0f, 50.0f, 60.0f, 18.0f, 10.0f,  -1.5f,   0,0,40,0,1,  0,0,55,4,   1, 3,0,2, 50,40,125,60,-0.2f }, // Master Cinema
        { 48.0f, 38.0f, 62.0f, 54.0f, 50.0f, 12.0f,  4.0f,  -1.0f,   1,55, 45,0,1,  0,0,55,4,   1, 1,1,1, 35,30,110,40,-0.3f }  // Cadeia Completa
    };

    static const char* kPresetNames[25] =
    {
        "Radio Clara", "TV Presente", "Podcast Natural", "Narracao Intima",
        "Voz Grave com Definicao", "Voz Leve com Corpo",
        "Trailer Profundo", "Chamada de Cinema", "Vinheta Energetica", "Impacto Controlado",
        "Vocal Natural", "Vocal Pop Presente", "Balada Profunda", "Dobra Suave",
        "Harmonia Aberta", "Atmosfera Eterea",
        "Afinaçao Natural", "Afinaçao Radio Pop", "Voz Completa Studio",
        "Vocoder Robotico", "Vocoder Harmonico", "Hematron Metal",
        "Master Broadcast", "Master Cinema", "Cadeia Completa"
    };
}

MettavoxadAudioProcessor::MettavoxadAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "METTAVOXAD_STATE", createParameterLayout())
{
    oversampler4x = std::make_unique<juce::dsp::Oversampling<float>> (
        2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
}

juce::AudioProcessorValueTreeState::ParameterLayout MettavoxadAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // 8 Macros Principais (Modo Essencial)
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("macroPeso",         "Peso",         0.0f, 100.0f, 48.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("macroCalor",        "Calor",        0.0f, 100.0f, 35.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("macroClareza",      "Clareza",      0.0f, 100.0f, 52.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("macroPresenca",     "Presenca",     0.0f, 100.0f, 45.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("macroCompressao",   "Compressao",   0.0f, 100.0f, 50.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("macroProfundidade", "Profundidade", 0.0f, 100.0f, 18.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("macroHarmonia",     "Harmonia",     0.0f, 100.0f, 0.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("macroSaida",        "Saida",       -18.0f,  12.0f, 0.00f));

    // Afinacao Automatica
    p.push_back (std::make_unique<juce::AudioParameterBool>  ("atOn",         "Afinacao ON",     false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("atAmount",     "Afinacao Amount",  0.0f, 100.0f, 60.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("atSpeed",      "Afinacao Speed",   5.0f, 400.0f, 60.00f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("atKey",       "Tonalidade",  juce::StringArray ({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }), 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("atScale",     "Escala",      juce::StringArray ({ "Cromatica", "Maior", "Menor" }), 1));

    // Vocoder / Hematron
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("vocMode",     "Modo Voz",    juce::StringArray ({ "Desligado", "Vocoder Neon", "Hematron Cinema", "Prisma Digital" }), 1));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("vocMix",       "Vocoder Mix",  0.0f, 100.0f, 85.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("vocTone",      "Vocoder Tone", 0.0f, 100.0f, 55.00f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("vocNote",     "Portadora",   juce::StringArray ({ "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3" }), 4));

    // Masterizacao
    p.push_back (std::make_unique<juce::AudioParameterBool>  ("masterOn",     "Master ON",     false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mstLowDb",     "Master Grave",  -8.0f,   8.0f,  0.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mstMidDb",     "Master Medio",  -8.0f,   8.0f,  0.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mstHighDb",    "Master Agudo",  -8.0f,   8.0f,  0.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mstMbAmount",  "Multibanda",     0.0f, 100.0f, 30.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mstExciter",   "Exciter",        0.0f, 100.0f, 25.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mstWidth",     "Largura",        0.0f, 200.0f, 110.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mstLoudness",  "Loudness",       0.0f, 100.0f, 35.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mstCeilingDb", "Teto",         -12.0f,   0.0f, -0.30f));

    // Parametros Tecnicos da cadeia vocal
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("inputGainDb",       "Input Gain",   -24.0f, 24.0f, 0.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("hpfHz",             "High-Pass Hz",  20.0f, 220.0f, 75.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("gateThresholdDb",   "Gate Thresh",  -75.0f, -18.0f, -54.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("deEsserAmountPct",  "De-Esser",       0.0f, 100.0f, 42.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("reverbPreDelayMs",  "Reverb PreDelay",0.0f, 140.0f, 22.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("reverbDecaySec",    "Reverb Decay",   0.35f,  6.5f, 1.10f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("reverbDuckingPct",  "Reverb Ducking", 0.0f, 100.0f, 62.00f));
    p.push_back (std::make_unique<juce::AudioParameterBool>  ("qualityOversample", "Modo HQ 4x",     false));
    p.push_back (std::make_unique<juce::AudioParameterBool>  ("globalBypass",      "Global Bypass",  false));

    // Append new switches to preserve every existing parameter index.
    p.push_back(std::make_unique<juce::AudioParameterBool>("vocalOn","Vocal ON",true));
    p.push_back(std::make_unique<juce::AudioParameterBool>("vocOn","Robo ON",false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("atTranspose","Transposicao",-12.0f,12.0f,0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("atHumanise","Humanizacao",0.0f,100.0f,20.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("atMix","Mistura Afinacao",0.0f,100.0f,100.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("atOutput","Saida Afinacao",-12.0f,6.0f,0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("vocTexture","Textura",0.0f,100.0f,70.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("vocFormant","Formantes",-8.0f,8.0f,0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("vocWidth","Largura Robo",0.0f,100.0f,40.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("vocDepth","Espaco Robo",0.0f,100.0f,10.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("vocOutput","Saida Robo",-12.0f,6.0f,0.0f));
    return { p.begin(), p.end() };
}

bool MettavoxadAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;
    return (mainIn == juce::AudioChannelSet::mono() || mainIn == juce::AudioChannelSet::stereo());
}

void MettavoxadAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    hpfFilter.reset();
    pesoFilter.reset();
    mudDynFilter.reset();
    clarezaFilter.reset();
    presencaFilter.reset();
    deEsserFilter.reset();
    deepReverb.prepare (sampleRate);
    voiceAnalyzer.reset (sampleRate);

    pitchEngine.prepare (sampleRate);
    vocoderEngine.prepare (sampleRate);
    masteringEngine.prepare (sampleRate);
    harmonyEngine.prepare(sampleRate);

    oversampler4x->initProcessing (static_cast<size_t> (samplesPerBlock));
    smoothInGain.reset (sampleRate, 0.018);
    smoothOutGain.reset (sampleRate, 0.018);
    smoothDrive.reset (sampleRate, 0.018);
    smoothCompThresh.reset (sampleRate, 0.018);

    const bool hq = apvts.getRawParameterValue ("qualityOversample")->load() > 0.5f;
    setLatencySamples (hq && apvts.getRawParameterValue("vocalOn")->load()>0.5f && apvts.getRawParameterValue("globalBypass")->load()<0.5f ? static_cast<int> (oversampler4x->getLatencyInSamples()) : 0);
    lastHqOversamplingState = hq;
}

void MettavoxadAudioProcessor::releaseResources() {}

void MettavoxadAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples             = buffer.getNumSamples();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    if (numSamples == 0 || totalNumInputChannels == 0)
        return;

    if (totalNumInputChannels == 1 && totalNumOutputChannels > 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);

    const bool bypass = apvts.getRawParameterValue ("globalBypass")->load() > 0.5f;
    if (bypass)
    {
        livePitchHz.store (0.0f);
        if(getLatencySamples()!=0) setLatencySamples(0);
        const float rms=buffer.getRMSLevel(0,0,numSamples);
        meterInputRmsDb.store(juce::Decibels::gainToDecibels(rms,-72.0f));
        meterOutputRmsDb.store(meterInputRmsDb.load());meterCompGrDb.store(0);
        return;
    }

    const bool hq = apvts.getRawParameterValue ("qualityOversample")->load() > 0.5f;
    const bool vocalOn=apvts.getRawParameterValue("vocalOn")->load()>0.5f;
    const bool vocOn=apvts.getRawParameterValue("vocOn")->load()>0.5f;
    const int wantedLatency=hq && vocalOn ? static_cast<int>(oversampler4x->getLatencyInSamples()) : 0;
    if (wantedLatency != getLatencySamples()) setLatencySamples(wantedLatency);
    if (hq != lastHqOversamplingState)
    {
        setLatencySamples (hq && apvts.getRawParameterValue("vocalOn")->load()>0.5f && apvts.getRawParameterValue("globalBypass")->load()<0.5f ? static_cast<int> (oversampler4x->getLatencyInSamples()) : 0);
        lastHqOversamplingState = hq;
    }

    const float harmonia=apvts.getRawParameterValue("macroHarmonia")->load();
    const float peso         = apvts.getRawParameterValue ("macroPeso")->load();
    const float calor        = apvts.getRawParameterValue ("macroCalor")->load();
    const float clareza      = apvts.getRawParameterValue ("macroClareza")->load();
    const float presenca     = apvts.getRawParameterValue ("macroPresenca")->load();
    const float compressao   = apvts.getRawParameterValue ("macroCompressao")->load();
    const float profundidade = apvts.getRawParameterValue ("macroProfundidade")->load();
    const float saidaDb      = apvts.getRawParameterValue ("macroSaida")->load();
    const float inGainDb     = apvts.getRawParameterValue ("inputGainDb")->load();
    const float hpfHz        = apvts.getRawParameterValue ("hpfHz")->load();
    const float gateDb       = apvts.getRawParameterValue ("gateThresholdDb")->load();
    const float deEssPct     = apvts.getRawParameterValue ("deEsserAmountPct")->load();
    const float revPreMs     = apvts.getRawParameterValue ("reverbPreDelayMs")->load();
    const float revDecay     = apvts.getRawParameterValue ("reverbDecaySec")->load();
    const float revDuck      = apvts.getRawParameterValue ("reverbDuckingPct")->load();

    // Modulos avancados
    const bool  atOn      = apvts.getRawParameterValue ("atOn")->load() > 0.5f;
    const float atAmount  = apvts.getRawParameterValue ("atAmount")->load();
    const float atSpeed   = apvts.getRawParameterValue ("atSpeed")->load();
    const int   atKey     = (int) apvts.getRawParameterValue ("atKey")->load();
    const int   atScale   = (int) apvts.getRawParameterValue ("atScale")->load();
    const int   vocMode   = (int) apvts.getRawParameterValue ("vocMode")->load();
    const float vocMix    = apvts.getRawParameterValue ("vocMix")->load();
    const float vocTone   = apvts.getRawParameterValue ("vocTone")->load();
    const int   vocNote   = (int) apvts.getRawParameterValue ("vocNote")->load();
    const bool  masterOn  = apvts.getRawParameterValue ("masterOn")->load() > 0.5f;
    const float mstLow    = apvts.getRawParameterValue ("mstLowDb")->load();
    const float mstMid    = apvts.getRawParameterValue ("mstMidDb")->load();
    const float mstHigh   = apvts.getRawParameterValue ("mstHighDb")->load();
    const float mstMb     = apvts.getRawParameterValue ("mstMbAmount")->load();
    const float mstExc    = apvts.getRawParameterValue ("mstExciter")->load();
    const float mstWidth  = apvts.getRawParameterValue ("mstWidth")->load();
    const float mstLoud   = apvts.getRawParameterValue ("mstLoudness")->load();
    const float mstCeil   = apvts.getRawParameterValue ("mstCeilingDb")->load();

    pitchEngine.setParams (atOn, atAmount, atSpeed, atKey, atScale,
        apvts.getRawParameterValue("atTranspose")->load(),apvts.getRawParameterValue("atHumanise")->load(),
        apvts.getRawParameterValue("atMix")->load(),apvts.getRawParameterValue("atOutput")->load());
    vocoderEngine.setParams (vocOn ? vocMode : 0, vocMix, vocTone, vocNote,
        apvts.getRawParameterValue("vocTexture")->load(),apvts.getRawParameterValue("vocFormant")->load(),
        apvts.getRawParameterValue("vocWidth")->load(),apvts.getRawParameterValue("vocDepth")->load(),
        apvts.getRawParameterValue("vocOutput")->load());
    masteringEngine.setParams (masterOn, mstLow, mstMid, mstHigh, mstMb, mstExc, mstWidth, mstLoud, mstCeil);

    hpfFilter.setHighPass (currentSampleRate, hpfHz);
    pesoFilter.setPeaking (currentSampleRate, 135.0, 0.95, ((peso - 45.0f) / 55.0f) * 5.5f);
    mudDynFilter.setPeaking (currentSampleRate, 280.0, 1.6, -(clareza * 0.055f));
    clarezaFilter.setPeaking (currentSampleRate, 2750.0, 0.9, ((clareza - 40.0f) / 60.0f) * 4.8f);
    presencaFilter.setHighShelf (currentSampleRate, 7500.0, ((presenca - 35.0f) / 65.0f) * 5.2f);
    deEsserFilter.setPeaking (currentSampleRate, 6800.0, 3.2, -(deEssPct * 0.095f));

    smoothInGain.setTargetValue (juce::Decibels::decibelsToGain (inGainDb));
    smoothOutGain.setTargetValue (juce::Decibels::decibelsToGain (saidaDb + (compressao * 0.032f)));
    smoothDrive.setTargetValue (1.0f + (calor * 0.018f));
    smoothCompThresh.setTargetValue (-10.0f - (compressao * 0.24f));

    float* chL = buffer.getWritePointer (0);
    float* chR = totalNumOutputChannels > 1 ? buffer.getWritePointer (1) : chL;

    double sumInSq = 0.0, sumOutSq = 0.0;
    float maxGrDb = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float smoothedInput=smoothInGain.getNextValue();
        const float gIn = vocalOn ? smoothedInput : 1.0f;
        float sL = mettavoxad_dsp::sanitizeSample (chL[i] * gIn);
        float sR = mettavoxad_dsp::sanitizeSample (chR[i] * gIn);
        sumInSq += 0.5 * (sL * sL + sR * sR);

        // 0. Afinacao Automatica (antes da cadeia vocal)
        pitchEngine.processStereo (sL, sR);

        const float pkIn = std::max (std::abs (sL), std::abs (sR));
        voiceAnalyzer.pushSample (0.5f * (sL + sR));

        if(vocalOn) {
        // 1. Entrada e Limpeza (HPF + Soft Expander/Gate)
        const float inSampleDb = juce::Decibels::gainToDecibels (pkIn, -96.0f);
        const float gateTarget = (inSampleDb >= gateDb) ? 1.0f : 0.15f;
        gateEnvState = gateEnvState * 0.994f + gateTarget * 0.006f;
        sL = hpfFilter.processSample (0, sL * gateEnvState);
        sR = hpfFilter.processSample (1, sR * gateEnvState);

        // 2. Corpo, Tonalidade e Calor (Saturacao Harmonica Suave)
        sL = mudDynFilter.processSample (0, pesoFilter.processSample (0, sL));
        sR = mudDynFilter.processSample (1, pesoFilter.processSample (1, sR));

        const float drv = smoothDrive.getNextValue();
        const float normDrv = std::tanh (drv);
        if(hq) {
            float* channels[]{&sL,&sR};
            juce::dsp::AudioBlock<float> audio(channels,2,1);
            auto up=oversampler4x->processSamplesUp(audio);
            for(size_t channel=0;channel<up.getNumChannels();++channel)
                for(size_t sample=0;sample<up.getNumSamples();++sample)
                    up.getChannelPointer(channel)[sample]=std::tanh(up.getChannelPointer(channel)[sample]*drv)/normDrv;
            oversampler4x->processSamplesDown(audio);
        } else {
            sL = std::tanh (sL * drv) / normDrv;
            sR = std::tanh (sR * drv) / normDrv;
        }

        sL = presencaFilter.processSample (0, clarezaFilter.processSample (0, sL));
        sR = presencaFilter.processSample (1, clarezaFilter.processSample (1, sR));

        // 3. Dinamica & De-Esser
        sL = deEsserFilter.processSample (0, sL);
        sR = deEsserFilter.processSample (1, sR);

        const float pkPost = std::max (std::abs (sL), std::abs (sR));
        compEnvState = compEnvState * 0.992f + pkPost * 0.008f;
        const float envDb = juce::Decibels::gainToDecibels (compEnvState, -96.0f);
        const float thresh = smoothCompThresh.getNextValue();
        const float ratio  = 1.6f + (compressao * 0.032f);
        float grDb = 0.0f;
        if (envDb > thresh)
            grDb = (envDb - thresh) * (1.0f - 1.0f / ratio);
        if (grDb > maxGrDb) maxGrDb = grDb;
        const float cGain = juce::Decibels::decibelsToGain (-grDb);
        sL *= cGain;
        sR *= cGain;

        // 4. Deep Reverb com Ducking Vocal
        deepReverb.processStereo (sL, sR, pkPost, revPreMs, revDecay, 65.0f, 55.0f, 175.0f, 8200.0f, revDuck, profundidade * 0.55f);

        harmonyEngine.processStereo(sL,sR,harmonia);
        } // Vocal module bypass leaves the other modules available.
        // Vocal output belongs to its own module, before robot/master.
        const float outputGain=smoothOutGain.getNextValue();
        const float gOut=vocalOn ? outputGain : 1.0f;
        sL*=gOut;sR*=gOut;
        // 5. Vocoder / Hematron e Masterizacao
        vocoderEngine.processStereo (sL, sR);
        masteringEngine.processStereo (sL, sR);

        // 6. Protecao final (-0.1 dBFS)
        if(vocalOn || atOn || masterOn || (vocOn && vocMode>0 && vocMix>0)) {
            sL=juce::jlimit(-0.988f,0.988f,sL);
            sR=juce::jlimit(-0.988f,0.988f,sR);
        }

        if (std::abs (sL) >= 0.985f || std::abs (sR) >= 0.985f)
            meterClipFlag.store (true);

        chL[i] = sL;
        if (totalNumOutputChannels > 1)
            chR[i] = sR;

        sumOutSq += 0.5 * (sL * sL + sR * sR);
    }

    meterInputRmsDb.store  (juce::Decibels::gainToDecibels ( static_cast<float> (std::sqrt (sumInSq / numSamples)), -72.0f));
    meterOutputRmsDb.store (juce::Decibels::gainToDecibels ( static_cast<float> (std::sqrt (sumOutSq / numSamples)), -72.0f));
    meterCompGrDb.store    (maxGrDb);
    livePitchHz.store      (pitchEngine.lastDetectedHz);
}

const juce::String MettavoxadAudioProcessor::getProgramName (int index)
{
    return (index >= 0 && index < 24) ? juce::String::fromUTF8 (kPresetNames[index]) : "Default";
}

void MettavoxadAudioProcessor::setCurrentProgram (int index)
{
    currentProgramIndex = juce::jlimit (0, 24, index);
    const MettavoxadPreset& pv = kPresets[currentProgramIndex];

    auto applyPresetValue = [this] (const char* paramId, float value)
    {
        if (auto* param = apvts.getParameter (paramId))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };

    for(auto& preset:modulePresets) preset.store(-1);
    applyPresetValue("vocalOn",1);applyPresetValue("vocOn",pv.vocMode>0?1:0);
    applyPresetValue ("macroPeso",         pv.peso);
    applyPresetValue ("macroCalor",        pv.calor);
    applyPresetValue ("macroClareza",      pv.clareza);
    applyPresetValue ("macroPresenca",     pv.presenca);
    applyPresetValue ("macroCompressao",   pv.comp);
    applyPresetValue ("macroProfundidade", pv.prof);
    applyPresetValue ("macroHarmonia",     pv.harm);
    applyPresetValue ("macroSaida",        pv.saida);

    applyPresetValue ("atOn",         pv.atOn);
    applyPresetValue ("atAmount",     pv.atAmount>0 ? pv.atAmount : 60);
    applyPresetValue ("atSpeed",      pv.atSpeed);
    applyPresetValue ("atKey",        pv.atKey);
    applyPresetValue ("atScale",      pv.atScale);

    applyPresetValue ("vocMode",      pv.vocMode);
    applyPresetValue ("vocMix",       pv.vocMix>0 ? pv.vocMix : 85);
    applyPresetValue ("vocTone",      pv.vocTone);
    applyPresetValue ("vocNote",      pv.vocNote);

    applyPresetValue ("masterOn",     pv.masterOn);
    applyPresetValue ("mstLowDb",     pv.mstLow);
    applyPresetValue ("mstMidDb",     pv.mstMid);
    applyPresetValue ("mstHighDb",    pv.mstHigh);
    applyPresetValue ("mstMbAmount",  pv.mstMb);
    applyPresetValue ("mstExciter",   pv.mstExc);
    applyPresetValue ("mstWidth",     pv.mstWidth);
    applyPresetValue ("mstLoudness",  pv.mstLoud);
    applyPresetValue ("mstCeilingDb", pv.mstCeil);
    applyPresetValue("atTranspose",0);applyPresetValue("atHumanise",20);
    applyPresetValue("atMix",100);applyPresetValue("atOutput",0);
    applyPresetValue("vocTexture",70);applyPresetValue("vocFormant",0);
    applyPresetValue("vocWidth",40);applyPresetValue("vocDepth",10);applyPresetValue("vocOutput",0);
}

void MettavoxadAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("currentProgram",currentProgramIndex,nullptr);
    for(int module=0;module<4;++module) state.setProperty("modulePreset"+juce::String(module),modulePresets[module].load(),nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MettavoxadAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType())) {
        auto state=juce::ValueTree::fromXml(*xmlState);
        currentProgramIndex=juce::jlimit(0,24,static_cast<int>(state.getProperty("currentProgram",0)));
        for(int module=0;module<4;++module) modulePresets[module].store(static_cast<int>(state.getProperty("modulePreset"+juce::String(module),-1)));
        apvts.replaceState(state);
    }
}

juce::AudioProcessorEditor* MettavoxadAudioProcessor::createEditor()
{
    return new MettavoxadAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MettavoxadAudioProcessor();
}
namespace {
struct ModulePreset {const char* name;std::array<float,8> values;int mode=1;};
const ModulePreset vocalPresets[]{
 {"Radio Clara",{48,30,62,55,55,10,0,-2}}, {"TV Presente",{40,28,70,65,60,8,0,-1}},
 {"Podcast Natural",{45,24,54,40,35,5,0,-2}}, {"Cinema Intimo",{58,40,50,35,40,16,0,-3}},
 {"Trailer Profundo",{70,48,60,60,68,20,8,-4}}, {"Voz Quente",{60,52,52,42,45,12,0,-3}}};
// amount, speed, transpose, humanise, mix, output, key, scale
const ModulePreset tunePresets[]{
 {"Locucao Natural",{55,90,0,80,100,0,0,0}}, {"Radio Polido",{85,35,0,30,100,0,0,0}},
 {"Pop Afinado",{100,15,0,15,100,0,0,1}}, {"Hard Tune Digital",{100,5,0,0,100,0,0,2}},
 {"Voz Grave -4",{65,40,-4,20,100,0,0,0}}, {"Voz Aerea +4",{70,40,4,20,100,-1,0,0}}};
const ModulePreset masterPresets[]{
 {"Radio Equilibrado",{1,0,1.5f,40,20,100,25,-1}}, {"Podcast Limpo",{0,0.5f,1,25,10,100,10,-1}},
 {"Cinema Amplo",{2,-0.5f,1,30,15,130,20,-1}}, {"Impacto Controlado",{2,1,2,65,35,110,60,-1}},
 {"Voz Suave",{1,-1,0,20,5,100,5,-2}}, {"Musica Brilhante",{1,0,3,45,35,140,40,-1}}};
// mix, tone, texture, formant, carrier note, width, depth, output
const ModulePreset robotPresets[]{
 {"Neon Claro",{85,75,75,1,4,45,10,0},1}, {"Hematron Cinema",{90,65,65,-3,0,35,8,0},2},
 {"Android Elegante",{80,80,40,1,7,25,5,0},2}, {"Prisma Pop",{90,85,90,2,9,65,15,-1},3},
 {"Trailer Futurista",{100,65,90,-5,0,45,12,-1},2}, {"Digital Suave",{70,60,40,0,2,25,5,0},1}};
const char* ids[4][8]{
 {"macroPeso","macroCalor","macroClareza","macroPresenca","macroCompressao","macroProfundidade","macroHarmonia","macroSaida"},
 {"atAmount","atSpeed","atTranspose","atHumanise","atMix","atOutput","atKey","atScale"},
 {"mstLowDb","mstMidDb","mstHighDb","mstMbAmount","mstExciter","mstWidth","mstLoudness","mstCeilingDb"},
 {"vocMix","vocTone","vocTexture","vocFormant","vocNote","vocWidth","vocDepth","vocOutput"}};
const char* onIds[]{"vocalOn","atOn","masterOn","vocOn"};
const ModulePreset* bank(int module) {switch(module){case 0:return vocalPresets;case 1:return tunePresets;case 2:return masterPresets;default:return robotPresets;}}
}
juce::StringArray MettavoxadAudioProcessor::getModulePresetNames(int module) const {
    juce::StringArray names;for(int i=0;i<6;++i) names.add(bank(juce::jlimit(0,3,module))[i].name);return names;
}
void MettavoxadAudioProcessor::applyModulePreset(int module,int index) {
    if(module<0 || module>3 || index<0 || index>5) return;
    const auto& preset=bank(module)[index];
    for(int i=0;i<8;++i) if(auto* p=apvts.getParameter(ids[module][i])) {
        p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(preset.values[i]));p->endChangeGesture();
    }
    if(module==3) if(auto* p=apvts.getParameter("vocMode")) p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(preset.mode)));
    if(auto* p=apvts.getParameter(onIds[module])) p->setValueNotifyingHost(1);
    modulePresets[module].store(index);
}
void MettavoxadAudioProcessor::soloModule(int module) {
    for(int m=0;m<4;++m) if(auto* p=apvts.getParameter(onIds[m])) p->setValueNotifyingHost(m==module?1.0f:0.0f);
    if(auto* p=apvts.getParameter("globalBypass")) p->setValueNotifyingHost(0);
}

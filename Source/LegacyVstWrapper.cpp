#include "LegacyVstAbi.h"
#include "PluginProcessor.h"
#include <cstring>
#if defined(_WIN32)
#include <windows.h>
#endif

thread_local bool fromHost = false;

namespace {
using namespace legacy;
void copyText(void* destination, const juce::String& text, size_t capacity) {
    if (destination && capacity) text.copyToUTF8(static_cast<char*>(destination), capacity);
}
struct Instance final : juce::AudioProcessorListener {
    juce::ScopedJuceInitialiser_GUI gui;
    MettavoxadAudioProcessor processor;
    Effect effect {};
    Callback host;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    Rect rect {0, 0, 660, 1100};
    juce::MemoryBlock chunk;
    juce::AudioBuffer<float> buffer;
    juce::MidiBuffer midi;
    double sampleRate = 44100;
    int blockSize = 1024;
    bool active = false;
    explicit Instance(Callback callback) : host(callback) { processor.addListener(this); }
    ~Instance() override {
        editor.reset();
        if (active) processor.releaseResources();
        processor.removeListener(this);
    }
    void activate() {
        if (active) processor.releaseResources();
        processor.setRateAndBufferSizeDetails(sampleRate, blockSize);
        processor.prepareToPlay(sampleRate, blockSize);
        buffer.setSize(2, blockSize);
        effect.initialDelay = processor.getLatencySamples();
        active = true;
    }
    void audioProcessorParameterChanged(juce::AudioProcessor*, int index, float value) override {
        if (!fromHost) host(&effect, 0, index, 0, nullptr, value); // automate
    }
    void audioProcessorParameterChangeGestureBegin(juce::AudioProcessor*, int index) override {
        host(&effect, 43, index, 0, nullptr, 0);
    }
    void audioProcessorParameterChangeGestureEnd(juce::AudioProcessor*, int index) override {
        host(&effect, 44, index, 0, nullptr, 0);
    }
    void audioProcessorChanged(juce::AudioProcessor*, const ChangeDetails& details) override {
        if (details.latencyChanged) {
            effect.initialDelay = processor.getLatencySamples();
            host(&effect, 13, 0, 0, nullptr, 0);
        }
    }
};
Instance& instance(Effect* effect) { return *static_cast<Instance*>(effect->object); }
juce::AudioProcessorParameter* parameter(Instance& i, int index) {
    const auto& parameters = i.processor.getParameters();
    return juce::isPositiveAndBelow(index, parameters.size()) ? parameters[index] : nullptr;
}
void setParameter(Effect* e, int32_t index, float value) {
    if (auto* p = parameter(instance(e), index)) {
        const juce::ScopedValueSetter<bool> guard(fromHost, true);
        p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, value));
    }
}
float getParameter(Effect* e, int32_t index) {
    if (auto* p = parameter(instance(e), index)) return p->getValue();
    return 0;
}
void processCore(Effect* e, float** inputs, float** outputs, int32_t frames, bool accumulate) {
    if (frames <= 0 || !outputs) return;
    auto& i = instance(e);
    if (!i.active) { if (!accumulate) for(int c=0;c<2;++c) if(outputs[c]) std::fill_n(outputs[c],frames,0.0f); return; }
    // Split oversized host blocks; no allocations on the audio thread.
    for (int offset=0; offset<frames; offset+=i.blockSize) {
        const int n=juce::jmin(i.blockSize, frames-offset);
        for (int c=0;c<2;++c) {
            auto* src=inputs ? inputs[c] : nullptr;
            if (src) i.buffer.copyFrom(c,0,src+offset,n); else i.buffer.clear(c,0,n);
        }
        float* channels[] {i.buffer.getWritePointer(0), i.buffer.getWritePointer(1)};
        juce::AudioBuffer<float> view(channels,2,n);
        i.midi.clear();
        i.processor.processBlock(view,i.midi);
        for(int c=0;c<2;++c) if(outputs[c]) {
            if (accumulate) for(int j=0;j<n;++j) outputs[c][offset+j]+=channels[c][j];
            else std::copy_n(channels[c],n,outputs[c]+offset);
        }
    }
}
void process(Effect* e, float** in, float** out, int32_t n) { processCore(e,in,out,n,false); }
void processAdding(Effect* e, float** in, float** out, int32_t n) { processCore(e,in,out,n,true); }
intptr_t dispatchCore(Effect* e, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt) {
    auto& i=instance(e);
    switch(opcode) {
        case 0: return 1; // open
        case 1: delete &i; return 1; // close: effect lifetime ends here
        case 2: if(value>=0 && value<i.processor.getNumPrograms()) i.processor.setCurrentProgram(static_cast<int>(value)); return 0;
        case 3: return i.processor.getCurrentProgram();
        case 5: copyText(ptr,i.processor.getProgramName(i.processor.getCurrentProgram()),24); return 1;
        case 6: if(auto* p=parameter(i,index)) copyText(ptr,p->getLabel(),8); return 1;
        case 7: if(auto* p=parameter(i,index)) copyText(ptr,p->getText(p->getValue(),8),8); return 1;
        case 8: if(auto* p=parameter(i,index)) copyText(ptr,p->getName(8),8); return 1;
        case 10: if(opt>0) { i.sampleRate=opt; if(i.active) i.activate(); } return 0;
        case 11: if(value>0 && value<=1048576) { i.blockSize=static_cast<int>(value); if(i.active) i.activate(); } return 0;
        case 12:
            if(value) i.activate(); else if(i.active) {i.processor.releaseResources(); i.active=false;} return 0;
        case 13: // editor rectangle
            if(!i.editor) i.editor.reset(i.processor.createEditor());
            if(!i.editor || !ptr) return 0;
            i.rect={0,0,static_cast<int16_t>(i.editor->getHeight()),static_cast<int16_t>(i.editor->getWidth())};
            *static_cast<Rect**>(ptr)=&i.rect; return 1;
        case 14: // parent is the host's native child-window container
            if(!ptr) return 0;
            if(!i.editor) i.editor.reset(i.processor.createEditor());
            if(!i.editor) return 0;
            i.editor->addToDesktop(0,ptr); i.editor->setVisible(true); return 1;
        case 15: i.editor.reset(); return 1;
        case 19: return 0; // native event loop handles editor idle
        case 23:
            if(!ptr) return 0;
            i.processor.getStateInformation(i.chunk);
            *static_cast<void**>(ptr)=i.chunk.getData(); return static_cast<intptr_t>(i.chunk.getSize());
        case 24:
            if(ptr && value>0 && value<=16*1024*1024) {
                const juce::ScopedValueSetter<bool> guard(fromHost,true);
                i.processor.setStateInformation(ptr,static_cast<int>(value));
            } return 0;
        case 26: if(auto* p=parameter(i,index)) return p->isAutomatable() ? 1 : 0; return 0;
        case 29: if(juce::isPositiveAndBelow(index,i.processor.getNumPrograms())) {copyText(ptr,i.processor.getProgramName(index),24);return 1;} return 0;
        case 35: return 1; // effect category
        case 44:
            if(auto* p=i.processor.apvts.getParameter("globalBypass")) {
                const juce::ScopedValueSetter<bool> guard(fromHost,true);
                p->setValueNotifyingHost(value ? 1.0f : 0.0f);
            } return 1;
        case 45: copyText(ptr,"METTAVOXAD 2.1",32); return 1;
        case 47: copyText(ptr,"mettavoxad Audio Engineering",64); return 1;
        case 48: copyText(ptr,"METTAVOXAD 2.1",64); return 1;
        case 49: return 20100;
        case 51: // no MIDI, no host-specific extensions
            return 0;
        case 52: return static_cast<intptr_t>(i.sampleRate*i.processor.getTailLengthSeconds());
        case 58: return 2400; // legacy interface version 2.4
        default: return 0;
    }
}
intptr_t dispatch(Effect* e, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt) {
    try { return dispatchCore(e,opcode,index,value,ptr,opt); }
    catch (...) { return 0; }
}
}
#if defined(_WIN32)
#define MV_EXPORT extern "C" __declspec(dllexport)
#else
#define MV_EXPORT extern "C" __attribute__((visibility("default")))
#endif
MV_EXPORT legacy::Effect* VSTPluginMain(legacy::Callback host) {
    if (!host || host(nullptr,1,0,0,nullptr,0)==0) return nullptr;
    try {
#if defined(_WIN32)
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCWSTR>(&VSTPluginMain), &module);
        juce::Process::setCurrentModuleInstanceHandle(module);
#endif
        auto i=std::make_unique<Instance>(host);
        auto& e=i->effect;
        e.magic=0x56737450; e.dispatcher=dispatch; e.process=processAdding;
        e.setParameter=setParameter; e.getParameter=getParameter;
        e.numPrograms=i->processor.getNumPrograms(); e.numParams=i->processor.getParameters().size();
        e.numInputs=2; e.numOutputs=2; e.flags=1|16|32; e.ioRatio=1;
        e.object=i.get(); e.uniqueID=0x4d743231; e.version=20100; e.processReplacing=process;
        auto* result=&e; i.release(); return result;
    } catch (...) { return nullptr; }
}

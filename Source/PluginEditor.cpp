#include "PluginEditor.h"
namespace {
const char* names[]{"VOCAL","AUTOTUNE","MASTER","VOZ ROBO"};
const char* toggles[]{"vocalOn","atOn","masterOn","vocOn"};
const char* tabNames[]{"tab_vocal","tab_autotune","tab_master","tab_robot"};
const uint32_t colours[]{0xff00d9ff,0xffb38bff,0xff36edb0,0xffffaf4b};
const char* ids[4][8]{
 {"macroPeso","macroCalor","macroClareza","macroPresenca","macroCompressao","macroProfundidade","macroHarmonia","macroSaida"},
 {"atAmount","atSpeed","atTranspose","atHumanise","atMix","atOutput","atKey","atScale"},
 {"mstLowDb","mstMidDb","mstHighDb","mstMbAmount","mstExciter","mstWidth","mstLoudness","mstCeilingDb"},
 {"vocMix","vocTone","vocTexture","vocFormant","vocNote","vocWidth","vocDepth","vocOutput"}};
const char* titles[4][8]{
 {"PESO","CALOR","CLAREZA","PRESENCA","COMPRESSAO","PROFUNDIDADE","HARMONIA","SAIDA (dB)"},
 {"INTENSIDADE","VELOCIDADE (ms)","TRANSPOSICAO","HUMANIZACAO","MISTURA","SAIDA (dB)","TONALIDADE","ESCALA"},
 {"GRAVE (dB)","MEDIO (dB)","AGUDO (dB)","MULTIBANDA","EXCITER","LARGURA","LOUDNESS","TETO (dB)"},
 {"MISTURA","TIMBRE","TEXTURA","FORMANTES","PORTADORA","LARGURA","ESPACO","SAIDA (dB)"}};
}
MettavoxadAudioProcessorEditor::MettavoxadAudioProcessorEditor(MettavoxadAudioProcessor& p):AudioProcessorEditor(&p),processor(p) {
    setLookAndFeel(&premiumLook);
    for(int m=0;m<4;++m) {
        for(int k=0;k<8;++k) setupKnob(m,k,ids[m][k],titles[m][k]);
        tabs[m].setName(tabNames[m]);tabs[m].setButtonText(names[m]);tabs[m].setClickingTogglesState(true);tabs[m].setRadioGroupId(42);
        tabs[m].setColour(juce::TextButton::buttonColourId,juce::Colour(0xff101c24));
        tabs[m].setColour(juce::TextButton::buttonOnColourId,juce::Colour(colours[m]).withMultipliedBrightness(0.3f));
        tabs[m].setColour(juce::TextButton::textColourOnId,juce::Colour(colours[m]));
        tabs[m].onClick=[this,m]{showTab(m);};addAndMakeVisible(tabs[m]);
        auto& button=moduleButtons[m];button.setClickingTogglesState(true);button.setName(toggles[m]);
        button.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff18232d));
        button.setColour(juce::TextButton::buttonOnColourId,juce::Colour(colours[m]).withMultipliedBrightness(0.35f));
        button.setColour(juce::TextButton::textColourOnId,juce::Colour(colours[m]));addAndMakeVisible(button);
        buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,toggles[m],button));
        button.onClick=[this,m] {
            if(!moduleButtons[m].getToggleState()) return;
            auto restore=[this](const char* id,float value) {if(auto* param=processor.apvts.getParameter(id)) if(param->getValue()<=0) param->setValueNotifyingHost(param->convertTo0to1(value));};
            if(m==1) {restore("atAmount",85);restore("atMix",100);}
            if(m==3) {restore("vocMode",1);restore("vocMix",85);}
            timerCallback();
        };
        soloButtons[m].setButtonText("SOLO");soloButtons[m].setName("solo_"+juce::String(m));
        soloButtons[m].setColour(juce::TextButton::buttonColourId,juce::Colour(0xff192c38));
        soloButtons[m].onClick=[this,m] {
            processor.soloModule(m);
            if(m==3 && processor.apvts.getRawParameterValue("vocMode")->load()<0.5f) processor.applyModulePreset(3,0);
            timerCallback();
        };addAndMakeVisible(soloButtons[m]);
        presets[m].setName("preset_"+juce::String(m));presets[m].addItemList(p.getModulePresetNames(m),1);
        presets[m].setTextWhenNothingSelected("Escolha um preset / Personalizado");
        presets[m].setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff12222d));
        presets[m].setColour(juce::ComboBox::textColourId,juce::Colour(colours[m]));
        presets[m].onChange=[this,m] {const int index=presets[m].getSelectedId()-1;if(index>=0) processor.applyModulePreset(m,index);timerCallback();};
        addAndMakeVisible(presets[m]);
    }
    robotMode.setName("vocMode");
    if(auto* param=dynamic_cast<juce::AudioParameterChoice*>(p.apvts.getParameter("vocMode"))) robotMode.addItemList(param->choices,1);
    robotMode.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff12222d));
    robotMode.setColour(juce::ComboBox::textColourId,juce::Colour(colours[3]));addAndMakeVisible(robotMode);
    modeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts,"vocMode",robotMode);
    addAndMakeVisible(hq);addAndMakeVisible(bypass);addAndMakeVisible(status);
    hq.setName("qualityOversample");bypass.setName("globalBypass");
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"qualityOversample",hq));
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"globalBypass",bypass));
    status.setJustificationType(juce::Justification::centred);
    status.setFont(juce::Font(juce::FontOptions(12.0f,juce::Font::bold)));
    tabs[0].setToggleState(true,juce::dontSendNotification);showTab(0);
    setResizable(true,true);setResizeLimits(820,620,1280,900);setSize(980,700);
    timerCallback();startTimerHz(20);
}
MettavoxadAudioProcessorEditor::~MettavoxadAudioProcessorEditor(){stopTimer();setLookAndFeel(nullptr);}
void MettavoxadAudioProcessorEditor::setupKnob(int m,int slot,const char* id,const char* title) {
    auto& knob=knobs[m*8+slot];auto& label=labels[m*8+slot];
    knob.setName(id);knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::TextBoxBelow,false,120,22);
    knob.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(colours[m]));
    label.setText(title,juce::dontSendNotification);label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(juce::FontOptions(12.0f,juce::Font::bold)));label.setColour(juce::Label::textColourId,juce::Colour(0xffa9c3d1));
    addAndMakeVisible(knob);addAndMakeVisible(label);
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,id,knob));
    if(auto* parameter=processor.apvts.getParameter(id)) {
        const auto& range=parameter->getNormalisableRange();
        if(range.start<0 && range.end>0) knob.getProperties().set("neutral",0.0);
        if(auto* choice=dynamic_cast<juce::AudioParameterChoice*>(parameter)) {
            const auto choices=choice->choices;
            knob.textFromValueFunction=[choices](double value){return choices[juce::jlimit(0,choices.size()-1,static_cast<int>(std::round(value)))];};
            knob.valueFromTextFunction=[choices](const juce::String& text){return static_cast<double>(juce::jmax(0,choices.indexOf(text)));};
        }
    }
    if(juce::String(id)=="atSpeed" || juce::String(id)=="mstCeilingDb") knob.getProperties().set("reverseIntensity",true);
    if(juce::String(id)=="mstWidth") knob.getProperties().set("neutral",100.0);
}
void MettavoxadAudioProcessorEditor::showTab(int index) {
    currentTab=juce::jlimit(0,3,index);
    for(int m=0;m<4;++m) {
        presets[m].setVisible(m==currentTab);
        for(int k=0;k<8;++k){knobs[m*8+k].setVisible(m==currentTab);labels[m*8+k].setVisible(m==currentTab);}
    }
    robotMode.setVisible(currentTab==3);resized();timerCallback();repaint();
}
void MettavoxadAudioProcessorEditor::timerCallback() {
    const bool off=bypass.getToggleState();
    int count=0;
    for(int m=0;m<4;++m) {
        const bool active=moduleButtons[m].getToggleState();if(active) ++count;
        moduleButtons[m].setButtonText(juce::String(names[m])+(active?" ON":" OFF"));
        const bool processing=active && !off && (m!=3 || processor.apvts.getRawParameterValue("vocMode")->load()>0);
        for(int k=0;k<8;++k) {
            auto& knob=knobs[m*8+k];
            if(!knob.getProperties().contains("moduleOn") || static_cast<bool>(knob.getProperties()["moduleOn"])!=processing) {knob.getProperties().set("moduleOn",processing);knob.repaint();}
        }
        presets[m].setSelectedId(processor.getModulePresetIndex(m)+1,juce::dontSendNotification);
    }
    juce::String message;
    if(off) message="BYPASS GLOBAL ATIVO: AUDIO ORIGINAL";
    else if(count==0) message="TODAS AS ABAS DESLIGADAS: AUDIO ORIGINAL";
    else {
        message="MODULOS ATIVOS: ";for(int m=0;m<4;++m) if(moduleButtons[m].getToggleState()) message+=juce::String(names[m])+"  ";
        if(currentTab==1) {
            const float hz=processor.livePitchHz.load();message+=hz>0?"| PITCH "+juce::String(hz,1)+" Hz":"| AGUARDANDO VOZ MONOFONICA";
        }
    }
    status.setText(message,juce::dontSendNotification);status.setColour(juce::Label::textColourId,juce::Colour(off?0xffff8b8b:0xff82d6df));
    repaint(0,0,getWidth(),60);
}
void MettavoxadAudioProcessorEditor::paint(juce::Graphics& g) {
    const float w=static_cast<float>(getWidth()),h=static_cast<float>(getHeight());
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff101c26),w/2,0,juce::Colour(0xff05090d),w/2,h,false));g.fillRect(getLocalBounds());
    g.setColour(juce::Colour(0xffe6f7ff));g.setFont(juce::Font(juce::FontOptions(25.0f,juce::Font::bold)));
    g.drawText("METTAVOX AD 2.1",24,10,370,32,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff7aabba));g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("VOCAL DESIGN | MODULOS INDEPENDENTES | 24 PRESETS",24,42,540,18,juce::Justification::centredLeft);
    const juce::Rectangle<float> card(16,155,w-32,h-195);
    g.setColour(juce::Colour(0xff0b151d));g.fillRoundedRectangle(card,14);
    g.setColour(juce::Colour(colours[currentTab]).withAlpha(0.30f));g.drawRoundedRectangle(card,14,1.3f);
    g.setColour(juce::Colour(colours[currentTab]));g.setFont(juce::Font(juce::FontOptions(11.0f,juce::Font::bold)));
    g.drawText(juce::String("PRESET DE ")+names[currentTab],30,163,450,18,juce::Justification::centredLeft);
    if(currentTab==3) g.drawText("ARQUITETURA ROBOTICA",getWidth()/2+20,163,getWidth()/2-48,18,juce::Justification::centredLeft);
}
void MettavoxadAudioProcessorEditor::resized() {
    const int w=getWidth(),h=getHeight(),gap=12,cell=(w-48-3*gap)/4;
    hq.setBounds(w-300,16,100,28);bypass.setBounds(w-195,16,175,28);
    for(int m=0;m<4;++m) {
        const int x=24+m*(cell+gap);
        moduleButtons[m].setBounds(x,72,cell-57,30);soloButtons[m].setBounds(x+cell-53,72,53,30);
        tabs[m].setBounds(x,113,cell,32);
        presets[m].setBounds(30,184,currentTab==3?w/2-45:w-60,32);
    }
    robotMode.setBounds(w/2+20,184,w/2-50,32);
    const int row=(h-302)/2;
    for(int m=0;m<4;++m) for(int k=0;k<8;++k) {
        const int x=24+(k%4)*(cell+gap),y=230+(k/4)*(row+10);
        labels[m*8+k].setBounds(x,y,cell,20);
        knobs[m*8+k].setBounds(x,y+22,cell,row-24);
    }
    status.setBounds(24,h-32,w-48,24);
}

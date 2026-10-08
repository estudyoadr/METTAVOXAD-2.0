// ==============================================================================
// mettavoxad - Source/PluginEditor.h
// Interface Neon Futurista com 4 abas: VOCAL / AUTOTUNE / MASTER / VOZ ROBO.
// ==============================================================================
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class MettavoxPremiumLook : public juce::LookAndFeel_V4
{
public:
    // Knob neon: brilha com a intensidade do valor
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float position,
                           float start, float end, juce::Slider& slider) override
    {
        const auto radius = juce::jmin (w, h) * 0.36f;
        const auto cx = x + w * 0.5f, cy = y + h * 0.5f;
        const auto angle = start + position * (end - start);
        const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
        float intensity=position;
        if (slider.getProperties().contains("neutral")) {
            const double neutral=static_cast<double>(slider.getProperties()["neutral"]);
            intensity=static_cast<float>(std::abs(slider.getValue()-neutral)/juce::jmax(neutral-slider.getMinimum(),slider.getMaximum()-neutral));
        }
        if(static_cast<bool>(slider.getProperties()["reverseIntensity"])) intensity=1.0f-position;
        const bool on=!slider.getProperties().contains("moduleOn") || static_cast<bool>(slider.getProperties()["moduleOn"]);
        const float glow=(on ? 1.0f : 0.18f)*(0.12f+0.88f*juce::jlimit(0.0f,1.0f,intensity));   // intensidade -> brilho

        // trilho
        juce::Path ring; ring.addCentredArc (cx, cy, radius, radius, 0, start, end, true);
        g.setColour (juce::Colour (0xff1c2a33));
        g.strokePath (ring, juce::PathStrokeType (5.0f));

        // halo neon (3 camadas)
        juce::Path value;
        if(static_cast<bool>(slider.getProperties()["reverseIntensity"]))
            value.addCentredArc(cx,cy,radius,radius,0,angle,end,true);
        else value.addCentredArc (cx, cy, radius, radius, 0, start, angle, true);
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (accent.withAlpha (glow * 0.16f * (4 - i)));
            g.strokePath (value, juce::PathStrokeType (5.0f + 7.0f * i));
        }
        g.setColour (accent.withMultipliedBrightness(0.25f+0.75f*glow).withAlpha (0.25f + 0.75f * glow));
        g.strokePath (value, juce::PathStrokeType (4.5f));

        // corpo do knob
        const float r = radius - 8.0f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2e404c), cx - r, cy - r,
                                                 juce::Colour (0xff070d11), cx + r, cy + r, false));
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colour (0xff3d5666));
        g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);

        // ponteiro luminoso
        g.setColour (accent.withAlpha (0.25f+0.75f*glow));
        g.drawLine (cx + std::sin (angle) * r * 0.40f, cy - std::cos (angle) * r * 0.40f,
                    cx + std::sin (angle) * r * 0.85f, cy - std::cos (angle) * r * 0.85f, 2.6f);
        g.setColour (accent.withAlpha (glow * 0.35f));
        g.fillEllipse (cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
    }


};

class MettavoxadAudioProcessorEditor : public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit MettavoxadAudioProcessorEditor(MettavoxadAudioProcessor&);
    ~MettavoxadAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void showTab(int);
    void setupKnob(int module,int slot,const char* id,const char* label);
    MettavoxadAudioProcessor& processor;
    MettavoxPremiumLook premiumLook;
    std::array<juce::Slider,32> knobs;
    std::array<juce::Label,32> labels;
    std::array<juce::TextButton,4> tabs,moduleButtons,soloButtons;
    std::array<juce::ComboBox,4> presets;
    juce::ComboBox robotMode;
    juce::ToggleButton hq{"HQ 4x"},bypass{"BYPASS GLOBAL"};
    juce::Label status;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    int currentTab=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MettavoxadAudioProcessorEditor)
};

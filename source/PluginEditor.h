#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class AcidBaddEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit AcidBaddEditor (AcidBaddProcessor&);
    ~AcidBaddEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<SliderAttachment> attach;
    };

    struct StepUI
    {
        juce::Slider       pitch;
        juce::ToggleButton gate, accent, slide;
        std::unique_ptr<SliderAttachment> pitchAtt;
        std::unique_ptr<ButtonAttachment> gateAtt, accentAtt, slideAtt;
    };

    void addKnob (Knob& k, const juce::String& paramID, const juce::String& name);

    AcidBaddProcessor& processor;

    // synth controls
    Knob tuning, cutoff, reso, envmod, decay, accent, volume;
    juce::ComboBox waveBox;
    juce::Label    waveLabel;
    std::unique_ptr<ComboAttachment> waveAttach;

    // sequencer transport
    juce::ToggleButton playButton { "RUN" }, syncButton { "SYNC" };
    juce::Slider       tempoSlider, octaveSlider;
    juce::ComboBox     rootBox;
    juce::Label        tempoLabel, rootLabel, octaveLabel;
    std::unique_ptr<ButtonAttachment> playAtt, syncAtt;
    std::unique_ptr<SliderAttachment> tempoAtt, octaveAtt;
    std::unique_ptr<ComboAttachment>  rootAtt;

    // sequencer grid
    std::array<StepUI, AcidBaddProcessor::kNumSteps> steps;
    juce::Label rowLabelPitch, rowLabelGate, rowLabelAccent, rowLabelSlide;
    juce::Rectangle<int> gridArea;     // for drawing the play-head highlight
    int highlightStep = -1;

    juce::MidiKeyboardComponent keyboard;
    juce::LookAndFeel_V4 lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AcidBaddEditor)
};

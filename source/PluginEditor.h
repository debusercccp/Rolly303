#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Draws a software mouse pointer that follows the cursor. Needed on setups
// where the X server provides no visible hardware cursor (e.g. a rootful
// Xwayland with no window manager), but harmless anywhere. Transparent to
// clicks so it never interferes with the controls underneath.
class CursorOverlay : public juce::Component
{
public:
    CursorOverlay()
    {
        setInterceptsMouseClicks (false, false);
        setAlwaysOnTop (true);
        setPaintingIsUnclipped (true);
    }

    void moveTo (juce::Point<int> p)
    {
        if (! visiblePos || *visiblePos != p)
        {
            const auto old = visiblePos;
            visiblePos = p;
            if (old) repaint (cursorBounds (*old));
            repaint (cursorBounds (p));
        }
    }

    void hide()
    {
        if (visiblePos)
        {
            const auto old = *visiblePos;
            visiblePos.reset();
            repaint (cursorBounds (old));
        }
    }

    void paint (juce::Graphics& g) override
    {
        if (! visiblePos) return;

        const float x = (float) visiblePos->x;
        const float y = (float) visiblePos->y;

        juce::Path arrow;        // classic left-tip pointer, hot-spot at (x, y)
        arrow.startNewSubPath (x,         y);
        arrow.lineTo          (x,         y + 16.0f);
        arrow.lineTo          (x + 4.2f,  y + 12.3f);
        arrow.lineTo          (x + 7.0f,  y + 17.6f);
        arrow.lineTo          (x + 9.6f,  y + 16.5f);
        arrow.lineTo          (x + 6.8f,  y + 11.2f);
        arrow.lineTo          (x + 11.6f, y + 11.2f);
        arrow.closeSubPath();

        g.setColour (juce::Colours::white);
        g.fillPath (arrow);
        g.setColour (juce::Colours::black);
        g.strokePath (arrow, juce::PathStrokeType (1.0f));
    }

private:
    static juce::Rectangle<int> cursorBounds (juce::Point<int> p)
    {
        return { p.x - 1, p.y - 1, 16, 20 };   // arrow extent plus 1px stroke
    }

    std::optional<juce::Point<int>> visiblePos;
};

//==============================================================================
// TB-303 front-panel look: black knobs with a white pointer over a printed
// 0–10 tick scale, red-LED buttons, and a saw/square slide switch — drawn in
// the style of the original hardware panel.
class TB303LookAndFeel : public juce::LookAndFeel_V4
{
public:
    TB303LookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool highlighted, bool down) override;
};

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

    // software-cursor tracking (global mouse listener)
    void mouseMove  (const juce::MouseEvent&) override;
    void mouseDrag  (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit  (const juce::MouseEvent&) override;
    void updateSoftwareCursor (const juce::MouseEvent&);

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

    // synth controls (panel order of the original: tempo, waveform,
    // tuning, cut off freq, resonance, env mod, decay, accent, volume)
    Knob tempo, tuning, cutoff, reso, envmod, decay, accent, volume;
    juce::Slider waveSwitch;
    juce::Label  waveLabel;
    std::unique_ptr<SliderAttachment> waveAttach;

    // sequencer transport
    juce::ToggleButton playButton { "RUN/STOP" }, syncButton { "SYNC" };
    juce::Slider       octaveSlider;
    juce::ComboBox     rootBox;
    juce::Label        rootLabel, octaveLabel;
    std::unique_ptr<ButtonAttachment> playAtt, syncAtt;
    std::unique_ptr<SliderAttachment> octaveAtt;
    std::unique_ptr<ComboAttachment>  rootAtt;

    // sequencer grid
    std::array<StepUI, AcidBaddProcessor::kNumSteps> steps;
    juce::Label rowLabelPitch, rowLabelGate, rowLabelAccent, rowLabelSlide;
    juce::Rectangle<int> gridArea;     // for drawing the play-head LEDs
    juce::Rectangle<int> panelArea;    // silver face (drawn in paint)
    juce::Rectangle<int> seqArea;      // black sequencer section
    int highlightStep = -1;

    juce::MidiKeyboardComponent keyboard;
    TB303LookAndFeel lnf;

    CursorOverlay cursorOverlay;
    bool softwareCursorEnabled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AcidBaddEditor)
};

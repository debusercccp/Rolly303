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
// 303-style pattern editor: a piano-roll grid. Each of the 16 steps gets one
// note, picked on the vertical keyboard at the left (click a cell to set the
// note; click the lit cell again to make the step a rest). Two rows under the
// grid toggle ACCENT and SLIDE per step — exactly the data the hardware lets
// you enter. It reads and writes the p*/g*/a*/s* parameters directly.
class StepPianoRoll : public juce::Component,
                      private juce::Timer
{
public:
    explicit StepPianoRoll (Rolly303Processor&);
    ~StepPianoRoll() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void editAt (juce::Point<int> pos, bool isDrag);

    int  pitchOf (int step) const;
    bool flagOf  (const juce::String& prefix, int step) const;
    void setNorm (const juce::String& id, float value01);
    void setPitch (int step, int pitch);

    static constexpr int kPitches = 25;   // pitch parameter range is 0..24

    Rolly303Processor& proc;

    juce::Rectangle<int> keyCol, header, noteGrid, accentRow, slideRow;
    int          lastPlayHead = -2;
    juce::int64  lastSnapshot = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepPianoRoll)
};

//==============================================================================
class Rolly303Editor : public juce::AudioProcessorEditor
{
public:
    explicit Rolly303Editor (Rolly303Processor&);
    ~Rolly303Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
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

    void addKnob (Knob& k, const juce::String& paramID, const juce::String& name);
    void randomizePattern();

    Rolly303Processor& processor;

    // synth controls (panel order of the original: tempo, waveform,
    // tuning, cut off freq, resonance, env mod, decay, accent, volume)
    Knob tempo, tuning, cutoff, reso, envmod, decay, accent, volume;
    juce::Slider waveSwitch;
    juce::Label  waveLabel;
    std::unique_ptr<SliderAttachment> waveAttach;

    // sequencer transport
    juce::ToggleButton playButton { "RUN/STOP" }, syncButton { "SYNC" };
    juce::TextButton   randomizeButton { "RANDOMIZE" };
    juce::Slider       octaveSlider;
    juce::ComboBox     rootBox, scaleBox;
    juce::Label        rootLabel, octaveLabel, scaleLabel;
    std::unique_ptr<ButtonAttachment> playAtt, syncAtt;
    std::unique_ptr<SliderAttachment> octaveAtt;
    std::unique_ptr<ComboAttachment>  rootAtt, scaleAtt;

    // sequencer pattern editor (piano roll)
    StepPianoRoll pianoRoll;
    juce::Rectangle<int> gridArea;     // bounds of the piano roll (for the well)
    juce::Rectangle<int> panelArea;    // silver face (drawn in paint)
    juce::Rectangle<int> seqArea;      // sequencer deck (drawn in paint)

    juce::MidiKeyboardComponent keyboard;
    TB303LookAndFeel lnf;

    CursorOverlay cursorOverlay;
    bool softwareCursorEnabled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Rolly303Editor)
};

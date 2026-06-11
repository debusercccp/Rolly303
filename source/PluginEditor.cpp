#include "PluginEditor.h"

namespace
{
    const juce::Colour kBg     { 0xff1a1410 };
    const juce::Colour kPanel  { 0xff2b2018 };
    const juce::Colour kAccent { 0xffff7a18 }; // 303 orange
    const juce::Colour kText   { 0xffe8ddd0 };

    void styleRowLabel (juce::Label& l, const juce::String& t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centredRight);
        l.setFont (juce::Font (11.0f, juce::Font::bold));
    }
}

//==============================================================================
AcidBaddEditor::AcidBaddEditor (AcidBaddProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    lnf.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    lnf.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff4a3a2a));
    lnf.setColour (juce::Slider::thumbColourId, kText);
    lnf.setColour (juce::Slider::trackColourId, kAccent);
    lnf.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff3a2c20));
    lnf.setColour (juce::Slider::textBoxTextColourId, kText);
    lnf.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    lnf.setColour (juce::Label::textColourId, kText);
    lnf.setColour (juce::ToggleButton::textColourId, kText);
    lnf.setColour (juce::ToggleButton::tickColourId, kAccent);
    lnf.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff4a3a2a));
    setLookAndFeel (&lnf);

    addKnob (tuning, "tuning",    "TUNING");
    addKnob (cutoff, "cutoff",    "CUTOFF");
    addKnob (reso,   "resonance", "RESO");
    addKnob (envmod, "envmod",    "ENV MOD");
    addKnob (decay,  "decay",     "DECAY");
    addKnob (accent, "accent",    "ACCENT");
    addKnob (volume, "volume",    "VOLUME");

    waveBox.addItem ("Sawtooth", 1);
    waveBox.addItem ("Square",   2);
    waveBox.setColour (juce::ComboBox::backgroundColourId, kPanel);
    waveBox.setColour (juce::ComboBox::textColourId, kText);
    waveBox.setColour (juce::ComboBox::outlineColourId, kAccent);
    addAndMakeVisible (waveBox);
    waveAttach = std::make_unique<ComboAttachment> (processor.apvts, "wave", waveBox);

    waveLabel.setText ("WAVEFORM", juce::dontSendNotification);
    waveLabel.setJustificationType (juce::Justification::centred);
    waveLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    addAndMakeVisible (waveLabel);

    // --- transport -----------------------------------------------------------
    playButton.setColour (juce::ToggleButton::textColourId, kAccent);
    addAndMakeVisible (playButton);
    addAndMakeVisible (syncButton);
    playAtt = std::make_unique<ButtonAttachment> (processor.apvts, "playing",  playButton);
    syncAtt = std::make_unique<ButtonAttachment> (processor.apvts, "syncHost", syncButton);

    tempoSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    tempoSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 18);
    addAndMakeVisible (tempoSlider);
    tempoAtt = std::make_unique<SliderAttachment> (processor.apvts, "tempo", tempoSlider);
    tempoLabel.setText ("TEMPO", juce::dontSendNotification);
    tempoLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    addAndMakeVisible (tempoLabel);

    rootBox.addItemList ({ "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" }, 1);
    rootBox.setColour (juce::ComboBox::backgroundColourId, kPanel);
    rootBox.setColour (juce::ComboBox::textColourId, kText);
    rootBox.setColour (juce::ComboBox::outlineColourId, kAccent);
    addAndMakeVisible (rootBox);
    rootAtt = std::make_unique<ComboAttachment> (processor.apvts, "root", rootBox);
    rootLabel.setText ("ROOT", juce::dontSendNotification);
    rootLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    addAndMakeVisible (rootLabel);

    octaveSlider.setSliderStyle (juce::Slider::IncDecButtons);
    octaveSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 36, 18);
    addAndMakeVisible (octaveSlider);
    octaveAtt = std::make_unique<SliderAttachment> (processor.apvts, "octave", octaveSlider);
    octaveLabel.setText ("OCTAVE", juce::dontSendNotification);
    octaveLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    addAndMakeVisible (octaveLabel);

    // --- step grid -----------------------------------------------------------
    styleRowLabel (rowLabelPitch,  "PITCH");
    styleRowLabel (rowLabelGate,   "GATE");
    styleRowLabel (rowLabelAccent, "ACCENT");
    styleRowLabel (rowLabelSlide,  "SLIDE");
    addAndMakeVisible (rowLabelPitch);
    addAndMakeVisible (rowLabelGate);
    addAndMakeVisible (rowLabelAccent);
    addAndMakeVisible (rowLabelSlide);

    for (int i = 0; i < (int) steps.size(); ++i)
    {
        auto& st = steps[(size_t) i];
        const auto s = juce::String (i);

        st.pitch.setSliderStyle (juce::Slider::LinearVertical);
        st.pitch.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 30, 14);
        st.pitch.setColour (juce::Slider::textBoxTextColourId, kText.withAlpha (0.7f));
        addAndMakeVisible (st.pitch);
        st.pitchAtt = std::make_unique<SliderAttachment> (processor.apvts, "p" + s, st.pitch);

        for (auto* b : { &st.gate, &st.accent, &st.slide })
        {
            b->setButtonText ({});
            addAndMakeVisible (*b);
        }
        st.gateAtt   = std::make_unique<ButtonAttachment> (processor.apvts, "g" + s, st.gate);
        st.accentAtt = std::make_unique<ButtonAttachment> (processor.apvts, "a" + s, st.accent);
        st.slideAtt  = std::make_unique<ButtonAttachment> (processor.apvts, "s" + s, st.slide);
    }

    keyboard.setLowestVisibleKey (36);
    addAndMakeVisible (keyboard);

    // Software cursor: draw our own pointer when the host X server has no
    // visible hardware cursor. run-linux.sh sets this when it spins up a
    // bare Xwayland; leave it unset in a DAW so the system cursor is used.
    softwareCursorEnabled =
        juce::SystemStats::getEnvironmentVariable ("ACIDBADD_SOFTWARE_CURSOR", "0") != "0";

    if (softwareCursorEnabled)
    {
        addAndMakeVisible (cursorOverlay);
        cursorOverlay.moveTo (getMouseXYRelative());
        juce::Desktop::getInstance().addGlobalMouseListener (this);
    }

    setSize (900, 600);
    startTimerHz (30);
}

AcidBaddEditor::~AcidBaddEditor()
{
    if (softwareCursorEnabled)
        juce::Desktop::getInstance().removeGlobalMouseListener (this);

    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void AcidBaddEditor::updateSoftwareCursor (const juce::MouseEvent& e)
{
    const auto local = cursorOverlay.getLocalPoint (nullptr, e.getScreenPosition());
    if (cursorOverlay.getLocalBounds().contains (local))
        cursorOverlay.moveTo (local);
    else
        cursorOverlay.hide();
}

void AcidBaddEditor::mouseMove  (const juce::MouseEvent& e) { updateSoftwareCursor (e); }
void AcidBaddEditor::mouseDrag  (const juce::MouseEvent& e) { updateSoftwareCursor (e); }
void AcidBaddEditor::mouseEnter (const juce::MouseEvent& e) { updateSoftwareCursor (e); }
void AcidBaddEditor::mouseExit  (const juce::MouseEvent& e) { updateSoftwareCursor (e); }

//==============================================================================
void AcidBaddEditor::timerCallback()
{
    const int s = processor.playingStep.load();
    if (s != highlightStep)
    {
        highlightStep = s;
        repaint (gridArea);
    }
}

//==============================================================================
void AcidBaddEditor::addKnob (Knob& k, const juce::String& paramID, const juce::String& name)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
    addAndMakeVisible (k.slider);

    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (juce::Font (12.0f, juce::Font::bold));
    addAndMakeVisible (k.label);

    k.attach = std::make_unique<SliderAttachment> (processor.apvts, paramID, k.slider);
}

//==============================================================================
void AcidBaddEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    auto top = getLocalBounds().removeFromTop (54).toFloat();
    g.setColour (kPanel);
    g.fillRect (top);
    g.setColour (kAccent);
    g.setFont (juce::Font (26.0f, juce::Font::bold));
    g.drawText ("AcidBadd  303", top.reduced (16, 0),
                juce::Justification::centredLeft);
    g.setColour (kText.withAlpha (0.6f));
    g.setFont (juce::Font (12.0f));
    g.drawText ("TB-303 BASS LINE EMULATION", top.reduced (16, 0),
                juce::Justification::centredRight);

    // play-head column highlight behind the grid
    if (highlightStep >= 0 && ! gridArea.isEmpty())
    {
        const int n = (int) steps.size();
        const float w = gridArea.getWidth() / (float) n;
        const float x = gridArea.getX() + highlightStep * w;
        g.setColour (kAccent.withAlpha (0.18f));
        g.fillRect (juce::Rectangle<float> (x, (float) gridArea.getY(), w, (float) gridArea.getHeight()));
    }
}

void AcidBaddEditor::resized()
{
    if (softwareCursorEnabled)
    {
        cursorOverlay.setBounds (getLocalBounds());
        cursorOverlay.toFront (false);
    }

    auto area = getLocalBounds();
    area.removeFromTop (54);

    // --- on-screen keyboard along the bottom ---------------------------------
    keyboard.setBounds (area.removeFromBottom (74).reduced (8, 6));

    // --- synth knob row ------------------------------------------------------
    auto knobRow = area.removeFromTop (132).reduced (8, 4);
    {
        auto waveCol = knobRow.removeFromLeft (104);
        waveLabel.setBounds (waveCol.removeFromTop (18));
        waveBox.setBounds (waveCol.removeFromTop (26).reduced (4, 0));

        Knob* knobs[] { &tuning, &cutoff, &reso, &envmod, &decay, &accent, &volume };
        const int n = (int) std::size (knobs);
        const int w = knobRow.getWidth() / n;
        for (auto* k : knobs)
        {
            auto col = knobRow.removeFromLeft (w);
            k->label.setBounds (col.removeFromTop (16));
            k->slider.setBounds (col.reduced (3));
        }
    }

    // --- transport row -------------------------------------------------------
    auto transport = area.removeFromTop (40).reduced (8, 4);
    playButton.setBounds (transport.removeFromLeft (70));
    syncButton.setBounds (transport.removeFromLeft (80));
    transport.removeFromLeft (10);
    tempoLabel.setBounds (transport.removeFromLeft (46));
    tempoSlider.setBounds (transport.removeFromLeft (130));
    transport.removeFromLeft (16);
    rootLabel.setBounds (transport.removeFromLeft (40));
    rootBox.setBounds (transport.removeFromLeft (64).reduced (0, 6));
    transport.removeFromLeft (16);
    octaveLabel.setBounds (transport.removeFromLeft (54));
    octaveSlider.setBounds (transport.removeFromLeft (96).reduced (0, 6));

    // --- step grid -----------------------------------------------------------
    auto grid = area.reduced (8, 4);
    auto labelCol = grid.removeFromLeft (46);
    gridArea = grid;

    const int n = (int) steps.size();
    const float colW = grid.getWidth() / (float) n;

    // Vertical layout of one column: pitch slider, then the three toggle rows.
    const int btnH = 22;
    const int pitchH = grid.getHeight() - 3 * btnH;

    // Row labels on the left, aligned to the toggle rows.
    auto lblBlock = labelCol;
    lblBlock.removeFromTop (pitchH);
    rowLabelPitch.setBounds (labelCol.getX(), grid.getY() + pitchH / 2 - 8, labelCol.getWidth() - 4, 16);
    rowLabelGate  .setBounds (lblBlock.removeFromTop (btnH).withTrimmedRight (4));
    rowLabelAccent.setBounds (lblBlock.removeFromTop (btnH).withTrimmedRight (4));
    rowLabelSlide .setBounds (lblBlock.removeFromTop (btnH).withTrimmedRight (4));

    for (int i = 0; i < n; ++i)
    {
        auto& st = steps[(size_t) i];
        juce::Rectangle<int> col (grid.getX() + (int) std::round (i * colW), grid.getY(),
                                  (int) std::ceil (colW), grid.getHeight());
        auto inner = col.reduced (2, 0);

        st.pitch.setBounds (inner.removeFromTop (pitchH));

        auto centreBtn = [] (juce::Rectangle<int> r)
        {
            return r.withSizeKeepingCentre (juce::jmin (20, r.getWidth()), juce::jmin (20, r.getHeight()));
        };
        st.gate.setBounds   (centreBtn (inner.removeFromTop (btnH)));
        st.accent.setBounds (centreBtn (inner.removeFromTop (btnH)));
        st.slide.setBounds  (centreBtn (inner.removeFromTop (btnH)));
    }
}

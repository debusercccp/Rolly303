#include "PluginEditor.h"

namespace
{
    // 303 panel palette
    const juce::Colour kCase      { 0xff141416 };   // dark case around the face
    const juce::Colour kFaceHi    { 0xffe2e2de };   // brushed-silver face
    const juce::Colour kFaceLo    { 0xffbcbcba };
    const juce::Colour kInk       { 0xff1c1c1c };   // panel print
    const juce::Colour kRed       { 0xffd02a20 };   // 303 red (LEDs / lettering)
    const juce::Colour kSeqPanel  { 0xff1d1d22 };   // black sequencer section
    const juce::Colour kSeqText   { 0xffd8d4cc };
    const juce::Colour kBtnBody   { 0xff34343c };

    void styleRowLabel (juce::Label& l, const juce::String& t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centredRight);
        l.setFont (juce::Font (11.0f, juce::Font::bold));
        l.setColour (juce::Label::textColourId, kSeqText);
    }

    void drawWaveGlyph (juce::Graphics& g, juce::Rectangle<float> r, bool square)
    {
        juce::Path p;
        const float y0 = r.getBottom(), y1 = r.getY();
        if (square)
        {
            p.startNewSubPath (r.getX(), y0);
            p.lineTo (r.getX(), y1);
            p.lineTo (r.getCentreX(), y1);
            p.lineTo (r.getCentreX(), y0);
            p.lineTo (r.getRight(), y0);
            p.lineTo (r.getRight(), y1);
        }
        else
        {
            p.startNewSubPath (r.getX(), y0);
            p.lineTo (r.getCentreX(), y1);
            p.lineTo (r.getCentreX(), y0);
            p.lineTo (r.getRight(), y1);
        }
        g.strokePath (p, juce::PathStrokeType (1.6f));
    }
}

//==============================================================================
TB303LookAndFeel::TB303LookAndFeel()
{
    setColour (juce::Label::textColourId, kInk);
    setColour (juce::Slider::textBoxTextColourId, kInk);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ToggleButton::textColourId, kSeqText);
    setColour (juce::ComboBox::backgroundColourId, kBtnBody);
    setColour (juce::ComboBox::textColourId, kSeqText);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff55555f));
    setColour (juce::ComboBox::arrowColourId, kSeqText);
    setColour (juce::PopupMenu::backgroundColourId, kSeqPanel);
    setColour (juce::PopupMenu::textColourId, kSeqText);
    setColour (juce::TextButton::buttonColourId, kBtnBody);
    setColour (juce::TextButton::textColourOffId, kSeqText);
}

void TB303LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                         float sliderPos, float startAngle, float endAngle,
                                         juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float size   = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto  centre = bounds.getCentre();
    const float radius = size * 0.5f;
    const float knobR  = radius * 0.66f;

    // printed 0–10 tick scale around the knob
    g.setColour (kInk);
    for (int i = 0; i <= 10; ++i)
    {
        const float ang = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
        const auto p1 = centre.getPointOnCircumference (knobR + 2.5f, ang);
        const auto p2 = centre.getPointOnCircumference (radius - (i % 5 == 0 ? 0.0f : 2.5f), ang);
        g.drawLine ({ p1, p2 }, i % 5 == 0 ? 1.6f : 1.1f);
    }

    // knob body: black with a subtle top-left sheen and skirt ring
    auto body = juce::Rectangle<float> (knobR * 2.0f, knobR * 2.0f).withCentre (centre);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff45454a), body.getTopLeft(),
                                             juce::Colour (0xff0a0a0c), body.getBottomRight(), false));
    g.fillEllipse (body);
    g.setColour (juce::Colours::black);
    g.drawEllipse (body, 1.2f);
    g.setColour (juce::Colour (0xff5a5a60));
    g.drawEllipse (body.reduced (knobR * 0.22f), 1.0f);

    // white pointer line
    const float ang = startAngle + sliderPos * (endAngle - startAngle);
    g.setColour (juce::Colours::white);
    g.drawLine ({ centre.getPointOnCircumference (knobR * 0.30f, ang),
                  centre.getPointOnCircumference (knobR - 2.0f,  ang) }, 2.4f);
}

void TB303LookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                                         float sliderPos, float, float,
                                         juce::Slider::SliderStyle style, juce::Slider& slider)
{
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);

    if (slider.getComponentID() == "waveswitch")
    {
        // 2-position slide switch with the saw / square glyphs beside it
        auto sw = r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 36.0f, 46.0f), 16.0f);
        g.setColour (kInk);
        drawWaveGlyph (g, { sw.getX() - 16.0f, sw.getY() + 3.0f, 11.0f, 10.0f }, false);  // saw
        drawWaveGlyph (g, { sw.getRight() + 6.0f, sw.getY() + 3.0f, 11.0f, 10.0f }, true); // square

        g.setColour (juce::Colour (0xff202022));
        g.fillRoundedRectangle (sw, 3.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (sw, 3.0f, 1.0f);

        const bool right = slider.getValue() > 0.5;
        auto thumb = sw.reduced (2.0f).removeFromLeft (sw.getWidth() * 0.5f - 2.0f)
                       .translated (right ? sw.getWidth() * 0.5f : 0.0f, 0.0f);
        g.setGradientFill (juce::ColourGradient (kFaceHi, thumb.getTopLeft(),
                                                 kFaceLo, thumb.getBottomRight(), false));
        g.fillRoundedRectangle (thumb, 2.0f);
        return;
    }

    if (style == juce::Slider::LinearVertical)   // sequencer pitch faders
    {
        auto track = r.withSizeKeepingCentre (4.0f, r.getHeight() - 8.0f);
        g.setColour (juce::Colour (0xff0d0d10));
        g.fillRoundedRectangle (track, 2.0f);

        auto thumb = juce::Rectangle<float> (juce::jmin (r.getWidth() - 4.0f, 22.0f), 9.0f)
                         .withCentre ({ r.getCentreX(), sliderPos });
        g.setGradientFill (juce::ColourGradient (kFaceHi, thumb.getTopLeft(),
                                                 kFaceLo, thumb.getBottomRight(), false));
        g.fillRoundedRectangle (thumb, 2.0f);
        g.setColour (kRed);
        g.fillRect (thumb.withSizeKeepingCentre (thumb.getWidth() - 4.0f, 2.0f));
        return;
    }

    LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, 0, 0, style, slider);
}

void TB303LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                         bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();

    if (b.getButtonText().isNotEmpty())
    {
        // panel button (RUN/STOP, SYNC): dark key with an LED on the left
        g.setColour (down ? kBtnBody.darker (0.4f)
                          : highlighted ? kBtnBody.brighter (0.15f) : kBtnBody);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);

        auto led = juce::Rectangle<float> (8.0f, 8.0f)
                       .withCentre ({ r.getX() + 12.0f, r.getCentreY() });
        g.setColour (on ? kRed : juce::Colour (0xff571712));
        g.fillEllipse (led);
        if (on)
        {
            g.setColour (kRed.withAlpha (0.35f));
            g.fillEllipse (led.expanded (3.0f));
        }

        g.setColour (kSeqText);
        g.setFont (juce::Font (12.0f, juce::Font::bold));
        g.drawText (b.getButtonText(), r.withTrimmedLeft (20.0f).toNearestInt(),
                    juce::Justification::centredLeft);
        return;
    }

    // sequencer step button: rounded key with a red LED that lights when on
    g.setColour (down ? kBtnBody.darker (0.4f)
                      : highlighted ? kBtnBody.brighter (0.15f) : kBtnBody);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);

    auto led = r.withSizeKeepingCentre (juce::jmin (r.getWidth(), r.getHeight()) - 8.0f,
                                        juce::jmin (r.getWidth(), r.getHeight()) - 8.0f);
    g.setColour (on ? kRed : juce::Colour (0xff571712));
    g.fillEllipse (led);
    if (on)
    {
        g.setColour (kRed.withAlpha (0.35f));
        g.fillEllipse (led.expanded (2.5f));
    }
}

//==============================================================================
AcidBaddEditor::AcidBaddEditor (AcidBaddProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    // knob row, in the original panel order
    addKnob (tempo,  "tempo",     "TEMPO");
    addKnob (tuning, "tuning",    "TUNING");
    addKnob (cutoff, "cutoff",    "CUT OFF FREQ");
    addKnob (reso,   "resonance", "RESONANCE");
    addKnob (envmod, "envmod",    "ENV MOD");
    addKnob (decay,  "decay",     "DECAY");
    addKnob (accent, "accent",    "ACCENT");
    addKnob (volume, "volume",    "VOLUME");

    // waveform slide switch (saw / square)
    waveSwitch.setComponentID ("waveswitch");
    waveSwitch.setSliderStyle (juce::Slider::LinearHorizontal);
    waveSwitch.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (waveSwitch);
    waveAttach = std::make_unique<SliderAttachment> (processor.apvts, "wave", waveSwitch);

    waveLabel.setText ("WAVEFORM", juce::dontSendNotification);
    waveLabel.setJustificationType (juce::Justification::centred);
    waveLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    addAndMakeVisible (waveLabel);

    // --- transport -----------------------------------------------------------
    addAndMakeVisible (playButton);
    addAndMakeVisible (syncButton);
    playAtt = std::make_unique<ButtonAttachment> (processor.apvts, "playing",  playButton);
    syncAtt = std::make_unique<ButtonAttachment> (processor.apvts, "syncHost", syncButton);

    rootBox.addItemList ({ "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" }, 1);
    addAndMakeVisible (rootBox);
    rootAtt = std::make_unique<ComboAttachment> (processor.apvts, "root", rootBox);
    rootLabel.setText ("ROOT", juce::dontSendNotification);
    rootLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    rootLabel.setColour (juce::Label::textColourId, kSeqText);
    addAndMakeVisible (rootLabel);

    octaveSlider.setSliderStyle (juce::Slider::IncDecButtons);
    octaveSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 36, 18);
    octaveSlider.setColour (juce::Slider::textBoxTextColourId, kSeqText);
    addAndMakeVisible (octaveSlider);
    octaveAtt = std::make_unique<SliderAttachment> (processor.apvts, "octave", octaveSlider);
    octaveLabel.setText ("OCTAVE", juce::dontSendNotification);
    octaveLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    octaveLabel.setColour (juce::Label::textColourId, kSeqText);
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
        st.pitch.setColour (juce::Slider::textBoxTextColourId, kSeqText.withAlpha (0.7f));
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

    setSize (960, 640);
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
        repaint (gridArea.expanded (0, 14));
    }
}

//==============================================================================
void AcidBaddEditor::addKnob (Knob& k, const juce::String& paramID, const juce::String& name)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (k.slider);

    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (juce::Font (10.5f, juce::Font::bold));
    addAndMakeVisible (k.label);

    k.attach = std::make_unique<SliderAttachment> (processor.apvts, paramID, k.slider);
}

//==============================================================================
void AcidBaddEditor::paint (juce::Graphics& g)
{
    g.fillAll (kCase);

    // brushed-silver face
    auto face = panelArea.toFloat();
    g.setGradientFill (juce::ColourGradient (kFaceHi, face.getTopLeft(),
                                             kFaceLo, face.getBottomLeft(), false));
    g.fillRoundedRectangle (face, 6.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (face, 6.0f, 1.5f);

    // panel lettering
    auto title = face.reduced (18.0f, 8.0f).removeFromTop (46.0f);
    g.setColour (kInk);
    g.setFont (juce::Font (30.0f, juce::Font::bold));
    g.drawText ("AcidBadd 303", title, juce::Justification::topLeft);
    g.setColour (kRed);
    g.setFont (juce::Font (15.0f, juce::Font::bold | juce::Font::italic));
    g.drawText ("Bass Line", title.translated (4.0f, 28.0f), juce::Justification::topLeft);
    g.setColour (kInk.withAlpha (0.75f));
    g.setFont (juce::Font (11.0f));
    g.drawText ("COMPUTER CONTROLLED", title, juce::Justification::topRight);

    // thin print line separating the title from the knob row
    g.setColour (kInk.withAlpha (0.5f));
    g.fillRect (face.reduced (14.0f, 0.0f).removeFromTop (52.0f).removeFromBottom (1.0f));

    // black sequencer section
    auto seq = seqArea.toFloat();
    g.setColour (kSeqPanel);
    g.fillRoundedRectangle (seq, 6.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (seq, 6.0f, 1.5f);
    g.setColour (kRed);
    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.drawText ("PATTERN", seq.reduced (12.0f, 4.0f), juce::Justification::topLeft);

    // play-head: red LED above the active column + soft column glow
    if (highlightStep >= 0 && ! gridArea.isEmpty())
    {
        const int n = (int) steps.size();
        const float w = gridArea.getWidth() / (float) n;
        const float x = gridArea.getX() + highlightStep * w;

        g.setColour (kRed.withAlpha (0.12f));
        g.fillRect (juce::Rectangle<float> (x, (float) gridArea.getY(), w, (float) gridArea.getHeight()));

        auto led = juce::Rectangle<float> (7.0f, 7.0f)
                       .withCentre ({ x + w * 0.5f, (float) gridArea.getY() - 7.0f });
        g.setColour (kRed);
        g.fillEllipse (led);
        g.setColour (kRed.withAlpha (0.35f));
        g.fillEllipse (led.expanded (3.0f));
    }
}

void AcidBaddEditor::resized()
{
    if (softwareCursorEnabled)
    {
        cursorOverlay.setBounds (getLocalBounds());
        cursorOverlay.toFront (false);
    }

    auto area = getLocalBounds().reduced (10);

    // --- on-screen keyboard along the bottom ---------------------------------
    keyboard.setBounds (area.removeFromBottom (72).reduced (2, 4));

    // --- silver face: title + knob row ----------------------------------------
    panelArea = area.removeFromTop (220);
    auto knobRow = panelArea.withTrimmedTop (58).reduced (14, 6);

    {
        // waveform switch column on the left, like the panel's switch position
        auto waveCol = knobRow.removeFromLeft (110);
        waveLabel.setBounds (waveCol.removeFromTop (16));
        waveSwitch.setBounds (waveCol.withSizeKeepingCentre (waveCol.getWidth(), 36));

        Knob* knobs[] { &tempo, &tuning, &cutoff, &reso, &envmod, &decay, &accent, &volume };
        const int n = (int) std::size (knobs);
        const int w = knobRow.getWidth() / n;
        for (auto* k : knobs)
        {
            auto col = knobRow.removeFromLeft (w);
            k->label.setBounds (col.removeFromTop (16));
            k->slider.setBounds (col.reduced (2));
        }
    }

    area.removeFromTop (8);

    // --- black sequencer section ----------------------------------------------
    seqArea = area;
    auto seq = seqArea.reduced (12, 8);

    auto transport = seq.removeFromTop (34);
    transport.removeFromLeft (70);                       // clear the PATTERN print
    playButton.setBounds (transport.removeFromLeft (96));
    transport.removeFromLeft (8);
    syncButton.setBounds (transport.removeFromLeft (72));
    transport.removeFromLeft (24);
    rootLabel.setBounds (transport.removeFromLeft (40));
    rootBox.setBounds (transport.removeFromLeft (64).reduced (0, 5));
    transport.removeFromLeft (16);
    octaveLabel.setBounds (transport.removeFromLeft (54));
    octaveSlider.setBounds (transport.removeFromLeft (96).reduced (0, 5));

    seq.removeFromTop (14);                              // room for the play-head LEDs

    // --- step grid -----------------------------------------------------------
    auto grid = seq;
    auto labelCol = grid.removeFromLeft (48);
    gridArea = grid;

    const int n = (int) steps.size();
    const float colW = grid.getWidth() / (float) n;

    // Vertical layout of one column: pitch slider, then the three toggle rows.
    const int btnH = 24;
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
            return r.withSizeKeepingCentre (juce::jmin (24, r.getWidth()), juce::jmin (20, r.getHeight()));
        };
        st.gate.setBounds   (centreBtn (inner.removeFromTop (btnH)));
        st.accent.setBounds (centreBtn (inner.removeFromTop (btnH)));
        st.slide.setBounds  (centreBtn (inner.removeFromTop (btnH)));
    }
}

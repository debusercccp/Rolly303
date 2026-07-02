#include "PluginEditor.h"

namespace
{
    // 303 panel palette — matched to the original hardware front-panel photo:
    // a light brushed-silver upper face over a darker brushed-grey lower deck.
    const juce::Colour kCase      { 0xff0e0e10 };   // near-black case rim
    const juce::Colour kFaceHi    { 0xffececeb };   // brushed-silver face (top)
    const juce::Colour kFaceLo    { 0xffc2c2c0 };   //                     (bottom)
    const juce::Colour kInk       { 0xff242424 };   // dark panel print
    const juce::Colour kRed       { 0xffc62a1f };   // 303 red (LEDs / lettering)
    const juce::Colour kSeqHi     { 0xff6f6f72 };   // brushed-grey lower deck (top)
    const juce::Colour kSeqLo     { 0xff48484b };   //                        (bottom)
    const juce::Colour kSeqPanel  { 0xff56565a };   // lower-deck mid tone
    const juce::Colour kSeqText   { 0xffeceae4 };   // light print on the lower deck
    const juce::Colour kBtnBody   { 0xff26262a };   // dark keys / step buttons
    const juce::Colour kInset     { 0xff1b1b1e };   // recessed display wells
    const juce::Colour kBackdrop  { 0xffb9e4f0 };   // soft cyan behind the unit
    const juce::Colour kBezelHi   { 0xffdedeDB };   // brushed-metal chassis frame
    const juce::Colour kBezelLo   { 0xff97978f };

    // small recessed Phillips screw, for the metal-chassis corners
    void drawScrew (juce::Graphics& g, juce::Point<float> c, float r = 5.0f)
    {
        g.setColour (juce::Colour (0xff7d7d77));
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colour (0xff3a3a37));
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.0f);
        g.setColour (juce::Colour (0xff2b2b28));   // cross slot
        g.drawLine (c.x - r * 0.6f, c.y, c.x + r * 0.6f, c.y, 1.2f);
        g.drawLine (c.x, c.y - r * 0.6f, c.x, c.y + r * 0.6f, 1.2f);
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
Rolly303Editor::Rolly303Editor (Rolly303Processor& p)
    : AudioProcessorEditor (p),
      processor (p),
      pianoRoll (p),
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

    // modern extras row (overdrive + delay, as on the TB-03)
    addKnob (drive,     "drive",     "OVERDRIVE");
    addKnob (delayTime, "delaytime", "DELAY TIME");
    addKnob (delayFb,   "delayfb",   "FEEDBACK");
    addKnob (delayMix,  "delaymix",  "DELAY MIX");

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

    addAndMakeVisible (randomizeButton);
    randomizeButton.onClick = [this] { randomizePattern(); };

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

    modeBox.addItemList ({ "Forward", "Reverse", "Ping-Pong", "Random" }, 1);
    addAndMakeVisible (modeBox);
    modeAtt = std::make_unique<ComboAttachment> (processor.apvts, "playmode", modeBox);
    modeLabel.setText ("MODE", juce::dontSendNotification);
    modeLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    modeLabel.setColour (juce::Label::textColourId, kSeqText);
    addAndMakeVisible (modeLabel);

    scaleBox.addItemList (acidscale::names(), 1);
    addAndMakeVisible (scaleBox);
    scaleAtt = std::make_unique<ComboAttachment> (processor.apvts, "scale", scaleBox);
    scaleLabel.setText ("SCALE", juce::dontSendNotification);
    scaleLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    scaleLabel.setColour (juce::Label::textColourId, kSeqText);
    addAndMakeVisible (scaleLabel);

    // --- pattern editor (piano roll) -----------------------------------------
    addAndMakeVisible (pianoRoll);

    keyboard.setLowestVisibleKey (36);
    addAndMakeVisible (keyboard);

    // Software cursor: draw our own pointer so the app is usable even when the
    // X server provides no visible hardware cursor (bare rootful Xwayland,
    // xwayland-satellite, ...). On the Linux standalone this is ON by default,
    // because whether the display has a visible cursor can't be detected
    // reliably; ROLLY303_SOFTWARE_CURSOR=0 forces it off, =1 forces it on
    // (e.g. for the plugin inside a DAW on such a display).
    const auto cursorEnv =
        juce::SystemStats::getEnvironmentVariable ("ROLLY303_SOFTWARE_CURSOR", "auto");
#if JUCE_LINUX
    const bool cursorAuto = juce::JUCEApplicationBase::isStandaloneApp();
#else
    const bool cursorAuto = false;
#endif
    softwareCursorEnabled = cursorEnv == "auto" ? cursorAuto : cursorEnv != "0";

    if (softwareCursorEnabled)
    {
        // Hide the hardware cursor over the editor and every control, so a
        // desktop that does show one doesn't end up with two pointers.
        hideHardwareCursor (*this);

        addAndMakeVisible (cursorOverlay);
        cursorOverlay.moveTo (getMouseXYRelative());
        juce::Desktop::getInstance().addGlobalMouseListener (this);
    }

    setSize (1000, 720);
}

Rolly303Editor::~Rolly303Editor()
{
    if (softwareCursorEnabled)
        juce::Desktop::getInstance().removeGlobalMouseListener (this);

    setLookAndFeel (nullptr);
}

//==============================================================================
void Rolly303Editor::hideHardwareCursor (juce::Component& c)
{
    c.setMouseCursor (juce::MouseCursor::NoCursor);
    for (auto* child : c.getChildren())
        hideHardwareCursor (*child);
}

//==============================================================================
void Rolly303Editor::updateSoftwareCursor (const juce::MouseEvent& e)
{
    const auto local = cursorOverlay.getLocalPoint (nullptr, e.getScreenPosition());
    if (cursorOverlay.getLocalBounds().contains (local))
        cursorOverlay.moveTo (local);
    else
        cursorOverlay.hide();
}

void Rolly303Editor::mouseMove  (const juce::MouseEvent& e) { updateSoftwareCursor (e); }
void Rolly303Editor::mouseDrag  (const juce::MouseEvent& e) { updateSoftwareCursor (e); }
void Rolly303Editor::mouseEnter (const juce::MouseEvent& e) { updateSoftwareCursor (e); }
void Rolly303Editor::mouseExit  (const juce::MouseEvent& e) { updateSoftwareCursor (e); }

//==============================================================================
void Rolly303Editor::addKnob (Knob& k, const juce::String& paramID, const juce::String& name)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (k.slider);

    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (juce::Font (10.5f, juce::Font::bold));
    k.label.setColour (juce::Label::outlineColourId, kInk);   // boxed panel print
    addAndMakeVisible (k.label);

    k.attach = std::make_unique<SliderAttachment> (processor.apvts, paramID, k.slider);
}

//==============================================================================
void Rolly303Editor::randomizePattern()
{
    // Roll a fresh acid line whose notes stay within the selected scale (over the
    // two-octave step range), with mostly-on gates and some accents/slides.
    auto& vts = processor.apvts;
    juce::Random rnd;

    const int scaleIdx = (int) vts.getRawParameterValue ("scale")->load();
    const juce::uint16 mask = acidscale::maskAt (scaleIdx);

    juce::Array<int> pool;                       // in-scale pitches across 0..24
    for (int p = 0; p <= 24; ++p)
        if (mask & (juce::uint16) (1u << (p % 12)))
            pool.add (p);
    if (pool.isEmpty())                          // safety: fall back to chromatic
        for (int p = 0; p <= 24; ++p) pool.add (p);

    auto setNorm = [&] (const juce::String& id, float v01)
    {
        if (auto* p = vts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, v01));
            p->endChangeGesture();
        }
    };

    for (int i = 0; i < Rolly303Processor::kNumSteps; ++i)
    {
        const auto s = juce::String (i);

        const int pitch = pool[rnd.nextInt (pool.size())];
        if (auto* p = vts.getParameter ("p" + s))
            setNorm ("p" + s, p->convertTo0to1 ((float) pitch));

        setNorm ("g" + s, rnd.nextFloat() < 0.85f ? 1.0f : 0.0f);   // ~15% rests
        setNorm ("a" + s, rnd.nextFloat() < 0.30f ? 1.0f : 0.0f);   // ~30% accents
        setNorm ("s" + s, rnd.nextFloat() < 0.20f ? 1.0f : 0.0f);   // ~20% slides
    }
}

//==============================================================================
void Rolly303Editor::paint (juce::Graphics& g)
{
    // soft cyan backdrop, like the unit photographed on a light surface
    g.fillAll (kBackdrop);
    g.setGradientFill (juce::ColourGradient (kBackdrop.brighter (0.10f), 0.0f, 0.0f,
                                             kBackdrop.darker (0.12f), 0.0f, (float) getHeight(), false));
    g.fillRect (getLocalBounds());

    // --- brushed-metal chassis the modules are bolted onto -------------------
    auto body = getLocalBounds().reduced (5).toFloat();
    g.setGradientFill (juce::ColourGradient (kBezelHi, body.getTopLeft(),
                                             kBezelLo, body.getBottomLeft(), false));
    g.fillRoundedRectangle (body, 14.0f);
    g.setColour (juce::Colours::white.withAlpha (0.045f));
    for (float yy = body.getY() + 3.0f; yy < body.getBottom(); yy += 3.0f)
        g.drawHorizontalLine ((int) yy, body.getX() + 3.0f, body.getRight() - 3.0f);
    g.setColour (juce::Colour (0xff5a5a55));
    g.drawRoundedRectangle (body, 14.0f, 1.5f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawRoundedRectangle (body.reduced (1.5f), 13.0f, 1.0f);

    // corner mounting screws
    for (auto p : { body.getTopLeft(), body.getTopRight(),
                    body.getBottomLeft(), body.getBottomRight() })
        drawScrew (g, p.translated (p.x < body.getCentreX() ? 13.0f : -13.0f,
                                    p.y < body.getCentreY() ? 13.0f : -13.0f));

    // --- brushed-silver upper face -------------------------------------------
    auto face = panelArea.toFloat();
    g.setGradientFill (juce::ColourGradient (kFaceHi, face.getTopLeft(),
                                             kFaceLo, face.getBottomLeft(), false));
    g.fillRoundedRectangle (face, 6.0f);
    // faint horizontal brushing
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    for (float yy = face.getY() + 2.0f; yy < face.getBottom(); yy += 3.0f)
        g.drawHorizontalLine ((int) yy, face.getX() + 2.0f, face.getRight() - 2.0f);
    g.setColour (kCase);
    g.drawRoundedRectangle (face, 6.0f, 1.5f);

    // maker logo printed straight on the silver, top-left (photo: Roland logo)
    g.setColour (kInk);
    g.setFont (juce::Font (27.0f, juce::Font::bold | juce::Font::italic));
    g.drawText ("Rolly", juce::Rectangle<float> (face.getX() + 18.0f, face.getY() + 10.0f,
                                                 110.0f, 30.0f),
                juce::Justification::centredLeft);

    // model print beside the logo: TB-303 in ink, "Computer Controlled" in red
    g.setFont (juce::Font (16.0f, juce::Font::bold));
    g.drawText ("TB-303", juce::Rectangle<float> (face.getX() + 150.0f, face.getY() + 12.0f,
                                                  90.0f, 18.0f),
                juce::Justification::centredLeft);
    g.setColour (kRed);
    g.setFont (juce::Font (11.5f, juce::Font::bold));
    g.drawText ("Computer Controlled",
                juce::Rectangle<float> (face.getX() + 150.0f, face.getY() + 30.0f, 160.0f, 14.0f),
                juce::Justification::centredLeft);

    // black "Bass Line" plate, top-right (white script on black, as on the panel)
    auto plate = juce::Rectangle<float> (face.getRight() - 200.0f, face.getY() + 8.0f, 182.0f, 40.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2a2e), plate.getTopLeft(),
                                             juce::Colour (0xff0c0c0e), plate.getBottomLeft(), false));
    g.fillRoundedRectangle (plate, 5.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (plate, 5.0f, 1.0f);
    g.setColour (juce::Colour (0xfff2f2f0));
    g.setFont (juce::Font (26.0f, juce::Font::bold | juce::Font::italic));
    g.drawText ("Bass Line", plate.reduced (14.0f, 0.0f), juce::Justification::centred);

    // thin print line under the title band
    g.setColour (kInk.withAlpha (0.45f));
    g.fillRect (face.reduced (14.0f, 0.0f).removeFromTop (56.0f).removeFromBottom (1.0f));

    // --- brushed-grey lower deck ---------------------------------------------
    auto seq = seqArea.toFloat();
    g.setGradientFill (juce::ColourGradient (kSeqHi, seq.getTopLeft(),
                                             kSeqLo, seq.getBottomLeft(), false));
    g.fillRoundedRectangle (seq, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    for (float yy = seq.getY() + 2.0f; yy < seq.getBottom(); yy += 3.0f)
        g.drawHorizontalLine ((int) yy, seq.getX() + 2.0f, seq.getRight() - 2.0f);
    g.setColour (kCase);
    g.drawRoundedRectangle (seq, 6.0f, 1.5f);

    // recessed well behind the step grid
    if (! gridArea.isEmpty())
    {
        auto well = gridArea.toFloat().expanded (8.0f, 18.0f);
        g.setColour (kInset);
        g.fillRoundedRectangle (well, 5.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (well, 5.0f, 1.0f);
    }

    g.setColour (kRed);
    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.drawText ("PATTERN", seq.reduced (12.0f, 4.0f), juce::Justification::topLeft);
    // (the piano-roll component draws the play-head and the pattern itself)
}

void Rolly303Editor::resized()
{
    if (softwareCursorEnabled)
    {
        cursorOverlay.setBounds (getLocalBounds());
        cursorOverlay.toFront (false);
    }

    auto area = getLocalBounds().reduced (10);

    // --- on-screen keyboard along the bottom ---------------------------------
    keyboard.setBounds (area.removeFromBottom (72).reduced (2, 4));

    // --- silver face: branding + knobs ----------------------------------------
    panelArea = area.removeFromTop (238);

    // VOLUME sits on the far right, spanning the full knob height (like the panel)
    auto faceBody = panelArea.withTrimmedTop (58).reduced (14, 8);
    {
        auto volCol = faceBody.removeFromRight (96);
        volume.label.setBounds (volCol.removeFromTop (16));
        volume.slider.setBounds (volCol.reduced (4));
    }

    // Top tone-knob row: TUNING, CUT OFF FREQ, RESONANCE, ENV MOD, DECAY, ACCENT
    auto knobRow = faceBody.removeFromTop (juce::jmax (96, faceBody.getHeight() - 70));
    {
        Knob* knobs[] { &tuning, &cutoff, &reso, &envmod, &decay, &accent };
        const int n = (int) std::size (knobs);
        const int w = knobRow.getWidth() / n;
        for (auto* k : knobs)
        {
            auto col = knobRow.removeFromLeft (w);
            k->label.setBounds (col.removeFromTop (16));
            k->slider.setBounds (col.reduced (3));
        }
    }

    // Lower-left control zone: TEMPO knob + WAVEFORM switch (panel's SHUFFLE
    // area), then the modern-extras knobs (overdrive + delay) beside them.
    faceBody.removeFromTop (6);
    {
        auto tempoCol = faceBody.removeFromLeft (96);
        tempo.label.setBounds (tempoCol.removeFromTop (16));
        tempo.slider.setBounds (tempoCol.reduced (4));

        faceBody.removeFromLeft (12);
        auto waveCol = faceBody.removeFromLeft (130);
        waveLabel.setBounds (waveCol.removeFromTop (16));
        waveSwitch.setBounds (waveCol.withSizeKeepingCentre (waveCol.getWidth(), 34));

        faceBody.removeFromLeft (16);
        Knob* fx[] { &drive, &delayTime, &delayFb, &delayMix };
        const int fxW = faceBody.getWidth() / (int) std::size (fx);
        for (auto* k : fx)
        {
            auto col = faceBody.removeFromLeft (fxW);
            k->label.setBounds (col.removeFromTop (16).reduced (8, 0));
            k->slider.setBounds (col.reduced (3));
        }
    }

    area.removeFromTop (8);

    // --- sequencer deck -------------------------------------------------------
    seqArea = area;
    auto seq = seqArea.reduced (12, 8);

    auto transport = seq.removeFromTop (34);
    transport.removeFromLeft (70);                       // clear the PATTERN print
    playButton.setBounds (transport.removeFromLeft (92));
    transport.removeFromLeft (8);
    syncButton.setBounds (transport.removeFromLeft (64));
    transport.removeFromLeft (14);
    modeLabel.setBounds (transport.removeFromLeft (40));
    modeBox.setBounds (transport.removeFromLeft (92).reduced (0, 5));
    transport.removeFromLeft (12);
    rootLabel.setBounds (transport.removeFromLeft (36));
    rootBox.setBounds (transport.removeFromLeft (56).reduced (0, 5));
    transport.removeFromLeft (12);
    octaveLabel.setBounds (transport.removeFromLeft (50));
    octaveSlider.setBounds (transport.removeFromLeft (84).reduced (0, 5));

    transport.removeFromLeft (12);
    scaleLabel.setBounds (transport.removeFromLeft (42));
    scaleBox.setBounds (transport.removeFromLeft (104).reduced (0, 5));

    randomizeButton.setBounds (transport.removeFromRight (104).reduced (0, 3));

    seq.removeFromTop (6);

    // --- piano-roll pattern editor -------------------------------------------
    gridArea = seq;
    pianoRoll.setBounds (gridArea);
}

//==============================================================================
namespace
{
    constexpr bool isBlackKey (int semitone)
    {
        switch (((semitone % 12) + 12) % 12)
        {
            case 1: case 3: case 6: case 8: case 10: return true;
            default:                                 return false;
        }
    }

    const char* const kNoteNames[12] =
        { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
}

StepPianoRoll::StepPianoRoll (Rolly303Processor& p) : proc (p)
{
    setOpaque (true);
    startTimerHz (24);
}

StepPianoRoll::~StepPianoRoll() { stopTimer(); }

//==============================================================================
int  StepPianoRoll::pitchOf (int step) const
{
    return (int) proc.apvts.getRawParameterValue ("p" + juce::String (step))->load();
}

bool StepPianoRoll::flagOf (const juce::String& prefix, int step) const
{
    return proc.apvts.getRawParameterValue (prefix + juce::String (step))->load() > 0.5f;
}

void StepPianoRoll::setNorm (const juce::String& id, float value01)
{
    if (auto* param = proc.apvts.getParameter (id))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, value01));
        param->endChangeGesture();
    }
}

void StepPianoRoll::setPitch (int step, int pitch)
{
    const auto id = "p" + juce::String (step);
    if (auto* param = proc.apvts.getParameter (id))
        setNorm (id, param->convertTo0to1 ((float) juce::jlimit (0, kPitches - 1, pitch)));
}

//==============================================================================
void StepPianoRoll::resized()
{
    auto r = getLocalBounds();
    keyCol = r.removeFromLeft (48);          // full-height note keyboard / row labels
    header = r.removeFromTop (16);           // step numbers across the grid

    const int rowH = 18;
    slideRow  = r.removeFromBottom (rowH);
    accentRow = r.removeFromBottom (rowH);
    r.removeFromBottom (2);
    noteGrid  = r;
}

//==============================================================================
void StepPianoRoll::paint (juce::Graphics& g)
{
    const int   n     = Rolly303Processor::kNumSteps;
    const float colW  = noteGrid.getWidth()  / (float) n;
    const float rowH  = noteGrid.getHeight() / (float) kPitches;
    const int   ph    = proc.playingStep.load();

    auto cellX = [&] (int step)  { return noteGrid.getX() + step * colW; };
    auto rowY  = [&] (int pitch) { return noteGrid.getY() + (kPitches - 1 - pitch) * rowH; };

    // Root note and selected scale: pitch rows are relative to Root (pitch 0 is
    // the root), so the keyboard names, the black/white keys and the in-scale
    // highlight are all keyed to the chosen Root + Scale.
    const int          root  = (int) proc.apvts.getRawParameterValue ("root")->load();
    const juce::uint16 mask  = acidscale::maskAt ((int) proc.apvts.getRawParameterValue ("scale")->load());

    auto absSemi  = [&] (int pitch) { return ((root + pitch) % 12 + 12) % 12; };
    auto inScale  = [&] (int pitch) { return (mask & (juce::uint16) (1u << (pitch % 12))) != 0; };
    auto isTonic  = [&] (int pitch) { return (pitch % 12) == 0; };

    const juce::Colour kScaleTint { 0xff2f6f74 };   // teal lane for in-scale rows
    const juce::Colour kTonicTint { 0xff7a4a2a };   // warmer lane for the root rows

    g.fillAll (kInset);

    // --- left keyboard: one key per pitch row (named for the actual notes) ----
    for (int pitch = 0; pitch < kPitches; ++pitch)
    {
        const int  semis = absSemi (pitch);
        const bool black = isBlackKey (semis);
        juce::Rectangle<float> key ((float) keyCol.getX() + 2.0f, rowY (pitch),
                                    (float) keyCol.getWidth() - 3.0f, rowH);
        g.setColour (black ? juce::Colour (0xff232327) : juce::Colour (0xffd7d7d2));
        g.fillRect (key.reduced (0.0f, 0.5f));

        if (isTonic (pitch))            // mark the root key with a red tab
        {
            g.setColour (kRed);
            g.fillRect (key.getX(), key.getY() + 0.5f, 3.0f, rowH - 1.0f);
        }

        if (! black && rowH >= 9.0f)    // label the white keys (octave number on C)
        {
            const juce::String name = juce::String (kNoteNames[semis])
                                    + (semis == 0 ? juce::String (pitch / 12 + 1) : juce::String());
            g.setColour (juce::Colour (0xff3a3a3a));
            g.setFont (juce::Font (juce::jmin (11.0f, rowH - 1.0f)));
            g.drawText (name, key.reduced (6.0f, 0.0f), juce::Justification::centredLeft);
        }
    }

    // --- grid background: scale-highlighted lanes ----------------------------
    for (int pitch = 0; pitch < kPitches; ++pitch)
    {
        const juce::Rectangle<float> lane ((float) noteGrid.getX(), rowY (pitch),
                                           (float) noteGrid.getWidth(), rowH);
        if (isTonic (pitch))
        {
            g.setColour (kTonicTint.withAlpha (0.55f));
            g.fillRect (lane);
        }
        else if (inScale (pitch))
        {
            g.setColour (kScaleTint.withAlpha (0.30f));
            g.fillRect (lane);
        }
        else if (isBlackKey (absSemi (pitch)))   // out-of-scale: keep it recessed
        {
            g.setColour (juce::Colours::black.withAlpha (0.18f));
            g.fillRect (lane);
        }
    }

    for (int s = 0; s <= n; ++s)
    {
        const bool beat = (s % 4) == 0;
        g.setColour (juce::Colours::white.withAlpha (beat ? 0.16f : 0.06f));
        g.fillRect (cellX (s) - 0.5f, (float) noteGrid.getY(),
                    beat ? 1.4f : 0.8f, (float) noteGrid.getHeight());
    }

    // --- play-head column ----------------------------------------------------
    if (ph >= 0 && ph < n)
    {
        g.setColour (kRed.withAlpha (0.14f));
        g.fillRect (cellX (ph), (float) noteGrid.getY(), colW, (float) noteGrid.getHeight());
    }

    // --- the pattern: one note cell per step + accent / slide rows -----------
    for (int s = 0; s < n; ++s)
    {
        const int  pitch  = juce::jlimit (0, kPitches - 1, pitchOf (s));
        const bool gate   = flagOf ("g", s);
        const bool accent = flagOf ("a", s);
        const bool slide  = flagOf ("s", s);

        juce::Rectangle<float> cell (cellX (s) + 1.0f, rowY (pitch) + 1.0f,
                                     colW - 2.0f, rowH - 2.0f);

        if (gate)
        {
            // a slide step ties into the next step: draw a connecting bar
            if (slide)
            {
                g.setColour (kRed.withAlpha (0.55f));
                g.fillRect (cell.getX(), cell.getCentreY() - 2.0f,
                            colW, 4.0f);
            }
            g.setColour (accent ? juce::Colour (0xffff7a2a) : kRed);
            g.fillRoundedRectangle (cell, 2.5f);
            g.setColour (juce::Colours::white.withAlpha (0.25f));
            g.drawRoundedRectangle (cell, 2.5f, 1.0f);
        }
        else
        {
            // a rest still remembers its note — show a faint ghost outline
            g.setColour (kSeqText.withAlpha (0.18f));
            g.drawRoundedRectangle (cell, 2.5f, 1.0f);
        }
    }

    // --- step-number header --------------------------------------------------
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    for (int s = 0; s < n; ++s)
    {
        const bool beat = (s % 4) == 0;
        g.setColour (s == ph ? kRed : (beat ? kSeqText : kSeqText.withAlpha (0.55f)));
        g.drawText (juce::String (s + 1),
                    juce::Rectangle<float> (cellX (s), (float) header.getY(), colW, (float) header.getHeight()),
                    juce::Justification::centred);
    }

    // --- ACCENT / SLIDE toggle rows ------------------------------------------
    auto drawFlagRow = [&] (juce::Rectangle<int> row, const juce::String& prefix,
                            const juce::String& label, juce::Colour lit)
    {
        g.setColour (kSeqText);
        g.setFont (juce::Font (10.0f, juce::Font::bold));
        g.drawText (label, juce::Rectangle<int> (keyCol.getX(), row.getY(), keyCol.getWidth() - 4, row.getHeight()),
                    juce::Justification::centredRight);

        for (int s = 0; s < n; ++s)
        {
            juce::Rectangle<float> cell (cellX (s) + 2.0f, (float) row.getY() + 2.0f,
                                         colW - 4.0f, (float) row.getHeight() - 4.0f);
            const bool on = flagOf (prefix, s);
            g.setColour (on ? lit : kBtnBody);
            g.fillRoundedRectangle (cell, 3.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawRoundedRectangle (cell, 3.0f, 1.0f);
            if (on)
            {
                g.setColour (juce::Colours::white.withAlpha (0.85f));
                g.fillEllipse (cell.withSizeKeepingCentre (4.0f, 4.0f));
            }
        }
    };

    drawFlagRow (accentRow, "a", "ACCENT", juce::Colour (0xffff7a2a));
    drawFlagRow (slideRow,  "s", "SLIDE",  kRed);

    // outline
    g.setColour (juce::Colours::black);
    g.drawRect (getLocalBounds(), 1);
}

//==============================================================================
void StepPianoRoll::editAt (juce::Point<int> pos, bool isDrag)
{
    const int   n    = Rolly303Processor::kNumSteps;
    const float colW = noteGrid.getWidth()  / (float) n;
    const float rowH = noteGrid.getHeight() / (float) kPitches;

    if (noteGrid.contains (pos))
    {
        const int step = juce::jlimit (0, n - 1,
                            (int) ((pos.x - noteGrid.getX()) / colW));
        const int row  = juce::jlimit (0, kPitches - 1,
                            (int) ((pos.y - noteGrid.getY()) / rowH));
        const int pitch = kPitches - 1 - row;

        // Click the lit note again to turn the step into a rest; otherwise set
        // the note and make sure the step sounds. Dragging only paints notes.
        if (! isDrag && flagOf ("g", step) && pitchOf (step) == pitch)
        {
            setNorm ("g" + juce::String (step), 0.0f);
        }
        else
        {
            setPitch (step, pitch);
            setNorm ("g" + juce::String (step), 1.0f);
        }
        return;
    }

    if (isDrag) return;   // accent / slide toggles respond to clicks only

    auto toggleRow = [&] (juce::Rectangle<int> row, const juce::String& prefix)
    {
        if (! row.contains (pos)) return false;
        const int step = juce::jlimit (0, n - 1, (int) ((pos.x - noteGrid.getX()) / colW));
        setNorm (prefix + juce::String (step), flagOf (prefix, step) ? 0.0f : 1.0f);
        return true;
    };

    toggleRow (accentRow, "a") || toggleRow (slideRow, "s");
}

void StepPianoRoll::mouseDown (const juce::MouseEvent& e) { editAt (e.getPosition(), false); }
void StepPianoRoll::mouseDrag (const juce::MouseEvent& e) { editAt (e.getPosition(), true);  }

//==============================================================================
void StepPianoRoll::timerCallback()
{
    // Repaint only when the pattern or the play-head actually changed, so host
    // automation and external edits stay reflected without constant redraws.
    juce::int64 snap = 0;
    for (int s = 0; s < Rolly303Processor::kNumSteps; ++s)
    {
        snap = snap * 131 + pitchOf (s);
        snap = snap * 2 + (flagOf ("g", s) ? 1 : 0);
        snap = snap * 2 + (flagOf ("a", s) ? 1 : 0);
        snap = snap * 2 + (flagOf ("s", s) ? 1 : 0);
    }
    // root / scale change the highlight and key labels, so fold them in too
    snap = snap * 13 + (int) proc.apvts.getRawParameterValue ("root")->load();
    snap = snap * 13 + (int) proc.apvts.getRawParameterValue ("scale")->load();

    const int ph = proc.playingStep.load();
    if (snap != lastSnapshot || ph != lastPlayHead)
    {
        lastSnapshot = snap;
        lastPlayHead = ph;
        repaint();
    }
}

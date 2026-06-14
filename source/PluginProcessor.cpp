#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
namespace ids
{
    constexpr auto wave   = "wave";
    constexpr auto tuning = "tuning";
    constexpr auto cutoff = "cutoff";
    constexpr auto reso   = "resonance";
    constexpr auto envmod = "envmod";
    constexpr auto decay  = "decay";
    constexpr auto accent = "accent";
    constexpr auto volume = "volume";
}

//==============================================================================
Rolly303Processor::Rolly303Processor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout Rolly303Processor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ids::wave, 1 }, "Waveform",
        StringArray { "Sawtooth", "Square" }, 0));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::tuning, 1 }, "Tuning",
        NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f, "st"));

    // Hardware range: the 303's CUT OFF FREQ pot sweeps roughly 250 Hz – 2.4 kHz.
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::cutoff, 1 }, "Cutoff",
        NormalisableRange<float> (250.0f, 2400.0f, 0.1f, 0.5f), 750.0f, "Hz"));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::reso, 1 }, "Resonance",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.7f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::envmod, 1 }, "Env Mod",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.55f));

    // Hardware range: the 303's DECAY pot spans 200 ms – 2 s.
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::decay, 1 }, "Decay",
        NormalisableRange<float> (200.0f, 2000.0f, 1.0f, 0.5f), 400.0f, "ms"));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::accent, 1 }, "Accent",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.6f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::volume, 1 }, "Volume",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f));

    // --- sequencer transport -------------------------------------------------
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { "playing", 1 }, "Run", false));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { "syncHost", 1 }, "Sync to Host", true));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "tempo", 1 }, "Tempo",
        NormalisableRange<float> (40.0f, 240.0f, 0.1f), 130.0f, "BPM"));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { "root", 1 }, "Root Note",
        StringArray { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" }, 0));

    layout.add (std::make_unique<AudioParameterInt> (
        ParameterID { "octave", 1 }, "Octave", -2, 2, 0));

    // Scale used for the piano-roll highlight and the randomiser (relative to Root).
    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { "scale", 1 }, "Scale", acidscale::names(), 2 /* Minor */));

    // --- per-step pattern ----------------------------------------------------
    // A simple default acid riff so the sequencer makes noise out of the box.
    const int  defPitch [kNumSteps] = { 0,0,12,0, 3,0,0,7, 0,10,0,0, 5,0,0,3 };
    const bool defAccent[kNumSteps] = { 1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0 };
    const bool defSlide [kNumSteps] = { 0,0,1,0, 0,0,0,0, 0,1,0,0, 0,0,0,0 };

    for (int i = 0; i < kNumSteps; ++i)
    {
        const auto s = juce::String (i);
        layout.add (std::make_unique<AudioParameterInt> (
            ParameterID { "p" + s, 1 }, "Step " + juce::String (i + 1) + " Pitch",
            0, 24, defPitch[i]));
        layout.add (std::make_unique<AudioParameterBool> (
            ParameterID { "g" + s, 1 }, "Step " + juce::String (i + 1) + " Gate", true));
        layout.add (std::make_unique<AudioParameterBool> (
            ParameterID { "a" + s, 1 }, "Step " + juce::String (i + 1) + " Accent",
            (bool) defAccent[i]));
        layout.add (std::make_unique<AudioParameterBool> (
            ParameterID { "s" + s, 1 }, "Step " + juce::String (i + 1) + " Slide",
            (bool) defSlide[i]));
    }

    return layout;
}

//==============================================================================
void Rolly303Processor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    engine.prepare (sr);
    keyboardState.reset();
    seqStepPos = 0.0;
    seqLastStep = -1;
    seqCurNote = -1;
    seqGateOffPos = 1.0e18;
    wasPlaying = false;
    playingStep.store (-1);
}

bool Rolly303Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
}

void Rolly303Processor::updateEngineParams()
{
    acid::TB303Engine::Params p;
    p.wave        = (int) *apvts.getRawParameterValue (ids::wave) == 0
                        ? acid::Oscillator::Wave::Saw
                        : acid::Oscillator::Wave::Square;
    p.tuningSemis = apvts.getRawParameterValue (ids::tuning)->load();
    p.cutoffHz    = apvts.getRawParameterValue (ids::cutoff)->load();
    p.resonance   = apvts.getRawParameterValue (ids::reso)->load();
    p.envMod      = apvts.getRawParameterValue (ids::envmod)->load();
    p.decayMs     = apvts.getRawParameterValue (ids::decay)->load();
    p.accent      = apvts.getRawParameterValue (ids::accent)->load();
    p.volume      = apvts.getRawParameterValue (ids::volume)->load();
    engine.setParams (p);
}

//==============================================================================
Rolly303Processor::StepData Rolly303Processor::readStep (int index)
{
    const auto s = juce::String (index);
    StepData d;
    d.pitch  = (int) apvts.getRawParameterValue ("p" + s)->load();
    d.gate   = apvts.getRawParameterValue ("g" + s)->load() > 0.5f;
    d.accent = apvts.getRawParameterValue ("a" + s)->load() > 0.5f;
    d.slide  = apvts.getRawParameterValue ("s" + s)->load() > 0.5f;
    return d;
}

void Rolly303Processor::triggerStepMidi (juce::MidiBuffer& seq, int index, int sampleOffset)
{
    const StepData cur  = readStep (index);
    const StepData prev = readStep ((index + kNumSteps - 1) % kNumSteps);

    const int root   = (int) apvts.getRawParameterValue ("root")->load();
    const int octave = (int) apvts.getRawParameterValue ("octave")->load();
    const int note   = 36 + root + octave * 12 + cur.pitch;   // bass register

    if (cur.gate)
    {
        const float vel = cur.accent ? 1.0f : 0.5f;   // velocity >= 0.62 => accent

        if (prev.slide && seqCurNote >= 0)
        {
            // legato: sound the new note before releasing the old one so the
            // voice glides (engine treats an overlapping note-on as a slide).
            seq.addEvent (juce::MidiMessage::noteOn (1, note, vel), sampleOffset);
            if (seqCurNote != note)
                seq.addEvent (juce::MidiMessage::noteOff (1, seqCurNote), sampleOffset);
            seqCurNote = note;
        }
        else
        {
            if (seqCurNote >= 0)
                seq.addEvent (juce::MidiMessage::noteOff (1, seqCurNote), sampleOffset);
            seq.addEvent (juce::MidiMessage::noteOn (1, note, vel), sampleOffset);
            seqCurNote = note;
        }

        // Slide notes ring into the next step; others play a staccato half-step.
        seqGateOffPos = cur.slide ? 1.0e18 : std::floor (seqStepPos) + 0.5;
    }
    else // rest
    {
        if (seqCurNote >= 0)
        {
            seq.addEvent (juce::MidiMessage::noteOff (1, seqCurNote), sampleOffset);
            seqCurNote = -1;
        }
        seqGateOffPos = 1.0e18;
    }
}

void Rolly303Processor::renderSequencerMidi (juce::MidiBuffer& seq, int numSamples)
{
    double tempo = apvts.getRawParameterValue ("tempo")->load();
    const bool sync = apvts.getRawParameterValue ("syncHost")->load() > 0.5f;

    if (! wasPlaying)   // just started: restart the pattern from step 1
    {
        seqStepPos = 0.0;
        seqLastStep = -1;
        seqCurNote = -1;
        seqGateOffPos = 1.0e18;
    }

    // Lock to the host's tempo and transport position (e.g. Ableton's BPM).
    if (sync)
        if (auto* ph = getPlayHead())
            if (const auto pos = ph->getPosition())
            {
                if (const auto bpm = pos->getBpm(); bpm && *bpm > 0.0)
                    tempo = *bpm;
                if (pos->getIsPlaying())
                    if (const auto ppq = pos->getPpqPosition())
                        seqStepPos = *ppq * 4.0;   // 4 sixteenth-steps per quarter note
            }

    const double inc = tempo * 4.0 / 60.0 / sampleRate;   // steps advanced per sample

    for (int s = 0; s < numSamples; ++s)
    {
        const long long istep = (long long) std::floor (seqStepPos);
        if (istep != seqLastStep)
        {
            const int idx = (int) (((istep % kNumSteps) + kNumSteps) % kNumSteps);
            triggerStepMidi (seq, idx, s);
            seqLastStep = istep;
            playingStep.store (idx);
        }

        if (seqCurNote >= 0 && seqStepPos >= seqGateOffPos)
        {
            seq.addEvent (juce::MidiMessage::noteOff (1, seqCurNote), s);
            seqCurNote = -1;
            seqGateOffPos = 1.0e18;
        }

        seqStepPos += inc;
    }

    wasPlaying = true;
}

void Rolly303Processor::renderVoice (juce::AudioBuffer<float>& buffer,
                                     const juce::MidiBuffer& midi, int numSamples)
{
    auto* left = buffer.getWritePointer (0);
    int sample = 0;

    for (const auto meta : midi)
    {
        const int eventPos = juce::jlimit (0, numSamples, meta.samplePosition);
        while (sample < eventPos)
            left[sample++] = engine.renderSample();

        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
            engine.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff())
            engine.noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            engine.allNotesOff();
    }

    while (sample < numSamples)
        left[sample++] = engine.renderSample();

    // Mirror the mono voice to any additional output channels.
    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, left, numSamples);
}

//==============================================================================
void Rolly303Processor::processBlock (juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples = buffer.getNumSamples();

    // Merge on-screen keyboard input (standalone) with incoming MIDI.
    keyboardState.processNextMidiBuffer (midiMessages, 0, numSamples, true);

    updateEngineParams();

    const bool playing = apvts.getRawParameterValue ("playing")->load() > 0.5f;

    // The running sequencer generates its own MIDI for this block.
    juce::MidiBuffer seqMidi;
    if (playing)
    {
        renderSequencerMidi (seqMidi, numSamples);
    }
    else if (wasPlaying)           // sequencer just stopped: release its note
    {
        if (seqCurNote >= 0)
        {
            seqMidi.addEvent (juce::MidiMessage::noteOff (1, seqCurNote), 0);
            seqCurNote = -1;
        }
        wasPlaying = false;
        playingStep.store (-1);
    }

    // Drive the internal voice from the keyboard/external MIDI plus the
    // sequencer's MIDI, all sample-accurate and in timestamp order.
    juce::MidiBuffer voiceMidi;
    voiceMidi.addEvents (midiMessages, 0, numSamples, 0);
    voiceMidi.addEvents (seqMidi,      0, numSamples, 0);
    renderVoice (buffer, voiceMidi, numSamples);

    // The plugin's MIDI output carries the notes the sequencer produced, so the
    // pattern can be recorded or routed to other instruments in the host.
    midiMessages.clear();
    midiMessages.addEvents (seqMidi, 0, numSamples, 0);
}

//==============================================================================
juce::AudioProcessorEditor* Rolly303Processor::createEditor()
{
    return new Rolly303Editor (*this);
}

void Rolly303Processor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void Rolly303Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Rolly303Processor();
}

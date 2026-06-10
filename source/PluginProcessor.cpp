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
AcidBaddProcessor::AcidBaddProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AcidBaddProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ids::wave, 1 }, "Waveform",
        StringArray { "Sawtooth", "Square" }, 0));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::tuning, 1 }, "Tuning",
        NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f, "st"));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::cutoff, 1 }, "Cutoff",
        NormalisableRange<float> (30.0f, 6000.0f, 0.1f, 0.3f), 500.0f, "Hz"));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::reso, 1 }, "Resonance",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.7f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::envmod, 1 }, "Env Mod",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.55f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::decay, 1 }, "Decay",
        NormalisableRange<float> (30.0f, 2000.0f, 1.0f, 0.4f), 350.0f, "ms"));

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
        ParameterID { "syncHost", 1 }, "Sync to Host", false));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "tempo", 1 }, "Tempo",
        NormalisableRange<float> (40.0f, 240.0f, 0.1f), 130.0f, "BPM"));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { "root", 1 }, "Root Note",
        StringArray { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" }, 0));

    layout.add (std::make_unique<AudioParameterInt> (
        ParameterID { "octave", 1 }, "Octave", -2, 2, 0));

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
void AcidBaddProcessor::prepareToPlay (double sr, int)
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

bool AcidBaddProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
}

void AcidBaddProcessor::updateEngineParams()
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
AcidBaddProcessor::StepData AcidBaddProcessor::readStep (int index)
{
    const auto s = juce::String (index);
    StepData d;
    d.pitch  = (int) apvts.getRawParameterValue ("p" + s)->load();
    d.gate   = apvts.getRawParameterValue ("g" + s)->load() > 0.5f;
    d.accent = apvts.getRawParameterValue ("a" + s)->load() > 0.5f;
    d.slide  = apvts.getRawParameterValue ("s" + s)->load() > 0.5f;
    return d;
}

void AcidBaddProcessor::triggerStep (int index, double /*samplesPerStep*/)
{
    const StepData cur  = readStep (index);
    const StepData prev = readStep ((index + kNumSteps - 1) % kNumSteps);

    const int root   = (int) apvts.getRawParameterValue ("root")->load();
    const int octave = (int) apvts.getRawParameterValue ("octave")->load();
    const int note   = 36 + root + octave * 12 + cur.pitch;   // bass register

    if (cur.gate)
    {
        const float vel = cur.accent ? 1.0f : 0.7f;   // velocity >= 0.62 => accent

        if (prev.slide && seqCurNote >= 0)
        {
            // legato: new note while old one is held -> engine glides (no retrigger)
            engine.noteOn (note, vel);
            if (seqCurNote != note)
                engine.noteOff (seqCurNote);
            seqCurNote = note;
        }
        else
        {
            if (seqCurNote >= 0) { engine.noteOff (seqCurNote); seqCurNote = -1; }
            engine.noteOn (note, vel);                // fresh note -> retrigger envelope
            seqCurNote = note;
        }

        // Slide notes ring into the next step; others play a staccato half-step.
        seqGateOffPos = cur.slide ? 1.0e18 : std::floor (seqStepPos) + 0.5;
    }
    else // rest
    {
        if (seqCurNote >= 0) { engine.noteOff (seqCurNote); seqCurNote = -1; }
        seqGateOffPos = 1.0e18;
    }
}

void AcidBaddProcessor::renderSequencer (float* out, int numSamples)
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

    // Optionally lock to the host's tempo and transport position.
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
            triggerStep (idx, 1.0 / std::max (1.0e-9, inc));
            seqLastStep = istep;
            playingStep.store (idx);
        }

        if (seqCurNote >= 0 && seqStepPos >= seqGateOffPos)
        {
            engine.noteOff (seqCurNote);
            seqCurNote = -1;
            seqGateOffPos = 1.0e18;
        }

        out[s] = engine.renderSample();
        seqStepPos += inc;
    }

    wasPlaying = true;
}

//==============================================================================
void AcidBaddProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Merge on-screen keyboard input (standalone) with incoming MIDI.
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    updateEngineParams();

    const int numSamples = buffer.getNumSamples();
    auto* left = buffer.getWritePointer (0);
    const bool playing = apvts.getRawParameterValue ("playing")->load() > 0.5f;

    if (playing)
    {
        renderSequencer (left, numSamples);
    }
    else
    {
        if (wasPlaying)            // sequencer just stopped: silence the voice
        {
            engine.allNotesOff();
            seqCurNote = -1;
            wasPlaying = false;
            playingStep.store (-1);
        }

        int sample = 0;
        for (const auto meta : midiMessages)
        {
            const int eventPos = meta.samplePosition;
            while (sample < eventPos && sample < numSamples)
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
    }

    // Mirror mono voice to any additional output channels.
    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, left, numSamples);
}

//==============================================================================
juce::AudioProcessorEditor* AcidBaddProcessor::createEditor()
{
    return new AcidBaddEditor (*this);
}

void AcidBaddProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void AcidBaddProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AcidBaddProcessor();
}

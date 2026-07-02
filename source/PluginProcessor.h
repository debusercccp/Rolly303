#pragma once

#include <JuceHeader.h>
#include <vector>
#include "TB303Engine.h"

//==============================================================================
// Musical scales used both for the "scale" parameter (choice list) and for the
// piano-roll highlighting / randomiser. Each scale is a 12-bit mask where bit i
// means semitone i (measured from the root) belongs to the scale.
namespace acidscale
{
    struct Scale { const char* name; juce::uint16 mask; };

    inline juce::uint16 maskFor (std::initializer_list<int> degrees)
    {
        juce::uint16 r = 0;
        for (int d : degrees) r |= (juce::uint16) (1u << d);
        return r;
    }

    inline const std::vector<Scale>& all()
    {
        static const std::vector<Scale> s =
        {
            { "Chromatic",      0x0FFFu },
            { "Major",          maskFor ({ 0, 2, 4, 5, 7, 9, 11 }) },
            { "Minor",          maskFor ({ 0, 2, 3, 5, 7, 8, 10 }) },
            { "Harmonic Minor", maskFor ({ 0, 2, 3, 5, 7, 8, 11 }) },
            { "Melodic Minor",  maskFor ({ 0, 2, 3, 5, 7, 9, 11 }) },
            { "Dorian",         maskFor ({ 0, 2, 3, 5, 7, 9, 10 }) },
            { "Phrygian",       maskFor ({ 0, 1, 3, 5, 7, 8, 10 }) },
            { "Lydian",         maskFor ({ 0, 2, 4, 6, 7, 9, 11 }) },
            { "Mixolydian",     maskFor ({ 0, 2, 4, 5, 7, 9, 10 }) },
            { "Locrian",        maskFor ({ 0, 1, 3, 5, 6, 8, 10 }) },
        };
        return s;
    }

    inline juce::StringArray names()
    {
        juce::StringArray a;
        for (const auto& sc : all()) a.add (sc.name);
        return a;
    }

    inline juce::uint16 maskAt (int index)
    {
        const auto& a = all();
        return a[(size_t) juce::jlimit (0, (int) a.size() - 1, index)].mask;
    }
}

//==============================================================================
class Rolly303Processor : public juce::AudioProcessor
{
public:
    Rolly303Processor();
    ~Rolly303Processor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Rolly303"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    static constexpr int kNumSteps = 16;

    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "PARAMS", createLayout() };
    juce::MidiKeyboardState keyboardState;

    // 0-based index of the step currently being played (for UI highlight), or -1.
    std::atomic<int> playingStep { -1 };

    // Semitone offset applied to the running pattern; set by playing the
    // keyboard while the sequencer runs (hardware pattern-play transposition).
    std::atomic<int> seqTranspose { 0 };

private:
    void updateEngineParams();

    // --- step sequencer (emits MIDI; synced to the host tempo/transport) ------
    struct StepData { int pitch; bool gate, accent, slide; };
    StepData readStep (int index);
    void     renderSequencerMidi (juce::MidiBuffer& seq, int numSamples);
    void     triggerStepMidi (juce::MidiBuffer& seq, int index, int sampleOffset);
    void     renderVoice (juce::AudioBuffer<float>& buffer,
                          const juce::MidiBuffer& midi, int numSamples);
    void     applyDelay (float* channel, int numSamples);

    acid::TB303Engine engine;

    // --- delay effect (TB-03-style extra) --------------------------------------
    std::vector<float> delayBuf;    // 2 s circular buffer
    int                delayWrite = 0;

    juce::Random seqRandom;         // step picker for the Random play mode

    double    sampleRate = 44100.0;
    double    seqStepPos    = 0.0;     // absolute position in 16th-note steps
    long long seqLastStep   = -1;      // last absolute step index that fired
    int       seqCurNote    = -1;      // MIDI note currently sounding (or -1)
    double    seqGateOffPos = 1.0e18;  // step position at which to release the note
    bool      wasPlaying    = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Rolly303Processor)
};

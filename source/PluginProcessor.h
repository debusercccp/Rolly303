#pragma once

#include <JuceHeader.h>
#include "TB303Engine.h"

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

private:
    void updateEngineParams();

    // --- step sequencer (emits MIDI; synced to the host tempo/transport) ------
    struct StepData { int pitch; bool gate, accent, slide; };
    StepData readStep (int index);
    void     renderSequencerMidi (juce::MidiBuffer& seq, int numSamples);
    void     triggerStepMidi (juce::MidiBuffer& seq, int index, int sampleOffset);
    void     renderVoice (juce::AudioBuffer<float>& buffer,
                          const juce::MidiBuffer& midi, int numSamples);

    acid::TB303Engine engine;

    double    sampleRate = 44100.0;
    double    seqStepPos    = 0.0;     // absolute position in 16th-note steps
    long long seqLastStep   = -1;      // last absolute step index that fired
    int       seqCurNote    = -1;      // MIDI note currently sounding (or -1)
    double    seqGateOffPos = 1.0e18;  // step position at which to release the note
    bool      wasPlaying    = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Rolly303Processor)
};

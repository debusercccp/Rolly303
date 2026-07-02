// End-to-end regression test: drives the REAL Rolly303Processor (its
// sequencer generates the MIDI, the engine renders it) and analyses the
// rendered audio.
//
// The pattern's step 0 slides onto step 1 at the SAME pitch — the trigger of
// the stuck-note leak that used to turn every following note legato. Non-slide
// steps play staccato (gate closes at half step, ~8 ms release), so their
// second half must be silent; the slide step must tie through into the next.
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <cstdio>
#include <cmath>

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    Rolly303Processor proc;

    auto set = [&] (const juce::String& id, float plainValue)
    {
        auto* p = proc.apvts.getParameter (id);
        if (p == nullptr) { std::printf ("missing param %s\n", id.toRawUTF8()); std::exit (2); }
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
    };

    // internal clock, fixed tempo, forward mode, no FX tails in the gaps
    set ("syncHost", 0.0f);
    set ("tempo", 120.0f);
    set ("playmode", 0.0f);
    set ("delaymix", 0.0f);
    set ("drive", 0.0f);

    // pattern: step0 pitch 12 with SLIDE, step1 pitch 12 (same!), the rest
    // distinct pitches, no slides/accents, all gates on
    const int pitches[16] = { 12,12,0,3, 7,5,3,0, 12,10,7,5, 3,7,0,5 };
    for (int i = 0; i < 16; ++i)
    {
        const auto s = juce::String (i);
        set ("p" + s, (float) pitches[i]);
        set ("g" + s, 1.0f);
        set ("a" + s, 0.0f);
        set ("s" + s, i == 0 ? 1.0f : 0.0f);
    }
    set ("playing", 1.0f);

    constexpr double sr = 44100.0;
    constexpr int    block = 512;
    proc.prepareToPlay (sr, block);

    // render 4 bars
    const double stepLen = sr * 60.0 / 120.0 / 4.0;          // samples per 16th
    const int    total   = (int) (stepLen * 16.0 * 4.0);
    std::vector<float> audio; audio.reserve ((size_t) total + block);

    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    while ((int) audio.size() < total)
    {
        midi.clear();
        proc.processBlock (buf, midi);
        audio.insert (audio.end(), buf.getReadPointer (0), buf.getReadPointer (0) + block);
    }

    auto rmsOf = [&] (double from, double to)
    {
        const int a = juce::jlimit (0, total - 1, (int) from);
        const int b = juce::jlimit (a + 1, total, (int) to);
        double acc = 0.0;
        for (int i = a; i < b; ++i) acc += (double) audio[(size_t) i] * audio[(size_t) i];
        return std::sqrt (acc / (b - a));
    };

    // analyse bars 2..4 (bar 1 warms up; the leak accumulated from bar 1's slide)
    int failures = 0;
    for (int bar = 1; bar < 4; ++bar)
        for (int stp = 0; stp < 16; ++stp)
        {
            const double s0   = (bar * 16 + stp) * stepLen;
            const float  note = (float) rmsOf (s0 + 0.05 * stepLen, s0 + 0.45 * stepLen);
            const float  gap  = (float) rmsOf (s0 + 0.70 * stepLen, s0 + 0.95 * stepLen);

            if (note < 0.02f) { std::printf ("FAIL bar %d step %2d: no note (rms %.4f)\n", bar + 1, stp + 1, note); ++failures; }

            if (stp == 0)     // slide step: must tie THROUGH the gap into step 1
            {
                if (gap < 0.01f) { std::printf ("FAIL bar %d step  1: slide tie missing (gap rms %.4f)\n", bar + 1, gap); ++failures; }
            }
            else              // staccato step: gap must be silent
            {
                if (gap > 0.005f) { std::printf ("FAIL bar %d step %2d: gate stuck open (gap rms %.4f) — slided!\n", bar + 1, stp + 1, gap); ++failures; }
            }
        }

    std::printf ("%s (%d failures)\n", failures == 0 ? "ALL PASS" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}

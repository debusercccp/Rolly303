// Regression tests for the TB-303 voice (header-only, no JUCE needed).
//
// Covers the stuck-note leak: a slide onto the SAME pitch used to leave a
// duplicate entry in the held-note stack, so the stack never emptied again
// and every following note played legato/slided.
#include "TB303Engine.h"
#include <cstdio>

static void renderMs (acid::TB303Engine& e, int ms)
{
    for (int i = 0; i < 44 * ms; ++i)
        e.renderSample();
}

int main()
{
    acid::TB303Engine e;
    e.prepare (44100.0);
    e.setParams ({});

    int failures = 0;

    // --- same-pitch slide: on(60), legato on(60), single off(60) -------------
    e.noteOn (60, 0.5f);
    renderMs (e, 100);
    e.noteOn (60, 0.5f);          // slide onto the same pitch (was the leak)
    renderMs (e, 100);
    e.noteOff (60);
    renderMs (e, 1000);           // amp release is ~8 ms; 1 s is plenty
    if (e.isActive()) { std::puts ("FAIL: voice stuck after same-pitch slide"); ++failures; }
    else               std::puts ("ok: same-pitch slide releases");

    // --- the next note must start from silence and release normally ----------
    e.noteOn (62, 0.5f);
    renderMs (e, 50);
    e.noteOff (62);
    renderMs (e, 1000);
    if (e.isActive()) { std::puts ("FAIL: normal note stuck after the slide case"); ++failures; }
    else               std::puts ("ok: following note releases normally");

    // --- stray note-off for a pitch that is not held -------------------------
    e.noteOn (64, 0.5f);
    renderMs (e, 50);
    e.noteOff (60);               // not held: must be ignored, 64 keeps sounding
    if (! e.isActive()) { std::puts ("FAIL: stray note-off killed the voice"); ++failures; }
    e.noteOff (64);
    renderMs (e, 1000);
    if (e.isActive()) { std::puts ("FAIL: voice stuck after stray note-off"); ++failures; }
    else               std::puts ("ok: stray note-off ignored");

    std::printf ("%s (%d failures)\n", failures == 0 ? "ALL PASS" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}

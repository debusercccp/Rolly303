#pragma once

#include <cmath>
#include <vector>
#include <algorithm>

//==============================================================================
//  AcidBadd 303 — a faithful emulation of the Roland TB-303 mono voice.
//
//  Signal path (classic 303 topology):
//
//      MIDI ─▶ Slide(glide) ─▶ Oscillator(saw/square) ─▶ Diode-ladder LPF ─▶ VCA ─▶ out
//                                                            ▲                ▲
//                                       Env Mod × FilterEnv ─┘   AmpEnv ──────┘
//
//  The "acid" character comes from a single decay envelope that sweeps the
//  resonant low-pass filter on every note, plus per-note ACCENT (extra
//  punch/brightness) and SLIDE (portamento between legato notes).
//
//  This is a real-time DSP model, not a SPICE circuit reproduction, but it
//  reproduces the musically important behaviour of the original instrument.
//==============================================================================

namespace acid
{

constexpr double kPi = 3.14159265358979323846;

//==============================================================================
// PolyBLEP band-limited oscillator (saw / square), as on the 303's single VCO.
//==============================================================================
class Oscillator
{
public:
    enum class Wave { Saw = 0, Square = 1 };

    void setSampleRate (double sr) noexcept { sampleRate = sr; }
    void setWave (Wave w)          noexcept { wave = w; }
    void setFrequency (double hz)  noexcept { freq = hz; }
    void reset()                   noexcept { phase = 0.0; }

    inline float process() noexcept
    {
        const double dt = freq / sampleRate;          // phase increment 0..1
        double value;

        if (wave == Wave::Saw)
        {
            value = 2.0 * phase - 1.0;                 // naive saw
            value -= polyBlep (phase, dt);
        }
        else // Square (the 303's "pulse" position)
        {
            value = (phase < 0.5) ? 1.0 : -1.0;        // naive square
            value += polyBlep (phase, dt);
            double t2 = phase + 0.5;
            if (t2 >= 1.0) t2 -= 1.0;
            value -= polyBlep (t2, dt);
        }

        phase += dt;
        if (phase >= 1.0) phase -= 1.0;
        return (float) value;
    }

private:
    static inline double polyBlep (double t, double dt) noexcept
    {
        if (dt <= 0.0) return 0.0;
        if (t < dt)              { t /= dt;        return t + t - t * t - 1.0; }
        if (t > 1.0 - dt)        { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
        return 0.0;
    }

    double sampleRate = 44100.0;
    double freq = 110.0;
    double phase = 0.0;
    Wave   wave = Wave::Saw;
};

//==============================================================================
// Decaying envelope. Fast (near-instant) attack to 1.0 then exponential decay.
// Used for both the filter sweep (decays fully) and the amplitude (with a
// gate hold so the note rings while a key is down).
//==============================================================================
class Envelope
{
public:
    void setSampleRate (double sr) noexcept { sampleRate = sr; recalc(); }

    // Filter env: typically does not sustain (decays to 0 while held).
    // Amp env: sustains at 1.0 until note-off, then releases quickly.
    void setTimes (double attackMs, double decayMs, bool sustainWhileGated) noexcept
    {
        attackSamples = std::max (1.0, attackMs * 0.001 * sampleRate);
        decayMs_ = decayMs;
        sustain = sustainWhileGated;
        recalc();
    }

    void noteOn  (bool retrigger) noexcept
    {
        gate = true;
        if (retrigger) { stage = Stage::Attack; }   // slide/legato skips retrigger
    }
    void noteOff() noexcept { gate = false; }

    inline float process() noexcept
    {
        switch (stage)
        {
            case Stage::Idle:
                value = 0.0;
                break;

            case Stage::Attack:
                value += 1.0 / attackSamples;
                if (value >= 1.0) { value = 1.0; stage = Stage::Decay; }
                break;

            case Stage::Decay:
                if (sustain && gate)
                    value = 1.0;                       // hold while key down
                else
                    value *= decayCoef;                // exponential fall to 0
                if (value < 1.0e-4 && !(sustain && gate)) { value = 0.0; stage = Stage::Idle; }
                break;
        }

        // Amp env releases when the gate drops even if "sustain" was set.
        if (sustain && !gate && stage == Stage::Decay)
            value *= releaseCoef;

        return (float) value;
    }

    bool isActive() const noexcept { return stage != Stage::Idle || (sustain && gate); }

private:
    void recalc() noexcept
    {
        decayCoef   = std::exp (-1.0 / std::max (1.0, decayMs_ * 0.001 * sampleRate));
        releaseCoef = std::exp (-1.0 / std::max (1.0, 8.0     * 0.001 * sampleRate));
    }

    enum class Stage { Idle, Attack, Decay };
    Stage  stage = Stage::Idle;
    double sampleRate = 44100.0;
    double value = 0.0;
    double attackSamples = 4.0;
    double decayMs_ = 300.0;
    double decayCoef = 0.999, releaseCoef = 0.99;
    bool   sustain = false, gate = false;
};

//==============================================================================
// 4-pole resonant low-pass — the famous 303 "diode ladder" squelch.
//
// Topology-Preserving-Transform (zero-delay-feedback) ladder filter after
// Vadim Zavalishin, "The Art of VA Filter Design". Four TPT one-pole stages
// with a resonance feedback path, solved with the correct instantaneous
// (implicit) feedback so it stays stable even at high resonance and cutoff.
//==============================================================================
class LadderFilter
{
public:
    void setSampleRate (double sr) noexcept { sampleRate = sr; }
    void reset() noexcept { s1 = s2 = s3 = s4 = 0.0; }

    // cutoff in Hz, resonance 0..1 (≈1 reaches self-oscillation)
    void setParams (double cutoffHz, double resonance) noexcept
    {
        cutoffHz = std::clamp (cutoffHz, 20.0, sampleRate * 0.49);
        const double g = std::tan (kPi * cutoffHz / sampleRate);   // prewarped
        G = g / (1.0 + g);                                          // TPT one-pole coeff
        // Feedback must stay below 4 (the self-oscillation limit) for stability.
        // 3.95 gives a very high, squelchy resonance without diverging.
        k = std::clamp (resonance, 0.0, 1.0) * 3.95;
    }

    inline float process (float input, float drive) noexcept
    {
        const double x  = std::tanh (input * (1.0 + drive));        // input drive = growl
        const double a  = G;
        const double a2 = a * a, a3 = a2 * a, a4 = a3 * a;

        // Solve the zero-delay feedback: out4 = (a^4 x + B) / (1 + k a^4)
        const double B    = a3 * (1.0 - a) * s1
                          + a2 * (1.0 - a) * s2
                          + a  * (1.0 - a) * s3
                          +      (1.0 - a) * s4;
        const double out4 = (a4 * x + B) / (1.0 + k * a4);
        const double u    = x - k * out4;                           // filter input w/ feedback

        // Run the four stages forward with that input and update the states.
        const double v1 = a * (u  - s1); const double o1 = v1 + s1; s1 = o1 + v1;
        const double v2 = a * (o1 - s2); const double o2 = v2 + s2; s2 = o2 + v2;
        const double v3 = a * (o2 - s3); const double o3 = v3 + s3; s3 = o3 + v3;
        const double v4 = a * (o3 - s4); const double o4 = v4 + s4; s4 = o4 + v4;

        return (float) o4;
    }

private:
    double sampleRate = 44100.0;
    double G = 0.0, k = 0.0;
    double s1 = 0.0, s2 = 0.0, s3 = 0.0, s4 = 0.0;
};

//==============================================================================
// The complete monophonic 303 voice + note handling (note stack, accent, slide)
//==============================================================================
class TB303Engine
{
public:
    struct Params
    {
        Oscillator::Wave wave = Oscillator::Wave::Saw;
        float tuningSemis = 0.0f;   // -12..+12
        float cutoffHz    = 500.0f; // base cutoff
        float resonance   = 0.7f;   // 0..1
        float envMod      = 0.6f;   // 0..1  filter env depth
        float decayMs     = 300.0f; // filter env decay
        float accent      = 0.6f;   // 0..1  accent intensity
        float volume      = 0.8f;   // linear gain
    };

    void prepare (double sr)
    {
        sampleRate = sr;
        osc.setSampleRate (sr);
        filter.setSampleRate (sr);
        ampEnv.setSampleRate (sr);
        filtEnv.setSampleRate (sr);
        filter.reset();
        osc.reset();
        heldNotes.clear();
        glideFreq = targetFreq = 110.0;
        ampEnv.setTimes (1.0, 0.0, /*sustain*/ true);
    }

    void setParams (const Params& p) { params = p; osc.setWave (p.wave); }

    void noteOn (int midiNote, float velocity)
    {
        const bool wasSilent = heldNotes.empty();
        heldNotes.push_back (midiNote);

        const bool accented = velocity >= 0.62f;          // high velocity = accent
        const bool slide    = ! wasSilent;                // legato note = slide

        startNote (midiNote, accented, slide);
    }

    void noteOff (int midiNote)
    {
        auto it = std::find (heldNotes.rbegin(), heldNotes.rend(), midiNote);
        if (it != heldNotes.rend())
            heldNotes.erase (std::next (it).base());

        if (heldNotes.empty())
        {
            ampEnv.noteOff();
        }
        else
        {
            // fall back to the most recent still-held note (legato slide, no retrigger)
            startNote (heldNotes.back(), currentAccent, /*slide*/ true);
        }
    }

    void allNotesOff()
    {
        heldNotes.clear();
        ampEnv.noteOff();
    }

    inline float renderSample() noexcept
    {
        // --- slide / portamento -------------------------------------------------
        glideFreq += (targetFreq - glideFreq) * glideCoef;
        osc.setFrequency (glideFreq);

        // --- envelopes ----------------------------------------------------------
        const float a = ampEnv.process();
        const float fe = filtEnv.process();

        // Accent boosts brightness, env depth and loudness, like the 303 accent bus
        const float accAmt   = currentAccent ? params.accent : 0.0f;
        const float envDepth = params.envMod * (1.0f + 1.6f * accAmt);
        const float reso     = std::min (1.0f, params.resonance + 0.25f * accAmt);
        const float drive    = 0.3f + 1.2f * accAmt;

        // filter env (exponential in octaves) sweeps the cutoff
        const double sweep  = std::pow (2.0, (double) (envDepth * fe) * 6.5);
        const double cutoff = params.cutoffHz * sweep;
        filter.setParams (cutoff, reso);

        // --- oscillator -> filter -> VCA ---------------------------------------
        float s = osc.process();
        s = filter.process (s, drive);
        s *= a * (1.0f + 0.7f * accAmt) * params.volume;
        return s;
    }

    bool isActive() const noexcept { return ampEnv.isActive(); }

private:
    void startNote (int midiNote, bool accented, bool slide)
    {
        currentAccent = accented;
        targetFreq = 440.0 * std::pow (2.0, (midiNote - 69 + (double) params.tuningSemis) / 12.0);
        if (! slide)
            glideFreq = targetFreq;                       // hard pitch jump

        // slide time ≈ 60 ms (the fixed 303 slide); fast when not sliding
        const double slideMs = slide ? 60.0 : 1.0;
        glideCoef = 1.0 - std::exp (-1.0 / (slideMs * 0.001 * sampleRate));

        // On a slide (legato) the 303 does NOT retrigger its envelopes.
        ampEnv.setTimes (1.0, 0.0, true);
        filtEnv.setTimes (3.0, params.decayMs, /*sustain*/ false);
        ampEnv.noteOn  (! slide);
        filtEnv.noteOn (! slide);
    }

    Params       params;
    Oscillator   osc;
    LadderFilter filter;
    Envelope     ampEnv, filtEnv;

    double sampleRate = 44100.0;
    double targetFreq = 110.0, glideFreq = 110.0, glideCoef = 1.0;
    bool   currentAccent = false;

    std::vector<int> heldNotes;
};

} // namespace acid

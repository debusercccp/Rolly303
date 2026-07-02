#pragma once

#include <cmath>
#include <vector>
#include <algorithm>

//==============================================================================
//  Rolly303 — a faithful emulation of the Roland TB-303 mono voice.
//
//  Signal path (matching the original circuit topology):
//
//      MIDI ─▶ Slide(60ms glide) ─▶ VCO(saw/square) ─▶ HP 44Hz ─▶ diode ladder
//              ─▶ VCA ─▶ HP 24Hz ─▶ out
//
//  Hardware behaviours reproduced here:
//
//   * Diode-ladder LPF with a ~150 Hz high-pass in the resonance feedback
//     path — the reason a real 303's resonance thins the bass instead of
//     booming on low notes.
//   * Main Envelope Generator (MEG): fixed ~3 ms attack, exponential decay
//     set by the DECAY knob (200 ms – 2 s on the hardware).
//   * VCA envelope: fixed ~3 ms attack and a slow ~3.5 s decay that keeps
//     falling even while the gate is held (the 303 has no sustain), with a
//     fast cutoff when the gate ends.
//   * ACCENT: forces the MEG to a fast fixed decay (~200 ms), adds the MEG
//     to the VCA level (punch), and drives the filter through the accent
//     "sweep" capacitor. The capacitor charges through the RESONANCE pot, so
//     at high resonance repeated accents stack up charge — the famous
//     building "wow wow" sweep.
//   * SLIDE: ~60 ms exponential portamento, envelopes not retriggered.
//
//  This is a real-time DSP model, not a SPICE netlist, but every musically
//  audible mechanism of the original instrument is modelled.
//==============================================================================

namespace acid
{

constexpr double kPi = 3.14159265358979323846;

//==============================================================================
// TPT (zero-delay) one-pole, usable as low-pass or high-pass.
//==============================================================================
struct OnePoleTPT
{
    void setSampleRate (double sr) noexcept { fs = sr; update(); }
    void setCutoff (double hz)     noexcept { fc = hz; update(); }
    void reset()                   noexcept { s = 0.0; }

    inline double lp (double x) noexcept
    {
        const double v = G * (x - s);
        const double y = v + s;
        s = y + v;
        return y;
    }
    inline double hp (double x) noexcept { return x - lp (x); }

    double G = 0.0, s = 0.0;

private:
    void update() noexcept { const double g = std::tan (kPi * fc / fs); G = g / (1.0 + g); }
    double fs = 44100.0, fc = 100.0;
};

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
// 303-style envelope: near-instant linear attack, then exponential decay.
//
//  * Filter env (MEG): ignores the gate — it decays away even while the key
//    is held, which is what makes every 303 note a pluck.
//  * Amp env: decays slowly (~3.5 s) while gated — the 303 has no sustain
//    level — and switches to a fast release the moment the gate drops.
//==============================================================================
class Envelope
{
public:
    void setSampleRate (double sr) noexcept { sampleRate = sr; recalc(); }

    // gateSensitive = amp-env behaviour (fast release on gate-off).
    void configure (double attackMs, double decayMs, double releaseMs, bool gateSensitive_) noexcept
    {
        attackSamples = std::max (1.0, attackMs * 0.001 * sampleRate);
        decayMs_      = decayMs;
        releaseMs_    = releaseMs;
        gateSensitive = gateSensitive_;
        recalc();
    }

    void setDecay (double decayMs) noexcept { decayMs_ = decayMs; recalc(); }

    void noteOn (bool retrigger) noexcept
    {
        gate = true;
        if (retrigger) stage = Stage::Attack;   // slide/legato skips retrigger
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
                value *= (gateSensitive && ! gate) ? releaseCoef : decayCoef;
                if (value < 1.0e-4) { value = 0.0; stage = Stage::Idle; }
                break;
        }
        return (float) value;
    }

    float current() const noexcept { return (float) value; }
    bool isActive() const noexcept { return stage != Stage::Idle; }

private:
    void recalc() noexcept
    {
        decayCoef   = std::exp (-1.0 / std::max (1.0, decayMs_   * 0.001 * sampleRate));
        releaseCoef = std::exp (-1.0 / std::max (1.0, releaseMs_ * 0.001 * sampleRate));
    }

    enum class Stage { Idle, Attack, Decay };
    Stage  stage = Stage::Idle;
    double sampleRate = 44100.0;
    double value = 0.0;
    double attackSamples = 4.0;
    double decayMs_ = 300.0, releaseMs_ = 8.0;
    double decayCoef = 0.999, releaseCoef = 0.99;
    bool   gateSensitive = false, gate = false;
};

//==============================================================================
// The 303 diode-ladder low-pass.
//
// Four TPT one-pole stages solved with zero-delay (implicit) feedback, after
// Vadim Zavalishin, "The Art of VA Filter Design" — plus the 303's defining
// twist: a ~150 Hz high-pass filter in the resonance feedback path. The
// high-pass removes low frequencies from the feedback, so resonance squelches
// the mids/highs but never booms the bass, exactly like the hardware.
//
// Because the feedback high-pass is itself a TPT one-pole, its contribution
// splits into a term proportional to the current output (folded into the
// implicit solve) plus a known state term — so the loop is still solved
// exactly. The resolved loop input is soft-clipped, which both emulates the
// diode nonlinearity and keeps self-oscillation bounded.
//==============================================================================
class DiodeLadder
{
public:
    void setSampleRate (double sr) noexcept
    {
        sampleRate = sr;
        fbHp.setSampleRate (sr);
        fbHp.setCutoff (150.0);          // resonance feedback high-pass (hardware trait)
    }

    void reset() noexcept { s1 = s2 = s3 = s4 = 0.0; fbHp.reset(); }

    // cutoff in Hz, resonance 0..1 (1 reaches self-oscillation)
    void setParams (double cutoffHz, double resonance) noexcept
    {
        cutoffHz = std::clamp (cutoffHz, 20.0, sampleRate * 0.49);
        const double g = std::tan (kPi * cutoffHz / sampleRate);   // prewarped
        G = g / (1.0 + g);                                          // TPT one-pole coeff
        // The feedback high-pass steals loop gain, so k may go slightly past
        // the linear self-oscillation limit of 4; the loop soft-clip bounds it.
        k = std::clamp (resonance, 0.0, 1.0) * 4.2;
    }

    inline float process (float input) noexcept
    {
        const double x  = std::tanh ((double) input);               // diode input stage
        const double a  = G;
        const double a2 = a * a, a3 = a2 * a, a4 = a3 * a;

        // Feedback = k * HP(out4). Within this sample, HP(out4) = (1-Gfb)*(out4 - sFb),
        // so the loop sees an effective gain keff plus a known offset from the HP state.
        const double keff = k * (1.0 - fbHp.G);
        const double xin  = x + keff * fbHp.s;

        // Solve the zero-delay feedback: out4 = (a^4 xin + B) / (1 + keff a^4)
        const double B    = a3 * (1.0 - a) * s1
                          + a2 * (1.0 - a) * s2
                          + a  * (1.0 - a) * s3
                          +      (1.0 - a) * s4;
        const double out4 = (a4 * xin + B) / (1.0 + keff * a4);

        // Resolved ladder input, soft-clipped (diode limiting / bounded self-osc).
        const double u = std::tanh (xin - keff * out4);

        // Run the four stages forward with that input and update the states.
        const double v1 = a * (u  - s1); const double o1 = v1 + s1; s1 = o1 + v1;
        const double v2 = a * (o1 - s2); const double o2 = v2 + s2; s2 = o2 + v2;
        const double v3 = a * (o2 - s3); const double o3 = v3 + s3; s3 = o3 + v3;
        const double v4 = a * (o3 - s4); const double o4 = v4 + s4; s4 = o4 + v4;

        fbHp.lp (o4);                                               // advance the feedback HP
        return (float) o4;
    }

private:
    double sampleRate = 44100.0;
    double G = 0.0, k = 0.0;
    double s1 = 0.0, s2 = 0.0, s3 = 0.0, s4 = 0.0;
    OnePoleTPT fbHp;
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
        float cutoffHz    = 750.0f; // base cutoff (hardware: ~250..2400 Hz)
        float resonance   = 0.7f;   // 0..1
        float envMod      = 0.6f;   // 0..1  filter env depth
        float decayMs     = 400.0f; // MEG decay (hardware: 200..2000 ms)
        float accent      = 0.6f;   // 0..1  accent intensity
        float drive       = 0.0f;   // 0..1  overdrive amount (TB-03-style extra)
        float volume      = 0.8f;   // linear gain
    };

    // Hardware timing constants
    static constexpr double kAccentDecayMs = 200.0;   // accent forces fast MEG decay
    static constexpr double kAmpDecayMs    = 3500.0;  // VCA env decay (no sustain!)
    static constexpr double kAmpReleaseMs  = 8.0;     // VCA gate-off
    static constexpr double kSlideMs       = 60.0;    // fixed slide time constant

    void prepare (double sr)
    {
        sampleRate = sr;
        osc.setSampleRate (sr);
        filter.setSampleRate (sr);
        ampEnv.setSampleRate (sr);
        filtEnv.setSampleRate (sr);
        preHp.setSampleRate (sr);  preHp.setCutoff (44.5);   // pre-filter coupling cap
        postHp.setSampleRate (sr); postHp.setCutoff (24.0);  // output coupling cap
        filter.reset();
        preHp.reset();
        postHp.reset();
        osc.reset();
        heldNotes.clear();
        glideFreq = targetFreq = 110.0;
        accentCap = 0.0;
        ampEnv.configure (3.0, kAmpDecayMs, kAmpReleaseMs, /*gateSensitive*/ true);
        updateAccentCapCoefs();
    }

    void setParams (const Params& p)
    {
        params = p;
        osc.setWave (p.wave);
        updateAccentCapCoefs();
    }

    void noteOn (int midiNote, float velocity)
    {
        const bool wasSilent = heldNotes.empty();

        // A note-on for a pitch already in the stack (e.g. a slide onto the
        // same note) must not create a duplicate entry: its single note-off
        // would leave a stuck copy behind, and from then on the stack never
        // empties — turning every following note into a slide.
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), midiNote),
                         heldNotes.end());
        heldNotes.push_back (midiNote);

        const bool accented = velocity >= 0.62f;          // high velocity = accent
        const bool slide    = ! wasSilent;                // legato note = slide

        startNote (midiNote, accented, slide);
    }

    void noteOff (int midiNote)
    {
        auto it = std::find (heldNotes.rbegin(), heldNotes.rend(), midiNote);
        if (it == heldNotes.rend())
            return;                       // stray note-off: nothing to release
        heldNotes.erase (std::next (it).base());

        if (heldNotes.empty())
        {
            ampEnv.noteOff();
            filtEnv.noteOff();
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
        filtEnv.noteOff();
    }

    inline float renderSample() noexcept
    {
        // --- slide / portamento -------------------------------------------------
        glideFreq += (targetFreq - glideFreq) * glideCoef;
        osc.setFrequency (glideFreq);

        // --- envelopes ----------------------------------------------------------
        const float a  = ampEnv.process();
        const float fe = filtEnv.process();

        // --- accent sweep capacitor ----------------------------------------------
        // On accented notes the MEG is routed (scaled by the ACCENT knob) into an
        // R/C network whose charge path runs through the RESONANCE pot. Low reso:
        // the cap charges fast, accent is just a brighter pluck. High reso: the
        // cap smooths the sweep and keeps charge between closely-spaced accents,
        // so the sweep builds over consecutive notes — the "wow wow" effect.
        const float  accAmt = currentAccent ? params.accent : 0.0f;
        const double target = (double) (accAmt * fe);
        accentCap += (target - accentCap) * (target > accentCap ? capChargeCoef
                                                                : capDischargeCoef);

        // --- cutoff: MEG sweep (in octaves) + accent sweep ------------------------
        const double envOct = (double) params.envMod * fe * 4.3;
        const double accOct = accentCap * 3.0;
        const double cutoff = params.cutoffHz * std::pow (2.0, envOct + accOct);
        filter.setParams (cutoff, params.resonance);

        // --- oscillator -> HP -> diode ladder -> VCA -> HP ------------------------
        float s = osc.process();
        s = (float) preHp.hp (s);
        s = filter.process (s);

        // Accent adds the (fast-decay) MEG to the VCA level: the accent "punch".
        const float vca = a * (1.0f + 1.3f * accAmt * fe);
        s = (float) postHp.hp (s * vca);

        // Overdrive (a TB-03 / plug-in era extra, not on the 1982 hardware):
        // a tanh waveshaper blended in by the DRIVE knob, gain-compensated so
        // full drive saturates rather than just getting louder.
        if (params.drive > 0.001f)
        {
            const float pre = 1.0f + 9.0f * params.drive;
            const float wet = std::tanh (s * pre) * (0.9f / std::tanh (pre * 0.9f));
            s += params.drive * (wet - s);
        }

        return s * params.volume;
    }

    bool isActive() const noexcept { return ampEnv.isActive(); }

private:
    void startNote (int midiNote, bool accented, bool slide)
    {
        currentAccent = accented;
        targetFreq = 440.0 * std::pow (2.0, (midiNote - 69 + (double) params.tuningSemis) / 12.0);
        if (! slide)
            glideFreq = targetFreq;                       // hard pitch jump

        const double slideMs = slide ? kSlideMs : 1.0;
        glideCoef = 1.0 - std::exp (-1.0 / (slideMs * 0.001 * sampleRate));

        // Accent overrides the DECAY knob with a fast fixed decay (hardware trait).
        filtEnv.configure (3.0, accented ? kAccentDecayMs : (double) params.decayMs,
                           kAccentDecayMs, /*gateSensitive*/ false);

        // On a slide (legato) the 303 does NOT retrigger its envelopes.
        ampEnv.noteOn  (! slide);
        filtEnv.noteOn (! slide);
    }

    void updateAccentCapCoefs() noexcept
    {
        // Charge time constant grows with the resonance pot (1 ms .. ~100 ms);
        // discharge is slow (~250 ms) so charge survives between fast accents.
        const double r = (double) params.resonance;
        const double tauCharge = 0.001 + 0.1 * r * r;
        capChargeCoef    = 1.0 - std::exp (-1.0 / (tauCharge * sampleRate));
        capDischargeCoef = 1.0 - std::exp (-1.0 / (0.25 * sampleRate));
    }

    Params      params;
    Oscillator  osc;
    DiodeLadder filter;
    Envelope    ampEnv, filtEnv;
    OnePoleTPT  preHp, postHp;

    double sampleRate = 44100.0;
    double targetFreq = 110.0, glideFreq = 110.0, glideCoef = 1.0;
    double accentCap = 0.0, capChargeCoef = 1.0, capDischargeCoef = 0.01;
    bool   currentAccent = false;

    std::vector<int> heldNotes;
};

} // namespace acid

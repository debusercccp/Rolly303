# Rolly303

A standalone synthesizer and **VST3 plugin** that emulates the **Roland TB‑303 Bass Line** — the squelchy, resonant mono‑synth that defined acid house and techno. Builds from a single codebase to a **Standalone app** and a **VST3 plugin** on both **Windows** and **Linux**.

> This is a real‑time DSP *emulation* of the 303 voice (oscillator → diode‑ladder filter → decay envelope, plus accent & slide). It is not affiliated with or endorsed by Roland; "TB‑303" and "Roland" are trademarks of Roland Corporation and are used here only to describe the sound being emulated.

---

## Features

- **One oscillator** — band‑limited **Sawtooth** or **Square**, exactly like the 303.
- **Diode‑ladder low‑pass filter** with a high‑pass in the resonance feedback path — the hardware trait that makes 303 resonance squelch the mids instead of booming the bass.
- **Decay envelope → filter sweep** (`Env Mod` + `Decay`): the core acid pluck on every note. Knob ranges match the hardware (cutoff ≈ 250 Hz–2.4 kHz, decay 200 ms–2 s).
- **No‑sustain VCA envelope** — fixed ~3 ms attack and a slow ~3.5 s decay that keeps falling while a key is held, just like the original.
- **Accent** — high‑velocity notes force a fast fixed envelope decay (~200 ms), add punch to the VCA, and drive the filter through the accent **sweep capacitor**: its charging runs through the resonance pot, so at high resonance repeated accents build up the famous "wow‑wow" sweep.
- **Slide** — overlapping (legato) notes glide in pitch over the 303's fixed ~60 ms, with the envelope *not* retriggering, just like the original.
- **Built‑in 16‑step sequencer** with **per‑step pitch, gate, accent and slide** — the other half of what makes a 303 a 303.
- **Tuning** (±12 semitones), **Cutoff**, **Resonance**, **Env Mod**, **Decay**, **Accent**, **Volume**.
- **On‑screen keyboard** in the standalone app, plus full MIDI input in any DAW.
- Cross‑platform: **Windows** and **Linux**, **Standalone + VST3**.

### How to get the "acid" sound
Turn **Resonance** up high, set **Cutoff** fairly low, raise **Env Mod**, and play a fast 16th‑note pattern. Use note **velocity ≥ ~80** (out of 127) to trigger **accents**, and **overlap two notes** (legato) to trigger a **slide**.

---

## Controls

| Control     | Range          | What it does                                                        |
|-------------|----------------|---------------------------------------------------------------------|
| Waveform    | Saw / Square   | Oscillator shape (slide switch, like the panel).                    |
| Tuning      | −12…+12 st     | Master tuning.                                                       |
| Cut Off Freq| 250…2400 Hz    | Base filter cutoff (hardware pot range).                            |
| Resonance   | 0…1            | Filter resonance / squelch. Also slows the accent sweep cap.        |
| Env Mod     | 0…1            | How much the decay envelope opens the filter.                       |
| Decay       | 200…2000 ms    | Filter‑envelope decay (hardware pot range). Accents override it.    |
| Accent      | 0…1            | Intensity of accented notes (velocity ≥ ~80).                       |
| Volume      | 0…1            | Output level.                                                        |

---

## The 16‑step sequencer

The bottom panel is a classic 303‑style step sequencer. It drives the synth
itself, so it works in the **standalone app and in any DAW**.

### Transport (the row above the grid)

| Control | What it does |
|--------|--------------|
| **RUN**    | Start / stop the pattern. When off, the synth plays from MIDI / the on‑screen keyboard instead. |
| **SYNC**   | Lock tempo **and bar position** to your DAW's transport. Leave off to use the internal Tempo knob (and for the standalone app). |
| **TEMPO**  | Internal tempo in BPM (used when SYNC is off). |
| **ROOT**   | The root note the pattern is built from (C…B). |
| **OCTAVE** | Shift the whole pattern up/down by octaves. |

### Per‑step controls (the grid)

Each of the 16 columns is one 16th‑note step with four controls stacked top to bottom:

- **PITCH** (vertical slider) — semitone offset for that step, `0…24` (two octaves) above Root.
- **GATE** — note on/off. Turn it off to make a rest.
- **ACCENT** — gives that step extra volume, brightness and bite (the accent bus).
- **SLIDE** — glides *from this step into the next one* without re‑plucking the
  envelope, producing the signature 303 portamento.

The currently playing step is highlighted in orange while the sequencer runs.
The whole pattern (and every knob) is saved with your DAW project and restored on reload.

### Quick start
Press **RUN**. The synth ships with a default acid riff so you'll hear sound
immediately — then drag the PITCH sliders, toggle some ACCENT/SLIDE buttons,
and dial in **Cutoff / Resonance / Env Mod / Decay** to taste.

---

## Installation

There are **no pre‑built binaries** — the plugin and the standalone app must be
**compiled from source first** (see [Building from source](#building-from-source) below;
on Windows, `run-win.bat` builds *and* installs everything for you automatically).

After a successful build, the files to install are found under
`build/Rolly303_artefacts/Release/`:

```
build/Rolly303_artefacts/Release/
├── Standalone/
│   └── Rolly303        (or "Rolly303.exe" on Windows)
└── VST3/
    └── Rolly303.vst3   ← this is what goes in your VST3 folder
```

### Windows — step by step

You have two ways to use Rolly303 on Windows: as a **standalone app** (no DAW needed)
or as a **VST3 plugin** inside a DAW.

> **Easiest:** just double‑click **`run-win.bat`** — it compiles the project if needed
> and installs both the standalone app and the VST3 (including shortcuts and an
> uninstaller). The manual steps below are only needed if you prefer to do it yourself.

**A) Standalone app**
1. Build the project (see [Building from source](#building-from-source)).
2. Find **`Rolly303.exe`** in `build\Rolly303_artefacts\Release\Standalone\`.
3. Double‑click it to run. *(If Windows SmartScreen shows a blue "Windows protected your
   PC" box because the app is unsigned, click **More info → Run anyway**.)*
4. Click the **Options/⚙ (audio settings)** button and choose your sound output device,
   then play the on‑screen keyboard or a connected MIDI keyboard.

**B) VST3 plugin (use it in a DAW)**
1. Build the project (see [Building from source](#building-from-source)).
2. Copy **`Rolly303.vst3`** from `build\Rolly303_artefacts\Release\VST3\`
   into the standard Windows VST3 folder:
   ```
   C:\Program Files\Common Files\VST3\
   ```
   - Open File Explorer, paste that path into the address bar, press **Enter**.
   - If the `VST3` folder does not exist, create it.
   - Copying here needs administrator rights — click **Continue** if Windows asks.
3. Open your DAW (Ableton Live, FL Studio, Cubase, Reaper, Bitwig, Studio One, etc.)
   and **rescan / refresh** the plugin list:
   - *FL Studio:* Options → Manage plugins → **Find plugins**.
   - *Ableton Live:* Preferences → Plug‑Ins → turn on **Use VST3 System Folders**, then **Rescan**.
   - *Reaper:* Preferences → Plug‑ins → VST → **Re‑scan**.
4. Add **Rolly303** to a MIDI/instrument track and play it.

> **Requirements:** 64‑bit Windows 10 or 11. The standalone app needs no installer —
> it is a single `.exe`. To uninstall, just delete the files you copied.

### Linux

First build the project (see [Building from source](#building-from-source)), then:

- **VST3**: copy the `Rolly303.vst3` folder from
  `build/Rolly303_artefacts/Release/VST3/` to
  `~/.vst3/`   (create the folder if it does not exist):
  ```bash
  mkdir -p ~/.vst3
  cp -r "build/Rolly303_artefacts/Release/VST3/Rolly303.vst3" ~/.vst3/
  ```
- **Standalone**: run `build/Rolly303_artefacts/Release/Standalone/Rolly303`
  directly, or copy it anywhere you like (or just use `./run-linux.sh`).

Then rescan plugins in your DAW (Ableton, Bitwig, Reaper, FL Studio, Ardour, etc.).

---

## Building from source

The project is self‑contained: CMake downloads the JUCE framework automatically on first configure (an internet connection is required for that first build).

### 1. Prerequisites

**All platforms**
- [CMake](https://cmake.org/download/) ≥ 3.22
- A C++17 compiler

**Windows**
- Nothing to install by hand: `run-win.bat` finds CMake and the Microsoft C++
  build tools automatically, and installs them with `winget` if they are missing
  (one-time download on the first build).

**Linux** (Debian/Ubuntu names shown — install the equivalents on other distros)
```bash
sudo apt update
sudo apt install -y build-essential cmake git pkg-config \
    libasound2-dev libjack-jackd2-dev \
    libfreetype6-dev libfontconfig1-dev \
    libx11-dev libxext-dev libxinerama-dev libxrandr-dev \
    libxcursor-dev libxcomposite-dev libgl1-mesa-dev
```

### 2. Configure & build

```bash
# from the project root (the folder containing this README)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

The first configure step clones JUCE and may take a few minutes. Subsequent builds are fast.

> **Tip:** If you already have JUCE checked out locally, skip the download with:
> ```bash
> cmake -B build -DCMAKE_BUILD_TYPE=Release -DJUCE_SOURCE_DIR=/path/to/JUCE
> ```

### 3. Where the built files land

After a successful build, look under `build/Rolly303_artefacts/Release/`:

```
build/Rolly303_artefacts/Release/
├── Standalone/
│   └── Rolly303        (or "Rolly303.exe" on Windows)
└── VST3/
    └── Rolly303.vst3
```

Copy the VST3 to the plugin folders listed in **Installation** above, or run the
standalone directly.

---

## Running the standalone app

Just launch the `Rolly303` executable, or use the included scripts
from the project root after building from source:

- **Windows:** double‑click **`run-win.bat`** (or run it from a terminal). It is a
  real installer: it builds the Release binaries first if they are missing, asks for
  administrator rights, then installs the standalone app to
  `C:\Program Files\Rolly303`, the VST3 to `C:\Program Files\Common Files\VST3`,
  creates Start Menu and Desktop shortcuts, and registers an uninstaller in
  *Settings → Apps* ("Add or remove programs"). To remove everything, run
  **`uninstall-win.bat`** or uninstall from *Settings → Apps*.
- **Linux:** run **`./run-linux.sh`** — it launches the built standalone (building
  it first if missing), and handles pure‑Wayland desktops automatically (see below).

Once the app is open:

- The first time, open the app's **audio settings** (gear / "Options" button) to pick
  your output device and, on Linux, your ALSA or JACK device.
- Play with a connected **MIDI keyboard**, or click the **on‑screen keyboard** at the
  bottom of the window.

### Linux on a pure‑Wayland compositor (niri, river, Sway‑without‑Xwayland, …)

JUCE apps are **X11 apps**. On GNOME/KDE/XFCE this is invisible — Xwayland is already
running. But on a minimal Wayland compositor with **no X server running**, launching the
app prints:

```
Authorization required, but no authorization protocol specified
```

…and no window appears. That just means there is no X display to draw on. Two fixes:

**Easiest — use the included launcher.** It starts a temporary Xwayland (a single
window that hosts the synth) and cleans it up on exit:

```bash
./run-linux.sh
```

**Manual one‑liner** (equivalent):

```bash
WAYLAND_DISPLAY=wayland-1 Xwayland :12 -ac -decorate -geometry 960x660 &
DISPLAY=:12 "./build/Rolly303_artefacts/Release/Standalone/Rolly303"
```

> Needs the `xwayland` package: `sudo apt install xwayland`.

**Best, permanent fix —** install [`xwayland‑satellite`](https://github.com/Supreeeme/xwayland-satellite)
so X apps integrate seamlessly as native windows (recommended on niri):

```bash
cargo install xwayland-satellite       # or your distro's package
xwayland-satellite :0 &                 # add this to your compositor's startup
export DISPLAY=:0                        # export this in your shell profile
```

After that, every X11 app (this synth, and X11 plugin GUIs inside Linux DAWs) just works,
and you can run `Rolly303` directly with no wrapper.

---

## Project layout

```
rolly303/
├── CMakeLists.txt          # Build script (fetches JUCE, defines the plugin)
├── README.md               # This file
├── run-linux.sh            # Launches the built standalone on Linux (builds it if missing, handles pure-Wayland)
├── run-win.bat             # Windows installer: builds if needed, installs app + VST3, shortcuts, uninstaller
├── uninstall-win.bat       # Windows uninstaller (a copy is installed next to the app)
└── source/
    ├── TB303Engine.h       # The DSP: oscillator, ladder filter, envelopes, voice
    ├── PluginProcessor.h
    ├── PluginProcessor.cpp # Parameters, MIDI handling, audio rendering
    ├── PluginEditor.h
    └── PluginEditor.cpp    # The 303‑style GUI + on‑screen keyboard
```

---

## Troubleshooting

| Problem | Fix |
|--------|-----|
| First `cmake -B build` fails to download JUCE | Check your internet connection / proxy, or pass `-DJUCE_SOURCE_DIR=` pointing at a local JUCE checkout. |
| Linux build errors about missing headers | Re‑check the `apt install` list above; ALSA/X11/freetype `-dev` packages are required. |
| DAW doesn't see the plugin | Confirm the `.vst3` is in the correct folder and trigger a plugin **rescan**. VST3 only (no VST2). |
| `Authorization required, but no authorization protocol specified` and no window (Linux) | You're on a pure‑Wayland compositor with no X server. Use `./run-linux.sh`, or install `xwayland-satellite`. See *Linux on a pure‑Wayland compositor* above. |
| No sound in standalone | Open audio settings and select a valid output device / sample rate. |
| Crackle at very high resonance + cutoff | Expected near self‑oscillation; back off Resonance slightly. |

---

## License

You own this code. JUCE is downloaded at build time under its own license
(GPLv3 or a paid JUCE license — see <https://juce.com>); make sure your
distribution complies with JUCE's terms.

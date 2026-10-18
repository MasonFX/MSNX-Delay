# MSNX Delay

A recreation of the 3-channel delay built into the Akai MPC3000, as a VST3 / Audio Unit plugin for Windows and Mac.

The MPC3000's delay has three echo channels that all share one memory. Feedback from each channel goes back into that shared memory, so the echoes bleed into each other. That gives it a different feel from a typical multi-echo delay. This plugin copies that behavior, using the same controls and ranges as the original.

## Download

Go to the **[Releases](../../releases)** page and download the zip for your computer:

- `MSNX-Delay-Windows.zip`
- `MSNX-Delay-Mac.zip`

## Install

### Windows

1. Unzip the download.
2. Copy the **`MSNX Delay.vst3`** folder into `C:\Program Files\Common Files\VST3`.
3. Open your DAW and rescan plugins.

Nothing else to install. Updating from the older "MPC3K Delay"? Delete the old `MPC3K Delay.vst3` folder first so your DAW doesn't show two copies. If Windows shows "Windows protected your PC" when opening the standalone app, click **More info → Run anyway**.

### Mac (Apple Silicon and Intel, macOS 11+)

1. Unzip the download.
2. Double-click **`Install.command`**. If macOS won't open it, right-click it, choose **Open**, then click **Open** again.
3. Restart your DAW.

Logic and GarageBand use the Audio Unit version. Ableton, Bitwig, Reaper, FL Studio and most others can use the VST3.

The plugin isn't signed with a paid Apple developer account, so macOS blocks it by default. The installer tells macOS to trust it.

### Try it without a DAW

Both zips include a standalone app, `MSNX Delay.exe` on Windows and `MSNX Delay.app` on Mac. Open it, pick your audio device in its settings, and turn on an input.

## Controls

Each of the three channels has the four original MPC3000 controls:

| Control | Range | What it does |
| --- | --- | --- |
| Volume | 0 to 100 | How loud this echo is |
| Pan | 50L to 50R | Where this echo sits left to right |
| Time | 1 to 1486 ms | How long after the sound this echo plays |
| Feedback | 0 to 100% | How much of this echo is sent back in to repeat |

**Time Sync:** turn it on to set the channel's Time as a note length (1/32 up to 1/2, with dotted D and triplet T versions) that follows your DAW's tempo. Times longer than 1486 ms are held at 1486 ms, the original's limit.

**LFO (one per channel):** moves one of that channel's four controls up and down automatically.

| Control | What it does |
| --- | --- |
| Target | Which control the LFO moves: Volume, Pan, Time, Feedback, or Off |
| Shape | Sine, Triangle, Saw, Square, or Random (a new random value each cycle) |
| Rate | How long one LFO cycle takes, from 20 ms to 10 s |
| Rate Sync | Turn on to set the rate as a note length (1/32 up to 8 bars) instead. While your DAW plays, the LFO locks to its bars, so it lands the same way every time |
| Depth | How far the LFO pushes the control, as a % of that control's full range, in both directions |

Tips: a tiny Depth on **Time** (under 1%) gives a tape-style wobble; larger amounts give big pitch swoops. **Pan** with a slow synced Triangle makes an echo drift side to side.

Plus **Mix** (dry/wet). Leave it at 100% when using the plugin on a send or aux.

The screen at the top shows what every channel is doing right now, including tempo sync and LFO movement. Double-click any knob to reset it.

## Quick start

- Channel 1 starts on. Set its **Time** to about 375 ms and **Feedback** to about 30%.
- Turn up **Volume** on Channel 2 and Channel 3 to add echoes at other times.
- A channel with Volume at 0 still feeds back, just like the original. You won't hear that channel directly, but you'll hear its repeats through the other channels.

**Careful:** if the three Feedback knobs add up to more than 100%, the echoes keep growing forever. That matches the real MPC3000. An LFO on Feedback can also push the total over 100% at its peaks. Keep your speakers low when experimenting.

## Build from source

Requires CMake 3.22+ and a C++17 compiler (Visual Studio 2022 on Windows, Xcode on Mac). JUCE is downloaded automatically.

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release
```

The built plugins end up in `build/Mpc3kDelay_artefacts/Release/`.

## Status

This is an early version. The delay's behavior matches the MPC3000 manual. The original's sound character (its grit, hiss, and how the repeats age) still needs to be tuned against recordings of a real unit.

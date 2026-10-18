// Mpc3kDelay.h
// Clean core of the MPC3000 3-tap delay (no "character" yet).
// Plain C++ with no JUCE, so the same code can run in the plugin and on a Daisy Seed.
//
// How it works (from the MPC3000 manual, Effects Generator page):
//   - One shared delay memory, up to 1486 ms long.
//   - Three taps read from that memory, each at its own delay time.
//   - Each tap has Volume (0-100), Pan (50L-50R), Time (1-1486 ms), Feedback (0-100%).
//   - All three feedback amounts are added back into the SAME memory,
//     so feedback from one tap shows up in the others.
//   - Feedback ignores Volume: a tap at volume 0 still feeds back.
//   - If the three feedback amounts add up to more than 100%, it runs away.

#pragma once

#include <array>
#include <vector>

class Mpc3kDelay
{
public:
    static constexpr int   numTaps   = 3;
    static constexpr float maxTimeMs = 1486.0f;
    static constexpr float minTimeMs = 1.0f;

    // Call before processing, and again if the sample rate changes.
    void prepare (double sampleRate);

    // Clears the memory (silence), keeps the settings.
    void reset();

    // Settings use the same units as the MPC3000 screen:
    //   volume   0 to 100
    //   pan      -50 (full left) to +50 (full right), 0 = center
    //   timeMs   1 to 1486 (fractions allowed, for tempo sync and LFOs)
    //   feedback 0 to 100 (percent)
    //
    // Changes glide over a few milliseconds so moving settings (like an LFO)
    // don't click. The first setting after prepare() is applied instantly.
    void setTap (int index, float volume, float pan, float timeMs, float feedback);

    // Mono in (sum your stereo send to mono first), stereo out (wet only).
    // outL / outR are overwritten, not added to.
    void process (const float* in, float* outL, float* outR, int numSamples);

private:
    struct TapState
    {
        float gainL        = 0.0f;
        float gainR        = 0.0f;
        float delaySamples = 1.0f;
        float feedback     = 0.0f; // 0 to 1
    };

    struct Tap
    {
        TapState target;   // where the settings are heading
        TapState current;  // where they are right now (gliding toward target)
        bool snap = true;  // jump straight to the next setting instead of gliding
    };

    float readTap (float delaySamples) const;

    double sampleRate = 48000.0;
    float  glide      = 1.0f;     // how far to move toward the target each sample (0 to 1)
    std::vector<float> memory;    // the one shared delay memory
    int writePos = 0;
    std::array<Tap, numTaps> taps;
};

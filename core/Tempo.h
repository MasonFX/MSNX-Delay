// Tempo.h
// Note divisions for tempo sync, and the math to turn them into milliseconds.
// Plain C++ (no JUCE), so it can also run on a Daisy Seed later.

#pragma once

#include <array>

namespace tempo
{
    // beats = length in quarter notes. T = triplet, D = dotted.
    struct Division
    {
        const char* name;
        double beats;
    };

    // For the delay taps. Longer values would pass the 1486 ms limit at most tempos.
    inline constexpr std::array<Division, 12> delayDivisions {{
        { "1/32",  0.125 },
        { "1/16T", 1.0 / 6.0 },
        { "1/16",  0.25 },
        { "1/16D", 0.375 },
        { "1/8T",  1.0 / 3.0 },
        { "1/8",   0.5 },
        { "1/8D",  0.75 },
        { "1/4T",  2.0 / 3.0 },
        { "1/4",   1.0 },
        { "1/4D",  1.5 },
        { "1/2T",  4.0 / 3.0 },
        { "1/2",   2.0 },
    }};
    inline constexpr int defaultDelayDivision = 5; // 1/8

    // For the LFOs, which can also run slowly over several bars (bars assume 4/4).
    inline constexpr std::array<Division, 17> lfoDivisions {{
        { "1/32",   0.125 },
        { "1/16T",  1.0 / 6.0 },
        { "1/16",   0.25 },
        { "1/16D",  0.375 },
        { "1/8T",   1.0 / 3.0 },
        { "1/8",    0.5 },
        { "1/8D",   0.75 },
        { "1/4T",   2.0 / 3.0 },
        { "1/4",    1.0 },
        { "1/4D",   1.5 },
        { "1/2T",   4.0 / 3.0 },
        { "1/2",    2.0 },
        { "1/2D",   3.0 },
        { "1 bar",  4.0 },
        { "2 bars", 8.0 },
        { "4 bars", 16.0 },
        { "8 bars", 32.0 },
    }};
    inline constexpr int defaultLfoDivision = 13; // 1 bar

    // How long a number of beats lasts at a tempo, in milliseconds.
    inline double beatsToMs (double beats, double bpm)
    {
        return beats * 60000.0 / bpm;
    }
}

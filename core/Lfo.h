// Lfo.h
// A simple low-frequency oscillator: a slow wave used to move a knob automatically.
// Plain C++ (no JUCE), so it can also run on a Daisy Seed later.
//
// The LFO doesn't know about time itself. You move it forward with advance()
// (free-running), or set its position directly with setPhase() (locked to the
// DAW's bar position). Its output is always between -1 and +1.

#pragma once

#include <cmath>
#include <cstdint>

class Lfo
{
public:
    enum class Shape { Sine = 0, Triangle, Saw, Square, Random };

    // Give each LFO a different seed so their Random shapes don't move together.
    void seed (uint32_t s)
    {
        state = s * 2654435761u + 1u;
        held  = nextRandom();
    }

    void setShape (Shape s) { shape = s; }

    // Move forward by a number of cycles (1.0 = one full wave).
    void advance (double cycles)
    {
        double p = phase + cycles;
        if (p >= 1.0)
        {
            p -= std::floor (p);
            held = nextRandom(); // Random shape picks a new value each cycle
        }
        phase = p;
    }

    // Jump to a position in the wave (0 to 1). Used to lock to the DAW's bars.
    void setPhase (double p)
    {
        p -= std::floor (p);
        if (p < phase)
            held = nextRandom(); // wrapped around to a new cycle
        phase = p;
    }

    double getPhase() const { return phase; }

    // Current output, -1 to +1.
    float value() const
    {
        switch (shape)
        {
            case Shape::Sine:     return (float) std::sin (6.283185307179586 * phase);
            case Shape::Triangle: return (float) (phase < 0.5 ? 4.0 * phase - 1.0 : 3.0 - 4.0 * phase);
            case Shape::Saw:      return (float) (2.0 * phase - 1.0);
            case Shape::Square:   return phase < 0.5 ? 1.0f : -1.0f;
            case Shape::Random:   return held;
        }
        return 0.0f;
    }

private:
    // Small, fast random number generator (good enough for wobbling a knob).
    float nextRandom()
    {
        state = state * 1664525u + 1013904223u;
        return (float) ((state >> 8) * (1.0 / 16777216.0)) * 2.0f - 1.0f;
    }

    Shape    shape = Shape::Sine;
    double   phase = 0.0;
    float    held  = 0.0f;
    uint32_t state = 0x12345678u;
};

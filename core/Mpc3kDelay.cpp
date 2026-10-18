// Mpc3kDelay.cpp

#include "Mpc3kDelay.h"

#include <algorithm>
#include <cmath>

namespace
{
    // How quickly settings glide to a new value. Short enough that LFOs and
    // knob turns feel immediate, long enough to avoid clicks.
    constexpr double glideSeconds = 0.003;
}

void Mpc3kDelay::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    // Room for the longest delay plus a couple of samples for in-between reads.
    const auto size = (int) std::ceil (maxTimeMs * 0.001 * sampleRate) + 2;
    memory.assign ((size_t) size, 0.0f);
    writePos = 0;

    glide = (float) (1.0 - std::exp (-1.0 / (glideSeconds * sampleRate)));

    for (auto& t : taps)
        t.snap = true;
}

void Mpc3kDelay::reset()
{
    std::fill (memory.begin(), memory.end(), 0.0f);
    writePos = 0;

    for (auto& t : taps)
        t.current = t.target;
}

void Mpc3kDelay::setTap (int index, float volume, float pan, float timeMs, float feedback)
{
    if (index < 0 || index >= numTaps)
        return;

    // Never let an invalid number (NaN / infinity) into the delay.
    if (! std::isfinite (volume))   volume   = 0.0f;
    if (! std::isfinite (pan))      pan      = 0.0f;
    if (! std::isfinite (timeMs))   timeMs   = minTimeMs;
    if (! std::isfinite (feedback)) feedback = 0.0f;

    auto& t = taps[(size_t) index];
    auto& s = t.target;

    const float gain = std::clamp (volume, 0.0f, 100.0f) / 100.0f;

    // Pan: -50..+50 mapped to 0..1, equal-power law.
    // NOTE: the MPC3000's real pan law is unknown. Check against reference recordings.
    const float p     = (std::clamp (pan, -50.0f, 50.0f) + 50.0f) / 100.0f;
    const float angle = p * 1.5707963f; // 0 to pi/2
    s.gainL = gain * std::cos (angle);
    s.gainR = gain * std::sin (angle);

    // The MPC3000's Time knob moves in whole milliseconds (the plugin's knob does too).
    // Fractions are allowed here so tempo sync stays locked and LFOs move smoothly.
    const float ms = std::clamp (timeMs, minTimeMs, maxTimeMs);
    s.delaySamples = (float) (ms * 0.001 * sampleRate);

    s.feedback = std::clamp (feedback, 0.0f, 100.0f) / 100.0f;

    if (t.snap)
    {
        t.current = t.target;
        t.snap = false;
    }
}

float Mpc3kDelay::readTap (float delaySamples) const
{
    // Read the memory "delaySamples" behind the write point.
    // Linear blend between the two nearest samples, since the delay
    // in samples is rarely a whole number.
    const int size = (int) memory.size();
    float readPos = (float) writePos - delaySamples;
    if (readPos < 0.0f)
        readPos += (float) size;

    const int   i0   = std::clamp ((int) readPos, 0, size - 1);
    const int   i1   = (i0 + 1) % size;
    const float frac = readPos - (float) i0;

    return memory[(size_t) i0] + frac * (memory[(size_t) i1] - memory[(size_t) i0]);
}

void Mpc3kDelay::process (const float* in, float* outL, float* outR, int numSamples)
{
    if (memory.empty())
    {
        std::fill (outL, outL + numSamples, 0.0f);
        std::fill (outR, outR + numSamples, 0.0f);
        return;
    }

    const int size = (int) memory.size();

    for (int n = 0; n < numSamples; ++n)
    {
        float left = 0.0f, right = 0.0f, feedbackSum = 0.0f;

        for (auto& t : taps)
        {
            // Glide each setting a little closer to its target.
            auto& c = t.current;
            const auto& g = t.target;
            c.gainL        += glide * (g.gainL        - c.gainL);
            c.gainR        += glide * (g.gainR        - c.gainR);
            c.delaySamples += glide * (g.delaySamples - c.delaySamples);
            c.feedback     += glide * (g.feedback     - c.feedback);

            const float tap = readTap (c.delaySamples);

            // To the stereo mix, through volume and pan.
            left  += tap * c.gainL;
            right += tap * c.gainR;

            // Back into the shared memory, through feedback only (ignores volume).
            feedbackSum += tap * c.feedback;
        }

        float toMemory = in[n] + feedbackSum;

        // Safety net only, so a runaway can't produce infinite values and crash a host.
        // How the real unit clips is unknown; that belongs in the character layer later.
        toMemory = std::clamp (toMemory, -8.0f, 8.0f);

        memory[(size_t) writePos] = toMemory;
        writePos = (writePos + 1) % size;

        outL[n] = left;
        outR[n] = right;
    }
}

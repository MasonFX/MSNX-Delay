// core_test.cpp
// Checks the clean delay core against what the MPC3000 manual says it should do.
// Build: g++ -std=c++17 -O2 -I core core/Mpc3kDelay.cpp tests/core_test.cpp -o core_test

#include "Mpc3kDelay.h"

#include <cmath>
#include <cstdio>
#include <vector>

static constexpr double sr = 48000.0;
static int failures = 0;

static void check (bool ok, const char* what)
{
    std::printf ("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (! ok) ++failures;
}

// Runs a single click (impulse) through the delay and returns the mono output (L + R).
static std::vector<float> runClick (Mpc3kDelay& d, int length)
{
    std::vector<float> in ((size_t) length, 0.0f), l ((size_t) length), r ((size_t) length), out ((size_t) length);
    in[0] = 1.0f;
    d.process (in.data(), l.data(), r.data(), length);
    for (int i = 0; i < length; ++i)
        out[(size_t) i] = l[(size_t) i] + r[(size_t) i];
    return out;
}

static int ms (double m) { return (int) std::lround (m * 0.001 * sr); }

int main()
{
    // 1. A tap lands exactly on time.
    {
        Mpc3kDelay d; d.prepare (sr);
        d.setTap (0, 100, -50, 500, 0); // hard left so L+R = tap level
        auto out = runClick (d, ms (1000));
        check (std::abs (out[(size_t) ms (500)] - 1.0f) < 1e-4f, "Tap 1 at 500 ms lands at 500 ms, full level");
        check (std::abs (out[(size_t) ms (500) - 1]) < 1e-6f, "Nothing before the tap time");
    }

    // 2. Feedback ignores volume: a silent tap still feeds back.
    {
        Mpc3kDelay d; d.prepare (sr);
        d.setTap (0, 100, -50, 100, 0);  // audible, no feedback
        d.setTap (1, 0,   -50, 300, 50); // silent, 50% feedback
        auto out = runClick (d, ms (1000));
        // Tap 2's feedback writes the click back in at 300 ms; tap 1 then plays it 100 ms later.
        check (std::abs (out[(size_t) ms (100)] - 1.0f) < 1e-4f, "Audible tap plays the click at 100 ms");
        check (std::abs (out[(size_t) ms (400)] - 0.5f) < 1e-4f, "Silent tap's feedback is heard at 400 ms, at half level");
    }

    // 3. Shared memory: one tap's feedback shows up in another tap.
    {
        Mpc3kDelay d; d.prepare (sr);
        d.setTap (0, 100, -50, 200, 50); // tap 1: 200 ms, 50% feedback
        d.setTap (1, 100, -50, 350, 0);  // tap 2: 350 ms, no feedback
        auto out = runClick (d, ms (1200));
        // Tap 1 feeds the click back in at 200 ms, so tap 2 also plays it at 200 + 350 = 550 ms.
        check (std::abs (out[(size_t) ms (550)] - 0.5f) < 1e-4f, "Tap 1's feedback bleeds into tap 2 (550 ms)");
    }

    // 4. Total feedback under 100% dies out; over 100% runs away.
    auto energyAt = [] (float fbEach, double startMs, double endMs)
    {
        Mpc3kDelay d; d.prepare (sr);
        d.setTap (0, 100, 0, 100, fbEach);
        d.setTap (1, 100, 0, 170, fbEach);
        d.setTap (2, 100, 0, 230, fbEach);
        auto out = runClick (d, ms (endMs) + 1);
        double e = 0.0;
        for (int i = ms (startMs); i <= ms (endMs); ++i)
            e += (double) out[(size_t) i] * out[(size_t) i];
        return e;
    };
    {
        const double early = energyAt (30, 0, 1000), late = energyAt (30, 9000, 10000);
        check (late < early * 0.01, "3 x 30% feedback (90% total) dies out");
    }
    {
        const double early = energyAt (40, 0, 1000), late = energyAt (40, 9000, 10000);
        check (late > early, "3 x 40% feedback (120% total) runs away");
    }

    // 5. Invalid settings (NaN / infinity) are ignored instead of crashing.
    {
        Mpc3kDelay d; d.prepare (sr);
        d.setTap (0, NAN, INFINITY, NAN, -INFINITY);
        auto out = runClick (d, ms (100));
        bool finite = true;
        for (float v : out) finite = finite && std::isfinite (v);
        check (finite, "Invalid settings don't crash or produce garbage");
    }

    std::printf ("\n%s (%d failed)\n", failures == 0 ? "All checks passed" : "Some checks failed", failures);
    return failures == 0 ? 0 : 1;
}

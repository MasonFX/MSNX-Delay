// lfo_tempo_test.cpp
// Checks the LFO shapes and the tempo-sync math.

#include "Lfo.h"
#include "Tempo.h"

#include <cmath>
#include <cstdio>
#include <string_view>

static int failures = 0;

static void check (bool ok, const char* what)
{
    std::printf ("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (! ok) ++failures;
}

static bool near (double a, double b, double tol = 1e-3) { return std::abs (a - b) < tol; }

int main()
{
    // --- Tempo math ---
    using tempo::beatsToMs;
    using tempo::delayDivisions;
    using tempo::lfoDivisions;

    check (near (beatsToMs (delayDivisions[5].beats, 120.0), 250.0),    "1/8 at 120 BPM = 250 ms");
    check (near (beatsToMs (delayDivisions[6].beats, 120.0), 375.0),    "1/8D at 120 BPM = 375 ms");
    check (near (beatsToMs (delayDivisions[7].beats, 90.0), 444.444),   "1/4T at 90 BPM = 444.4 ms");
    check (near (beatsToMs (lfoDivisions[13].beats, 120.0), 2000.0),    "1 bar at 120 BPM = 2000 ms");
    check (std::string_view (delayDivisions[tempo::defaultDelayDivision].name) == "1/8", "Default tap division is 1/8");
    check (std::string_view (lfoDivisions[tempo::defaultLfoDivision].name) == "1 bar",  "Default LFO division is 1 bar");

    // --- LFO shapes ---
    Lfo lfo;
    lfo.seed (1);

    lfo.setShape (Lfo::Shape::Sine);
    lfo.setPhase (0.25);
    check (near (lfo.value(), 1.0), "Sine peaks a quarter of the way through");

    lfo.setShape (Lfo::Shape::Triangle);
    lfo.setPhase (0.5);
    check (near (lfo.value(), 1.0), "Triangle peaks halfway");

    lfo.setShape (Lfo::Shape::Square);
    lfo.setPhase (0.75);
    check (near (lfo.value(), -1.0), "Square is low in the second half");

    lfo.setShape (Lfo::Shape::Saw);
    lfo.setPhase (0.9);
    check (near (lfo.value(), 0.8), "Saw rises steadily");

    // --- Random shape holds a value for a whole cycle, then picks a new one ---
    lfo.setShape (Lfo::Shape::Random);
    lfo.setPhase (0.1);
    const float first = lfo.value();
    lfo.advance (0.3);
    check (lfo.value() == first, "Random holds its value within a cycle");
    lfo.advance (0.7);
    check (lfo.value() != first, "Random picks a new value on the next cycle");
    check (lfo.value() >= -1.0f && lfo.value() <= 1.0f, "Random stays between -1 and +1");

    // --- Free-running speed: 4 Hz for 0.25 s = exactly one cycle ---
    Lfo freeRun;
    freeRun.seed (2);
    for (int i = 0; i < 100; ++i)
        freeRun.advance (4.0 / 400.0); // 100 steps of 1/400 s at 4 Hz
    check (near (freeRun.getPhase(), 0.0, 1e-9) || near (freeRun.getPhase(), 1.0, 1e-9), "Free-running LFO completes one cycle in the right time");

    std::printf ("\n%s (%d failed)\n", failures == 0 ? "All checks passed" : "Some checks failed", failures);
    return failures == 0 ? 0 : 1;
}

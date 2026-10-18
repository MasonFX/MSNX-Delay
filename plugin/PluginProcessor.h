// PluginProcessor.h
// JUCE wrapper around the MPC3000 delay core.
// Holds the knobs (parameters), works out tempo sync and LFOs,
// and feeds the final settings to the core.

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Mpc3kDelay.h"
#include "Lfo.h"
#include "Tempo.h"

class Mpc3kDelayProcessor : public juce::AudioProcessor
{
public:
    Mpc3kDelayProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock; // keeps the double-precision version visible (silences a warning)

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 1.5; }

    int getNumPrograms() override    { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // All the knobs live here. The editor connects to them by ID.
    juce::AudioProcessorValueTreeState params;

    // Parameter IDs, e.g. tapId (0, "time") -> "tap1_time"
    static juce::String tapId (int tap, const char* name)
    {
        return "tap" + juce::String (tap + 1) + "_" + name;
    }

    // What each LFO can move. The order matches the "LFO Target" menu.
    enum LfoTarget { targetOff = 0, targetVolume, targetPan, targetTime, targetFeedback };

    // Menu contents, shared by the parameters and the front panel.
    static const juce::StringArray& lfoTargetNames()
    {
        static const juce::StringArray names { "Off", "Volume", "Pan", "Time", "Feedback" };
        return names;
    }

    static const juce::StringArray& lfoShapeNames()
    {
        static const juce::StringArray names { "Sine", "Triangle", "Saw", "Square", "Random" };
        return names;
    }

    static juce::StringArray delayDivisionNames()
    {
        juce::StringArray names;
        for (const auto& d : tempo::delayDivisions)
            names.add (d.name);
        return names;
    }

    static juce::StringArray lfoDivisionNames()
    {
        juce::StringArray names;
        for (const auto& d : tempo::lfoDivisions)
            names.add (d.name);
        return names;
    }

    // What each tap is actually doing right now, after tempo sync and its LFO.
    // Written by the audio, read by the front panel's display.
    struct LiveTap
    {
        std::atomic<float> volume   { 0.0f };
        std::atomic<float> pan      { 0.0f };
        std::atomic<float> timeMs   { 0.0f };
        std::atomic<float> feedback { 0.0f };
    };
    std::array<LiveTap, Mpc3kDelay::numTaps> live;
    std::atomic<float> liveBpm { 120.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Works out one tap's final settings (tempo sync + LFO) and sends them to the core.
    void updateTap (int index, int numSamples, double bpm, bool lockToHost, double beat);

    Mpc3kDelay delay;
    std::array<Lfo, Mpc3kDelay::numTaps> lfos;
    double currentSampleRate = 48000.0;

    // Settings are recalculated every 32 samples (under 1 ms), so even fast
    // LFOs move smoothly. The core glides between these steps.
    static constexpr int controlBlock = 32;

    // Fast pointers to the current knob values (read while playing).
    struct TapParams
    {
        std::atomic<float>* volume      = nullptr;
        std::atomic<float>* pan         = nullptr;
        std::atomic<float>* time        = nullptr;
        std::atomic<float>* feedback    = nullptr;
        std::atomic<float>* sync        = nullptr;
        std::atomic<float>* division    = nullptr;
        std::atomic<float>* lfoTarget   = nullptr;
        std::atomic<float>* lfoShape    = nullptr;
        std::atomic<float>* lfoSync     = nullptr;
        std::atomic<float>* lfoDivision = nullptr;
        std::atomic<float>* lfoRate     = nullptr;
        std::atomic<float>* lfoDepth    = nullptr;
    };
    std::array<TapParams, Mpc3kDelay::numTaps> tapParams;
    std::atomic<float>* mixParam = nullptr;

    // Scratch space, sized in prepareToPlay so nothing is allocated while playing.
    juce::AudioBuffer<float> monoIn, wet;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Mpc3kDelayProcessor)
};

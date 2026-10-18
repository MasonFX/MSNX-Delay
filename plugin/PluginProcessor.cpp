// PluginProcessor.cpp

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Reads a menu (choice) parameter as a safe index.
    int indexOf (const std::atomic<float>* param, int count)
    {
        return juce::jlimit (0, count - 1, (int) std::lround (param->load()));
    }

    juce::String percentText (float v)  { return juce::String (juce::roundToInt (v)) + "%"; }

    juce::String rateText (float ms)
    {
        return ms < 1000.0f ? juce::String (juce::roundToInt (ms)) + " ms"
                            : juce::String (ms / 1000.0f, 2) + " s";
    }

    float rateFromText (const juce::String& text)
    {
        const auto t = text.trim().toLowerCase();
        const float v = t.getFloatValue();
        const bool seconds = t.endsWith ("s") && ! t.endsWith ("ms");
        return seconds ? v * 1000.0f : v;
    }

    juce::String depthText (float v)
    {
        return (v < 10.0f ? juce::String (v, 2) : juce::String (v, 1)) + "%";
    }
}

Mpc3kDelayProcessor::Mpc3kDelayProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      params (*this, nullptr, "PARAMS", createLayout())
{
    for (int i = 0; i < Mpc3kDelay::numTaps; ++i)
    {
        auto& t       = tapParams[(size_t) i];
        t.volume      = params.getRawParameterValue (tapId (i, "vol"));
        t.pan         = params.getRawParameterValue (tapId (i, "pan"));
        t.time        = params.getRawParameterValue (tapId (i, "time"));
        t.feedback    = params.getRawParameterValue (tapId (i, "fdbk"));
        t.sync        = params.getRawParameterValue (tapId (i, "sync"));
        t.division    = params.getRawParameterValue (tapId (i, "div"));
        t.lfoTarget   = params.getRawParameterValue (tapId (i, "lfo_target"));
        t.lfoShape    = params.getRawParameterValue (tapId (i, "lfo_shape"));
        t.lfoSync     = params.getRawParameterValue (tapId (i, "lfo_sync"));
        t.lfoDivision = params.getRawParameterValue (tapId (i, "lfo_div"));
        t.lfoRate     = params.getRawParameterValue (tapId (i, "lfo_rate"));
        t.lfoDepth    = params.getRawParameterValue (tapId (i, "lfo_depth"));

        lfos[(size_t) i].seed ((uint32_t) i + 1);
    }
    mixParam = params.getRawParameterValue ("mix");
}

juce::AudioProcessorValueTreeState::ParameterLayout Mpc3kDelayProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < Mpc3kDelay::numTaps; ++i)
    {
        const String tap = "Channel " + String (i + 1) + " ";

        // --- The original MPC3000 controls (same ranges as its screen) ---
        // Tap 1 starts audible, taps 2 and 3 start silent.
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { tapId (i, "vol"), 1 }, tap + "Volume",
                                                           NormalisableRange<float> (0.0f, 100.0f, 1.0f),
                                                           i == 0 ? 100.0f : 0.0f,
                                                           AudioParameterFloatAttributes().withStringFromValueFunction (
                                                               [] (float v, int) { return String (roundToInt (v)); })));

        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { tapId (i, "pan"), 1 }, tap + "Pan",
                                                           NormalisableRange<float> (-50.0f, 50.0f, 1.0f), 0.0f,
                                                           AudioParameterFloatAttributes().withStringFromValueFunction (
                                                               [] (float v, int)
                                                               {
                                                                   const int p = roundToInt (v);
                                                                   if (p == 0) return String ("C");
                                                                   return p < 0 ? String (-p) + "L" : String (p) + "R";
                                                               })));

        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { tapId (i, "time"), 1 }, tap + "Time",
                                                           NormalisableRange<float> (Mpc3kDelay::minTimeMs, Mpc3kDelay::maxTimeMs, 1.0f),
                                                           Mpc3kDelay::maxTimeMs,
                                                           AudioParameterFloatAttributes()
                                                               .withLabel ("ms")
                                                               .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " ms"; })));

        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { tapId (i, "fdbk"), 1 }, tap + "Feedback",
                                                           NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f,
                                                           AudioParameterFloatAttributes()
                                                               .withLabel ("%")
                                                               .withStringFromValueFunction ([] (float v, int) { return percentText (v); })));

        // --- Tempo sync for this tap's Time ---
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { tapId (i, "sync"), 2 }, tap + "Time Sync", false));

        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { tapId (i, "div"), 2 }, tap + "Time Division",
                                                            delayDivisionNames(), tempo::defaultDelayDivision));

        // --- This tap's LFO ---
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { tapId (i, "lfo_target"), 2 }, tap + "LFO Target",
                                                            lfoTargetNames(), targetOff));

        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { tapId (i, "lfo_shape"), 2 }, tap + "LFO Shape",
                                                            lfoShapeNames(), 0));

        layout.add (std::make_unique<AudioParameterBool> (ParameterID { tapId (i, "lfo_sync"), 2 }, tap + "LFO Sync", true));

        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { tapId (i, "lfo_div"), 2 }, tap + "LFO Division",
                                                            lfoDivisionNames(), tempo::defaultLfoDivision));

        // Free-running rate, as the length of one LFO cycle. Skewed so short times get more of the knob.
        NormalisableRange<float> rateRange (20.0f, 10000.0f, 1.0f);
        rateRange.setSkewForCentre (500.0f);
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { tapId (i, "lfo_rate"), 2 }, tap + "LFO Rate",
                                                           rateRange, 500.0f,
                                                           AudioParameterFloatAttributes()
                                                               .withLabel ("ms")
                                                               .withStringFromValueFunction ([] (float v, int) { return rateText (v); })
                                                               .withValueFromStringFunction ([] (const String& s) { return rateFromText (s); })));

        // How far the LFO pushes the target, as % of that knob's full range, in both directions.
        // Skewed so small amounts (subtle Time wobble) are easy to dial in.
        NormalisableRange<float> depthRange (0.0f, 100.0f, 0.01f);
        depthRange.setSkewForCentre (10.0f);
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { tapId (i, "lfo_depth"), 2 }, tap + "LFO Depth",
                                                           depthRange, 10.0f,
                                                           AudioParameterFloatAttributes()
                                                               .withLabel ("%")
                                                               .withStringFromValueFunction ([] (float v, int) { return depthText (v); })));
    }

    // Extra (not on the original): dry/wet. 100% = wet only, for use on a send.
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                                                       NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f,
                                                       AudioParameterFloatAttributes()
                                                           .withLabel ("%")
                                                           .withStringFromValueFunction ([] (float v, int) { return percentText (v); })));
    return layout;
}

bool Mpc3kDelayProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Stereo out; mono or stereo in.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    const auto in = layouts.getMainInputChannelSet();
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void Mpc3kDelayProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    delay.prepare (currentSampleRate);
    monoIn.setSize (1, samplesPerBlock);
    wet.setSize (2, samplesPerBlock);
}

void Mpc3kDelayProcessor::updateTap (int i, int numSamples, double bpm, bool lockToHost, double beat)
{
    const auto& p = tapParams[(size_t) i];
    auto& lfo     = lfos[(size_t) i];

    float volume   = p.volume->load();
    float pan      = p.pan->load();
    float timeMs   = p.time->load();
    float feedback = p.feedback->load();

    // 1. Tempo sync: replace the Time knob with a note length at the current tempo.
    if (p.sync->load() > 0.5f)
    {
        const auto& div = tempo::delayDivisions[(size_t) indexOf (p.division, (int) tempo::delayDivisions.size())];
        timeMs = (float) tempo::beatsToMs (div.beats, bpm);
    }

    // 2. The LFO: work out where the wave is, then move it on.
    lfo.setShape ((Lfo::Shape) indexOf (p.lfoShape, lfoShapeNames().size()));

    const bool lfoSynced = p.lfoSync->load() > 0.5f;
    double cyclesPerSecond;

    if (lfoSynced)
    {
        const auto& div = tempo::lfoDivisions[(size_t) indexOf (p.lfoDivision, (int) tempo::lfoDivisions.size())];
        cyclesPerSecond = bpm / 60.0 / div.beats;

        // While the DAW is playing, lock the wave to its bar position so it
        // lands the same way every time you press play.
        if (lockToHost)
            lfo.setPhase (beat / div.beats);
    }
    else
    {
        cyclesPerSecond = 1000.0 / juce::jmax (1.0f, p.lfoRate->load());
    }

    const float wave = lfo.value();

    if (! (lfoSynced && lockToHost))
        lfo.advance (cyclesPerSecond * numSamples / currentSampleRate);

    // 3. Push the chosen knob by depth (% of its full range, both directions).
    const float amount = wave * p.lfoDepth->load() / 100.0f;

    switch (indexOf (p.lfoTarget, lfoTargetNames().size()))
    {
        case targetVolume:   volume   += amount * 100.0f; break;
        case targetPan:      pan      += amount * 100.0f; break;
        case targetTime:     timeMs   += amount * (Mpc3kDelay::maxTimeMs - Mpc3kDelay::minTimeMs); break;
        case targetFeedback: feedback += amount * 100.0f; break;
        default: break;
    }

    volume   = juce::jlimit (0.0f, 100.0f, volume);
    pan      = juce::jlimit (-50.0f, 50.0f, pan);
    timeMs   = juce::jlimit (Mpc3kDelay::minTimeMs, Mpc3kDelay::maxTimeMs, timeMs);
    feedback = juce::jlimit (0.0f, 100.0f, feedback);

    delay.setTap (i, volume, pan, timeMs, feedback);

    auto& l = live[(size_t) i];
    l.volume.store (volume);
    l.pan.store (pan);
    l.timeMs.store (timeMs);
    l.feedback.store (feedback);
}

void Mpc3kDelayProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numIn      = getTotalNumInputChannels();

    if (numIn == 0 || buffer.getNumChannels() < 2)
    {
        buffer.clear();
        return;
    }

    // Hosts can occasionally send a bigger block than promised.
    if (numSamples > monoIn.getNumSamples())
    {
        monoIn.setSize (1, numSamples, false, false, true);
        wet.setSize (2, numSamples, false, false, true);
    }

    // 1. Ask the DAW for its tempo and position (120 BPM if it doesn't say).
    double bpm = 120.0, beatAtStart = 0.0;
    bool hostPlaying = false;

    if (auto* host = getPlayHead())
    {
        if (auto pos = host->getPosition())
        {
            if (auto b = pos->getBpm())
                bpm = juce::jlimit (20.0, 999.0, *b);

            if (pos->getIsPlaying())
            {
                if (auto ppq = pos->getPpqPosition())
                {
                    beatAtStart = *ppq;
                    hostPlaying = true;
                }
            }
        }
    }
    liveBpm.store ((float) bpm);

    const double beatsPerSample = bpm / 60.0 / currentSampleRate;

    // 2. Sum the input to mono (the core, like the MPC3000 delay, takes one input).
    auto* mono = monoIn.getWritePointer (0);
    if (numIn >= 2)
    {
        const auto* l = buffer.getReadPointer (0);
        const auto* r = buffer.getReadPointer (1);
        for (int n = 0; n < numSamples; ++n)
            mono[n] = 0.5f * (l[n] + r[n]);
    }
    else
    {
        juce::FloatVectorOperations::copy (mono, buffer.getReadPointer (0), numSamples);
    }

    // 3. Run the delay in small steps, updating tempo sync and LFOs before each one.
    auto* wetL = wet.getWritePointer (0);
    auto* wetR = wet.getWritePointer (1);

    for (int start = 0; start < numSamples; start += controlBlock)
    {
        const int len     = juce::jmin (controlBlock, numSamples - start);
        const double beat = beatAtStart + start * beatsPerSample;

        for (int i = 0; i < Mpc3kDelay::numTaps; ++i)
            updateTap (i, len, bpm, hostPlaying, beat);

        delay.process (mono + start, wetL + start, wetR + start, len);
    }

    // 4. Blend dry and wet into the output.
    // With a mono input, the dry signal (kept in the mono buffer) goes to both sides.
    const float wetAmt = mixParam->load() / 100.0f;
    const float dryAmt = 1.0f - wetAmt;

    for (int ch = 0; ch < 2; ++ch)
    {
        const auto* dry = numIn >= 2 ? buffer.getReadPointer (ch) : monoIn.getReadPointer (0);
        auto* out       = buffer.getWritePointer (ch);
        const auto* w   = wet.getReadPointer (ch);

        for (int n = 0; n < numSamples; ++n)
            out[n] = dry[n] * dryAmt + w[n] * wetAmt;
    }
}

juce::AudioProcessorEditor* Mpc3kDelayProcessor::createEditor()
{
    return new Mpc3kDelayEditor (*this);
}

void Mpc3kDelayProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = params.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void Mpc3kDelayProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (params.state.getType()))
            params.replaceState (juce::ValueTree::fromXml (*xml));
}

// JUCE calls this to create the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Mpc3kDelayProcessor();
}

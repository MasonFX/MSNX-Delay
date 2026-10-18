// PluginEditor.h
// The front panel: a live LCD readout modeled on the MPC3000's delay screen,
// then one section per tap (its four knobs, time sync, and its LFO), plus Mix.

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// Draws the knobs, buttons and menus in the panel's style.
class Mpc3kLookAndFeel : public juce::LookAndFeel_V4
{
public:
    Mpc3kLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
};

// The green backlit screen. Shows what every tap is doing right now,
// including tempo sync and LFO movement.
class LcdDisplay : public juce::Component
{
public:
    explicit LcdDisplay (Mpc3kDelayProcessor&);
    void paint (juce::Graphics&) override;

private:
    Mpc3kDelayProcessor& proc;
};

class Mpc3kDelayEditor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit Mpc3kDelayEditor (Mpc3kDelayProcessor&);
    ~Mpc3kDelayEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    struct Choice
    {
        juce::ComboBox box;
        juce::Label    label;
        std::unique_ptr<ComboAttachment> attachment;
    };

    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<ButtonAttachment> attachment;
    };

    struct TapUi
    {
        juce::Label title, lfoHeading;
        Knob   volume, pan, time, feedback;
        Toggle timeSync;
        Choice timeDivision;
        Choice lfoTarget, lfoShape;
        Knob   lfoRate, lfoDepth;
        Toggle lfoSync;
        Choice lfoDivision;
        juce::Rectangle<int> panel, lfoArea;
    };

    void setUpKnob   (Knob&,   const juce::String& paramId, const juce::String& name);
    void setUpChoice (Choice&, const juce::String& paramId, const juce::String& name, const juce::StringArray& items);
    void setUpToggle (Toggle&, const juce::String& paramId, const juce::String& text);

    // Dims the controls that currently do nothing (e.g. the Time knob while synced).
    void updateControlStates();
    void timerCallback() override;

    Mpc3kDelayProcessor& proc;
    Mpc3kLookAndFeel lookAndFeel;   // declared before the controls so it outlives them

    LcdDisplay lcd;
    std::array<TapUi, Mpc3kDelay::numTaps> taps;
    Knob mix;
    juce::Rectangle<int> mixPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Mpc3kDelayEditor)
};

// PluginEditor.cpp

#include "PluginEditor.h"

namespace
{
    // --- Palette: warm grey hardware panel, charcoal knobs, green backlit LCD ---
    const juce::Colour panelColour   { 0xffcfcac0 };
    const juce::Colour sectionColour { 0xffc0baae };
    const juce::Colour lfoColour     { 0xffb2ac9f };
    const juce::Colour edgeColour    { 0xff8f897d };
    const juce::Colour ink           { 0xff2b2a28 };
    const juce::Colour quietInk      { 0xff5c5850 };
    const juce::Colour headerColour  { 0xff2e2e30 };
    const juce::Colour headerInk     { 0xffe8e4da };
    const juce::Colour headerQuiet   { 0xff9c978d };
    const juce::Colour accent        { 0xffd9542b };
    const juce::Colour knobCap       { 0xff3a3a3c };
    const juce::Colour knobEdge      { 0xff1e1e20 };
    const juce::Colour knobTrack     { 0xff9a9488 };
    const juce::Colour pointerInk    { 0xfff2efe8 };
    const juce::Colour fieldColour   { 0xffe9e5dc };
    const juce::Colour buttonColour  { 0xffdcd7cc };
    const juce::Colour ledOff        { 0xff6e4a40 };
    const juce::Colour ledOn         { 0xffff5a36 };
    const juce::Colour lcdBezel      { 0xff1c1c1d };
    const juce::Colour lcdBack       { 0xffb4c289 };
    const juce::Colour lcdInk        { 0xff222d14 };
    const juce::Colour lcdDim        { 0xff5d6b40 };

    // --- Layout (pixels) ---
    constexpr int editorW  = 1030;
    constexpr int editorH  = 572;
    constexpr int margin   = 20;
    constexpr int headerH  = 44;
    constexpr int lcdY     = 56;
    constexpr int lcdH     = 136;
    constexpr int panelY   = 206;
    constexpr int panelW   = 292;
    constexpr int panelH   = 346;
    constexpr int panelGap = 14;
    constexpr int knobW    = 68;

    juce::Font uiFont (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    }

    juce::Font lcdFont (float size)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain));
    }

    void styleLabel (juce::Label& l, float size, bool bold, juce::Justification just)
    {
        l.setFont (uiFont (size, bold));
        l.setColour (juce::Label::textColourId, ink);
        l.setJustificationType (just);
        l.setInterceptsMouseClicks (false, false);
    }

    int choiceIndex (Mpc3kDelayProcessor& p, const juce::String& id)
    {
        return juce::roundToInt (p.params.getRawParameterValue (id)->load());
    }

    bool isOn (Mpc3kDelayProcessor& p, const juce::String& id)
    {
        return p.params.getRawParameterValue (id)->load() > 0.5f;
    }

    juce::String plusMinus() { return juce::String (juce::CharPointer_UTF8 ("\xc2\xb1")); }
}

//==============================================================================
// Look and feel

Mpc3kLookAndFeel::Mpc3kLookAndFeel()
{
    using namespace juce;

    setColour (Label::textColourId, ink);

    setColour (Slider::textBoxTextColourId, ink);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxHighlightColourId, accent.withAlpha (0.35f));

    setColour (TextEditor::backgroundColourId, fieldColour);
    setColour (TextEditor::textColourId, ink);
    setColour (TextEditor::highlightColourId, accent.withAlpha (0.35f));
    setColour (TextEditor::outlineColourId, edgeColour);
    setColour (TextEditor::focusedOutlineColourId, accent);
    setColour (CaretComponent::caretColourId, ink);

    setColour (ComboBox::backgroundColourId, fieldColour);
    setColour (ComboBox::textColourId, ink);
    setColour (ComboBox::outlineColourId, edgeColour);
    setColour (ComboBox::arrowColourId, quietInk);
    setColour (ComboBox::focusedOutlineColourId, accent);

    setColour (PopupMenu::backgroundColourId, fieldColour);
    setColour (PopupMenu::textColourId, ink);
    setColour (PopupMenu::highlightedBackgroundColourId, headerColour);
    setColour (PopupMenu::highlightedTextColourId, headerInk);
}

void Mpc3kLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float startAngle, float endAngle, juce::Slider& slider)
{
    using namespace juce;

    const auto bounds = Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre  = bounds.getCentre();
    const float angle  = startAngle + sliderPos * (endAngle - startAngle);
    const PathStrokeType stroke (3.0f, PathStrokeType::curved, PathStrokeType::rounded);

    // Track
    Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (knobTrack);
    g.strokePath (track, stroke);

    // Value arc. Knobs that go both ways (Pan) light up from the middle.
    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float from   = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;

    if (std::abs (angle - from) > 0.001f)
    {
        Path value;
        value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, jmin (from, angle), jmax (from, angle), true);
        g.setColour (accent);
        g.strokePath (value, stroke);
    }

    // Cap
    const float capR = radius - 6.0f;
    g.setColour (knobCap);
    g.fillEllipse (centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f);
    g.setColour (knobEdge);
    g.drawEllipse (centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f, 1.5f);

    // Pointer line
    Path pointer;
    pointer.addRoundedRectangle (-1.5f, -capR + 3.0f, 3.0f, capR * 0.6f, 1.5f);
    g.setColour (pointerInk);
    g.fillPath (pointer, AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

void Mpc3kLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                         bool highlighted, bool down)
{
    using namespace juce;

    const auto r = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = button.getToggleState();

    g.setColour (down ? buttonColour.darker (0.15f) : highlighted ? buttonColour.brighter (0.08f) : buttonColour);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (edgeColour);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    // Little LED, lit when the button is on.
    const auto led = Rectangle<float> (r.getX() + 9.0f, r.getCentreY() - 4.0f, 8.0f, 8.0f);
    if (on)
    {
        g.setColour (ledOn.withAlpha (0.3f));
        g.fillEllipse (led.expanded (3.0f));
    }
    g.setColour (on ? ledOn : ledOff);
    g.fillEllipse (led);

    g.setColour (ink);
    g.setFont (uiFont (11.5f, true));
    g.drawText (button.getButtonText(), r.withTrimmedLeft (24.0f), Justification::centredLeft);
}

juce::Font Mpc3kLookAndFeel::getComboBoxFont (juce::ComboBox&) { return uiFont (13.0f); }
juce::Font Mpc3kLookAndFeel::getPopupMenuFont()                { return uiFont (14.0f); }

//==============================================================================
// LCD

LcdDisplay::LcdDisplay (Mpc3kDelayProcessor& p) : proc (p)
{
    setInterceptsMouseClicks (false, false);
}

void LcdDisplay::paint (juce::Graphics& g)
{
    using namespace juce;
    using P = Mpc3kDelayProcessor;

    auto bounds = getLocalBounds().toFloat();

    g.setColour (lcdBezel);
    g.fillRoundedRectangle (bounds, 8.0f);

    const auto screen = bounds.reduced (8.0f);
    g.setColour (lcdBack);
    g.fillRoundedRectangle (screen, 4.0f);

    const auto area = screen.reduced (16.0f, 8.0f).toNearestInt();
    const int rowH  = area.getHeight() / 5;
    auto rowRect = [&] (int row) { return area.withY (area.getY() + row * rowH).withHeight (rowH); };

    // Column positions (from the left of the screen text area).
    struct Col { int x, w; Justification just; };
    const Col cLabel { 0,   96,  Justification::centredLeft };
    const Col cVol   { 96,  48,  Justification::centredRight };
    const Col cPan   { 158, 52,  Justification::centredRight };
    const Col cTime  { 236, 190, Justification::centredLeft };
    const Col cFdbk  { 426, 56,  Justification::centredRight };
    const Col cLfo   { 516, area.getWidth() - 516, Justification::centredLeft };

    auto cell = [&] (int row, const Col& c, const String& text)
    {
        auto r = rowRect (row);
        g.drawText (text, r.getX() + c.x, r.getY(), c.w, r.getHeight(), c.just, true);
    };

    g.setFont (lcdFont (15.0f));

    // Row 0: title, tempo and mix
    g.setColour (lcdInk);
    cell (0, { 0, 300, Justification::centredLeft }, "Effects (Delay)");
    const auto mixText = "Mix " + String (roundToInt (proc.params.getRawParameterValue ("mix")->load())) + "%";
    cell (0, { 0, area.getWidth(), Justification::centredRight },
          String (proc.liveBpm.load(), 1) + " BPM    " + mixText);

    g.setColour (lcdDim);
    const auto line = rowRect (0).getBottom() - 2;
    g.drawHorizontalLine (line, (float) area.getX(), (float) area.getRight());

    // Row 1: column headings
    cell (1, cVol,  "Vol");
    cell (1, cPan,  "Pan");
    cell (1, cTime, "Time");
    cell (1, cFdbk, "Fdbk");
    cell (1, cLfo,  "LFO");

    // Rows 2-4: one per tap, showing the values it's using right now.
    g.setColour (lcdInk);
    for (int t = 0; t < Mpc3kDelay::numTaps; ++t)
    {
        const int row = t + 2;
        const auto& l = proc.live[(size_t) t];

        const int pan = roundToInt (l.pan.load());
        const String panText = pan == 0 ? String ("C") : pan < 0 ? String (-pan) + "L" : String (pan) + "R";

        String timeText = String (roundToInt (l.timeMs.load())) + " ms";
        if (isOn (proc, P::tapId (t, "sync")))
            timeText += "  " + P::delayDivisionNames()[choiceIndex (proc, P::tapId (t, "div"))];

        String lfoText = "--";
        const int target = choiceIndex (proc, P::tapId (t, "lfo_target"));
        if (target != P::targetOff)
        {
            const String rate = isOn (proc, P::tapId (t, "lfo_sync"))
                                  ? P::lfoDivisionNames()[choiceIndex (proc, P::tapId (t, "lfo_div"))]
                                  : proc.params.getParameter (P::tapId (t, "lfo_rate"))->getCurrentValueAsText();

            lfoText = P::lfoTargetNames()[target] + "  "
                    + P::lfoShapeNames()[choiceIndex (proc, P::tapId (t, "lfo_shape"))] + "  "
                    + rate + "  "
                    + plusMinus() + proc.params.getParameter (P::tapId (t, "lfo_depth"))->getCurrentValueAsText();
        }

        cell (row, cLabel, "Delay " + String (t + 1) + "=");
        cell (row, cVol,   String (roundToInt (l.volume.load())));
        cell (row, cPan,   panText);
        cell (row, cTime,  timeText);
        cell (row, cFdbk,  String (roundToInt (l.feedback.load())) + "%");
        cell (row, cLfo,   lfoText);
    }
}

//==============================================================================
// Editor

Mpc3kDelayEditor::Mpc3kDelayEditor (Mpc3kDelayProcessor& p)
    : AudioProcessorEditor (&p), proc (p), lcd (p)
{
    using P = Mpc3kDelayProcessor;

    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (lcd);

    for (int t = 0; t < Mpc3kDelay::numTaps; ++t)
    {
        auto& ui = taps[(size_t) t];

        ui.title.setText ("CHANNEL " + juce::String (t + 1), juce::dontSendNotification);
        styleLabel (ui.title, 15.0f, true, juce::Justification::centredLeft);
        addAndMakeVisible (ui.title);

        setUpKnob (ui.volume,   P::tapId (t, "vol"),  "VOLUME");
        setUpKnob (ui.pan,      P::tapId (t, "pan"),  "PAN");
        setUpKnob (ui.time,     P::tapId (t, "time"), "TIME");
        setUpKnob (ui.feedback, P::tapId (t, "fdbk"), "FEEDBACK");

        setUpToggle (ui.timeSync,     P::tapId (t, "sync"), "TIME SYNC");
        setUpChoice (ui.timeDivision, P::tapId (t, "div"), {}, P::delayDivisionNames());

        ui.lfoHeading.setText ("LFO", juce::dontSendNotification);
        styleLabel (ui.lfoHeading, 13.0f, true, juce::Justification::centredLeft);
        addAndMakeVisible (ui.lfoHeading);

        setUpChoice (ui.lfoTarget,   P::tapId (t, "lfo_target"), "TARGET", P::lfoTargetNames());
        setUpChoice (ui.lfoShape,    P::tapId (t, "lfo_shape"),  "SHAPE",  P::lfoShapeNames());
        setUpKnob   (ui.lfoRate,     P::tapId (t, "lfo_rate"),   "RATE");
        setUpToggle (ui.lfoSync,     P::tapId (t, "lfo_sync"),   "RATE SYNC");
        setUpChoice (ui.lfoDivision, P::tapId (t, "lfo_div"), {}, P::lfoDivisionNames());
        setUpKnob   (ui.lfoDepth,    P::tapId (t, "lfo_depth"),  "DEPTH");
    }

    setUpKnob (mix, "mix", "MIX");

    setSize (editorW, editorH);
    updateControlStates();
    startTimerHz (30);
}

Mpc3kDelayEditor::~Mpc3kDelayEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void Mpc3kDelayEditor::setUpKnob (Knob& k, const juce::String& paramId, const juce::String& name)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    // Set the value readout's colours on the knob itself (it's built before the knob joins the panel).
    k.slider.setColour (juce::Slider::textBoxTextColourId, ink);
    k.slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k.slider.setColour (juce::Slider::textBoxHighlightColourId, accent.withAlpha (0.35f));
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, knobW, 16);
    addAndMakeVisible (k.slider);

    k.label.setText (name, juce::dontSendNotification);
    styleLabel (k.label, 11.0f, true, juce::Justification::centred);
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<SliderAttachment> (proc.params, paramId, k.slider);

    // Double-click a knob to put it back to its default.
    if (auto* param = proc.params.getParameter (paramId))
        k.slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
}

void Mpc3kDelayEditor::setUpChoice (Choice& c, const juce::String& paramId, const juce::String& name,
                                    const juce::StringArray& items)
{
    c.box.addItemList (items, 1);
    c.box.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (c.box);

    if (name.isNotEmpty())
    {
        c.label.setText (name, juce::dontSendNotification);
        styleLabel (c.label, 11.0f, true, juce::Justification::centredLeft);
        addAndMakeVisible (c.label);
    }

    c.attachment = std::make_unique<ComboAttachment> (proc.params, paramId, c.box);
}

void Mpc3kDelayEditor::setUpToggle (Toggle& t, const juce::String& paramId, const juce::String& text)
{
    t.button.setButtonText (text);
    addAndMakeVisible (t.button);
    t.attachment = std::make_unique<ButtonAttachment> (proc.params, paramId, t.button);
}

void Mpc3kDelayEditor::updateControlStates()
{
    using P = Mpc3kDelayProcessor;

    auto show = [] (juce::Component& c, bool active, float alphaScale = 1.0f)
    {
        c.setEnabled (active);
        c.setAlpha ((active ? 1.0f : 0.35f) * alphaScale);
    };

    for (int t = 0; t < Mpc3kDelay::numTaps; ++t)
    {
        auto& ui = taps[(size_t) t];

        // Time: the knob while free, the note menu while synced.
        const bool timeSynced = isOn (proc, P::tapId (t, "sync"));
        show (ui.time.slider,       ! timeSynced);
        show (ui.time.label,        ! timeSynced);
        show (ui.timeDivision.box,  timeSynced);

        // LFO: everything fades back while its target is Off (still adjustable).
        const bool lfoOn   = choiceIndex (proc, P::tapId (t, "lfo_target")) != P::targetOff;
        const float fade   = lfoOn ? 1.0f : 0.6f;
        const bool rateSynced = isOn (proc, P::tapId (t, "lfo_sync"));

        show (ui.lfoShape.box,     true, fade);
        show (ui.lfoShape.label,   true, fade);
        show (ui.lfoSync.button,   true, fade);
        show (ui.lfoDepth.slider,  true, fade);
        show (ui.lfoDepth.label,   true, fade);
        show (ui.lfoRate.slider,   ! rateSynced, fade);
        show (ui.lfoRate.label,    ! rateSynced, fade);
        show (ui.lfoDivision.box,  rateSynced, fade);
    }
}

void Mpc3kDelayEditor::timerCallback()
{
    updateControlStates();
    lcd.repaint();
}

void Mpc3kDelayEditor::paint (juce::Graphics& g)
{
    using namespace juce;

    g.fillAll (panelColour);

    // Header band
    g.setColour (headerColour);
    g.fillRect (0, 0, getWidth(), headerH);
    g.setColour (accent);
    g.fillRect (0, headerH, getWidth(), 3);

    g.setColour (headerInk);
    g.setFont (uiFont (20.0f, true));
    g.drawText ("MSNX DELAY", margin, 0, 300, headerH, Justification::centredLeft);

    g.setColour (headerQuiet);
    g.setFont (uiFont (12.0f, true));
    g.drawText ("3-CHANNEL SHARED-MEMORY DELAY", getWidth() - margin - 300, 0, 300, headerH, Justification::centredRight);

    // Tap sections, each with a darker LFO area
    for (const auto& ui : taps)
    {
        g.setColour (sectionColour);
        g.fillRoundedRectangle (ui.panel.toFloat(), 8.0f);
        g.setColour (edgeColour);
        g.drawRoundedRectangle (ui.panel.toFloat().reduced (0.5f), 8.0f, 1.0f);

        g.setColour (lfoColour);
        g.fillRoundedRectangle (ui.lfoArea.toFloat(), 6.0f);
    }

    // Mix section
    g.setColour (sectionColour);
    g.fillRoundedRectangle (mixPanel.toFloat(), 8.0f);
    g.setColour (edgeColour);
    g.drawRoundedRectangle (mixPanel.toFloat().reduced (0.5f), 8.0f, 1.0f);
}

void Mpc3kDelayEditor::resized()
{
    lcd.setBounds (margin, lcdY, editorW - 2 * margin, lcdH);

    auto placeKnob = [] (Knob& k, int x, int y, int w)
    {
        k.label.setBounds (x, y, w, 14);
        k.slider.setBounds (x, y + 14, w, 74);
    };

    auto placeChoice = [] (Choice& c, int x, int y, int w)
    {
        c.label.setBounds (x + 2, y, w, 14);
        c.box.setBounds (x, y + 14, w, 24);
    };

    for (int t = 0; t < Mpc3kDelay::numTaps; ++t)
    {
        auto& ui = taps[(size_t) t];
        const int px = margin + t * (panelW + panelGap);
        const int py = panelY;

        ui.panel   = { px, py, panelW, panelH };
        ui.lfoArea = { px + 6, py + 168, panelW - 12, panelH - 174 };

        ui.title.setBounds (px + 14, py + 8, 120, 22);

        // The four MPC3000 knobs
        placeKnob (ui.volume,   px + 8,             py + 34, knobW);
        placeKnob (ui.pan,      px + 8 + 69,        py + 34, knobW);
        placeKnob (ui.time,     px + 8 + 69 * 2,    py + 34, knobW);
        placeKnob (ui.feedback, px + 8 + 69 * 3,    py + 34, knobW);

        // Time sync row
        ui.timeSync.button.setBounds (px + 14, py + 130, 116, 26);
        ui.timeDivision.box.setBounds (px + 140, py + 131, panelW - 154, 24);

        // LFO area
        ui.lfoHeading.setBounds (px + 16, py + 174, 60, 18);
        placeChoice (ui.lfoTarget, px + 14,  py + 196, 128);
        placeChoice (ui.lfoShape,  px + 150, py + 196, 128);

        placeKnob (ui.lfoRate, px + 14, py + 244, 72);
        ui.lfoSync.button.setBounds    (px + 94, py + 262, 104, 26);
        ui.lfoDivision.box.setBounds   (px + 94, py + 296, 104, 24);
        placeKnob (ui.lfoDepth, px + 206, py + 244, 72);
    }

    const int mixX = margin + Mpc3kDelay::numTaps * (panelW + panelGap);
    mixPanel = { mixX, panelY, editorW - margin - mixX, panelH };
    placeKnob (mix, mixPanel.getX() + 2, mixPanel.getCentreY() - 50, mixPanel.getWidth() - 4);
}

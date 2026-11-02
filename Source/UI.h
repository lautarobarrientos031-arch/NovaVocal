#pragma once
#include "Common.h"
#include "Params.h"

namespace nv::theme
{
    inline const juce::Colour orange { 0xffff8c1a };
    inline const juce::Colour blue   { 0xff2b8fe0 };
    inline const juce::Colour bgTop  { 0xff22333f };
    inline const juce::Colour bgBot  { 0xff0c1318 };
    inline const juce::Colour card   { 0xff182027 };
    inline const juce::Colour edge   { 0xff2b3944 };
    inline const juce::Colour text   { 0xffeaf0f4 };
    inline const juce::Colour sub    { 0xff8a9ba7 };
    inline const juce::Colour dark   { 0xff10161b };
}

namespace nv
{
//==============================================================================
class NovaLnF : public juce::LookAndFeel_V4
{
public:
    NovaLnF()
    {
        using namespace theme;
        setColour (juce::TextButton::buttonColourId, card);
        setColour (juce::TextButton::buttonOnColourId, orange);
        setColour (juce::TextButton::textColourOffId, text);
        setColour (juce::TextButton::textColourOnId, dark);
        setColour (juce::ComboBox::backgroundColourId, dark);
        setColour (juce::ComboBox::outlineColourId, edge);
        setColour (juce::ComboBox::textColourId, text);
        setColour (juce::ComboBox::arrowColourId, orange);
        setColour (juce::PopupMenu::backgroundColourId, card);
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, orange);
        setColour (juce::PopupMenu::highlightedTextColourId, dark);
        setColour (juce::AlertWindow::backgroundColourId, card);
        setColour (juce::AlertWindow::textColourId, text);
        setColour (juce::AlertWindow::outlineColourId, edge);
        setColour (juce::TextEditor::backgroundColourId, dark);
        setColour (juce::TextEditor::textColourId, text);
        setColour (juce::TextEditor::outlineColourId, edge);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override { return F (12.f, true); }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        auto fill = on ? theme::orange : theme::card;
        if (down) fill = fill.brighter (0.15f); else if (over) fill = fill.brighter (0.08f);
        if (! b.isEnabled()) fill = fill.withAlpha (0.4f);
        g.setColour (fill);
        g.fillRoundedRectangle (r, 6.f);
        g.setColour (on ? theme::orange : theme::edge);
        g.drawRoundedRectangle (r, 6.f, 1.f);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.f);
        const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float th = std::max (3.f, r * 0.11f), ar = r - th * 0.5f;
        const float ang = a0 + pos * (a1 - a0);

        juce::Path track, val;
        track.addCentredArc (c.x, c.y, ar, ar, 0.f, a0, a1, true);
        val.addCentredArc (c.x, c.y, ar, ar, 0.f, a0, ang, true);
        const juce::PathStrokeType st (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        g.setColour (juce::Colour (0xff0a0f13));
        g.strokePath (track, st);
        g.setColour (theme::orange);
        g.strokePath (val, st);

        const float br = r - th * 1.9f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff35424d), c.x, c.y - br,
                                                 juce::Colour (0xff11171c), c.x, c.y + br, false));
        g.fillEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f);
        g.setColour (juce::Colour (0xff060a0d));
        g.drawEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f, 1.5f);

        juce::Path p;
        p.addRoundedRectangle (-1.6f, -br + 3.f, 3.2f, br * 0.42f, 1.5f);
        g.setColour (theme::orange);
        g.fillPath (p, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                           const juce::Slider::SliderStyle, juce::Slider&) override
    {
        const float cy = (float) y + (float) h * 0.5f;
        juce::Rectangle<float> tr ((float) x + 4.f, cy - 2.f, (float) w - 8.f, 4.f);
        g.setColour (juce::Colour (0xff0a0f13));
        g.fillRoundedRectangle (tr, 2.f);
        g.setColour (theme::orange);
        g.fillRoundedRectangle (tr.withRight (juce::jmax (tr.getX() + 1.f, pos)), 2.f);
        g.setColour (theme::text);
        g.fillEllipse (pos - 6.f, cy - 6.f, 12.f, 12.f);
        g.setColour (theme::orange);
        g.drawEllipse (pos - 6.f, cy - 6.f, 12.f, 12.f, 2.f);
    }
};

//==============================================================================
class PowerButton : public juce::ToggleButton
{
public:
    void paintButton (juce::Graphics& g, bool, bool) override
    {
        auto b = getLocalBounds().toFloat().reduced (2.f);
        const bool on = getToggleState();
        g.setColour (on ? theme::blue : juce::Colour (0xff2a343c));
        g.fillEllipse (b);
        g.setColour (on ? juce::Colours::white : theme::sub);
        const float cx = b.getCentreX(), cy = b.getCentreY(), r = b.getWidth() * 0.27f;
        juce::Path p;
        p.addCentredArc (cx, cy, r, r, 0.f, 0.6f, juce::MathConstants<float>::twoPi - 0.6f, true);
        p.startNewSubPath (cx, cy - r * 1.15f);
        p.lineTo (cx, cy - r * 0.1f);
        g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
};

//==============================================================================
// Perilla con tarjeta, titulo, valor grande y subtitulo (como "60.3 AMOUNT")
class ParamKnob : public juce::Component
{
public:
    ParamKnob (juce::AudioProcessorValueTreeState& a, const char* pid, const juce::String& t, const juce::String& s,
               int dec = 1, const juce::String& suf = {}, bool compactMode = false)
        : title (t), sub (s), suffix (suf), decimals (dec), compact (compactMode)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (slider);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, pid, slider);
    }

    void paint (juce::Graphics& g) override
    {
        if (! compact)
        {
            auto b = getLocalBounds().toFloat();
            g.setColour (theme::card);
            g.fillRoundedRectangle (b, 10.f);
            g.setColour (theme::edge);
            g.drawRoundedRectangle (b.reduced (0.5f), 10.f, 1.f);
            g.setColour (theme::sub);
            g.setFont (F (11.f, true));
            g.drawText (title, getLocalBounds().removeFromTop (28), juce::Justification::centred);
        }
        g.setColour (theme::text);
        g.setFont (F (compact ? 11.f : 18.f, true));
        g.drawText (juce::String (slider.getValue(), decimals) + suffix, valueArea, juce::Justification::centred);
        g.setColour (theme::sub);
        g.setFont (F (compact ? 9.f : 10.f));
        g.drawText (sub, subArea, juce::Justification::centred);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced (compact ? 0 : 8);
        if (! compact) b.removeFromTop (22);
        auto bottom = b.removeFromBottom (compact ? 28 : 46);
        valueArea = bottom.removeFromTop (compact ? 16 : 28);
        subArea = bottom;
        slider.setBounds (b);
    }

private:
    juce::String title, sub, suffix;
    int decimals;
    bool compact;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    juce::Rectangle<int> valueArea, subArea;
};

//==============================================================================
// Slider horizontal chico con titulo ("CORTE GRAVES")
class MiniSlider : public juce::Component
{
public:
    MiniSlider (juce::AudioProcessorValueTreeState& a, const char* pid, const juce::String& cap) : caption (cap.toUpperCase())
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (slider);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, pid, slider);
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::sub);
        g.setFont (F (9.f, true));
        g.drawText (caption, getLocalBounds().removeFromTop (13), juce::Justification::centred);
    }
    void resized() override { slider.setBounds (getLocalBounds().withTrimmedTop (13)); }

private:
    juce::String caption;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

//==============================================================================
// Selector segmentado (Plate | Room | Large)
class SegmentedControl : public juce::Component
{
public:
    juce::StringArray labels;
    std::function<void (int)> onChange;
    float fontSize = 11.f;

    void setSelected (int i)
    {
        if (i != selected) { selected = i; repaint(); }
    }
    int getSelected() const { return selected; }

    void paint (juce::Graphics& g) override
    {
        if (labels.isEmpty()) return;
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff0d1318));
        g.fillRoundedRectangle (b, 6.f);
        g.setColour (theme::edge);
        g.drawRoundedRectangle (b.reduced (0.5f), 6.f, 1.f);
        const float w = b.getWidth() / (float) labels.size();
        g.setFont (F (fontSize, true));
        for (int i = 0; i < labels.size(); ++i)
        {
            auto r = juce::Rectangle<float> (b.getX() + (float) i * w, b.getY(), w, b.getHeight()).reduced (2.f);
            if (i == selected)
            {
                g.setColour (theme::orange);
                g.fillRoundedRectangle (r, 4.f);
                g.setColour (theme::dark);
            }
            else g.setColour (theme::sub);
            g.drawText (labels[i], r, juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (labels.isEmpty()) return;
        const int i = juce::jlimit (0, labels.size() - 1, (int) (e.position.x / ((float) getWidth() / (float) labels.size())));
        if (onChange) onChange (i); else setSelected (i);
    }

private:
    int selected = 0;
};

// Selector ligado a un parametro de tipo Choice
class ChoiceSelector : public SegmentedControl
{
public:
    ChoiceSelector (juce::AudioProcessorValueTreeState& a, const char* pid, const juce::StringArray& items, float font = 11.f)
    {
        labels = items;
        fontSize = font;
        att = std::make_unique<juce::ParameterAttachment> (*a.getParameter (pid),
                                                           [this] (float v) { setSelected ((int) std::lround (v)); },
                                                           a.undoManager);
        att->sendInitialUpdate();
        onChange = [this] (int i) { att->setValueAsCompleteGesture ((float) i); };
    }

private:
    std::unique_ptr<juce::ParameterAttachment> att;
};

//==============================================================================
class LevelMeter : public juce::Component
{
public:
    explicit LevelMeter (std::atomic<float>& l) : level (l) {}

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff0a0f13));
        g.fillRoundedRectangle (b, 4.f);
        constexpr int segs = 28;
        const float segH = (b.getHeight() - 8.f) / (float) segs, db = level.load();
        for (int i = 0; i < segs; ++i)
        {
            const float segDb = -60.f + (float) i * (60.f / (float) segs);
            const bool lit = db > segDb;
            juce::Colour c = segDb > -3.f ? juce::Colour (0xffe5322d) : (segDb > -14.f ? theme::orange : juce::Colour (0xffffb347));
            g.setColour (lit ? c : juce::Colour (0xff1a2229));
            g.fillRoundedRectangle (b.getX() + 4.f, b.getBottom() - 4.f - (float) (i + 1) * segH + 1.f, b.getWidth() - 8.f, segH - 2.f, 1.5f);
        }
    }

private:
    std::atomic<float>& level;
};

// Columna de entrada/salida: medidor + lectura en dB + perilla de ganancia
class IOStrip : public juce::Component, private juce::Timer
{
public:
    IOStrip (juce::AudioProcessorValueTreeState& a, const char* pid, const juce::String& t, std::atomic<float>& lvl)
        : title (t), level (lvl), meter (lvl), knob (a, pid, "", "dB", 1, "", true)
    {
        addAndMakeVisible (meter);
        addAndMakeVisible (knob);
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::sub);
        g.setFont (F (11.f, true));
        g.drawText (title, getLocalBounds().removeFromTop (22), juce::Justification::centred);
        g.setColour (theme::dark);
        g.fillRoundedRectangle (readout.toFloat(), 4.f);
        g.setColour (theme::edge);
        g.drawRoundedRectangle (readout.toFloat().reduced (0.5f), 4.f, 1.f);
        g.setColour (theme::text);
        g.setFont (F (12.f, true));
        const float db = level.load();
        g.drawText (db <= -99.f ? juce::String ("-inf") : juce::String (db, 1), readout, juce::Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromTop (24);
        knob.setBounds (area.removeFromBottom (76).reduced (6, 0));
        readout = area.removeFromBottom (30).withSizeKeepingCentre (52, 22);
        meter.setBounds (area.withSizeKeepingCentre (26, area.getHeight() - 6));
    }

    void timerCallback() override { meter.repaint(); repaint (readout); }

private:
    juce::String title;
    std::atomic<float>& level;
    LevelMeter meter;
    ParamKnob knob;
    juce::Rectangle<int> readout;
};
}

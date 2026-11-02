#pragma once
#include "PluginProcessor.h"
#include "UI.h"

namespace nv
{
//==============================================================================
// Base de las ventanas de cada seccion: titulo, boton on/off, perillas y selectores
class SectionPanel : public juce::Component, private juce::Timer
{
public:
    SectionPanel (NovaVocalAudioProcessor& p, const juce::String& t, const char* onId)
        : proc (p), title (t), powerAtt (p.apvts, onId, powerBtn)
    {
        addAndMakeVisible (powerBtn);
    }

    void addKnob (const char* pid, const juce::String& t, const juce::String& s, int dec, const juce::String& suf)
    {
        knobs.push_back (std::make_unique<ParamKnob> (proc.apvts, pid, t, s, dec, suf));
        addAndMakeVisible (*knobs.back());
    }

    void addChoice (const char* pid, const juce::StringArray& items, const juce::String& caption, float font = 11.f)
    {
        auto& c = choices.emplace_back();
        c.sel = std::make_unique<ChoiceSelector> (proc.apvts, pid, items, font);
        c.caption = caption;
        c.weight = items.size() + 1;
        addAndMakeVisible (*c.sel);
    }

    void paint (juce::Graphics& g) override
    {
        g.setGradientFill (juce::ColourGradient (theme::bgTop, 0.f, 0.f, theme::bgBot, 0.f, (float) getHeight(), false));
        g.fillAll();
        g.setColour (theme::text);
        g.setFont (F (19.f, true));
        g.drawText (title, 18, 18, getWidth() - 90, 36, juce::Justification::centredLeft);
        g.setColour (theme::sub);
        g.setFont (F (9.5f, true));
        for (auto& c : choices) g.drawText (c.caption.toUpperCase(), c.capArea, juce::Justification::centredLeft);
        if (extraH > 0) paintExtra (g, extraArea);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced (18);
        auto head = b.removeFromTop (36);
        powerBtn.setBounds (head.removeFromRight (30).withSizeKeepingCentre (28, 28));
        b.removeFromTop (6);
        if (extraH > 0)
        {
            extraArea = b.removeFromTop (extraH);
            b.removeFromTop (12);
        }
        if (! choices.empty())
        {
            auto row = b.removeFromBottom (50);
            b.removeFromBottom (12);
            int total = 0;
            for (auto& c : choices) total += c.weight;
            const int rowW = row.getWidth();
            for (auto& c : choices)
            {
                auto cell = row.removeFromLeft (rowW * c.weight / std::max (1, total));
                c.capArea = cell.removeFromTop (16).reduced (6, 0);
                c.sel->setBounds (cell.reduced (6, 0));
            }
        }
        if (! knobs.empty())
        {
            const int gap = 8, count = (int) knobs.size();
            const int kw = std::min (130, (b.getWidth() - gap * (count - 1)) / count);
            auto row = b.withSizeKeepingCentre (kw * count + gap * (count - 1), b.getHeight());
            for (auto& k : knobs)
            {
                k->setBounds (row.removeFromLeft (kw));
                row.removeFromLeft (gap);
            }
        }
    }

protected:
    void startLive() { startTimerHz (30); }
    virtual void paintExtra (juce::Graphics&, juce::Rectangle<int>) {}
    float P (const char* pid) const { return proc.apvts.getRawParameterValue (pid)->load(); }

    void drawBar (juce::Graphics& g, juce::Rectangle<int> r, float frac, const juce::String& label, const juce::String& value)
    {
        auto rf = r.toFloat();
        g.setColour (theme::dark);
        g.fillRoundedRectangle (rf, 8.f);
        g.setColour (theme::edge);
        g.drawRoundedRectangle (rf.reduced (0.5f), 8.f, 1.f);
        auto bar = rf.reduced (14.f, 0.f).withSizeKeepingCentre (rf.getWidth() - 28.f, 12.f);
        g.setColour (juce::Colour (0xff0a0f13));
        g.fillRoundedRectangle (bar, 6.f);
        g.setColour (theme::orange);
        g.fillRoundedRectangle (bar.withWidth (std::max (6.f, bar.getWidth() * juce::jlimit (0.f, 1.f, frac))), 6.f);
        g.setColour (theme::sub);
        g.setFont (F (9.5f, true));
        g.drawText (label, r.reduced (14, 6).removeFromTop (14), juce::Justification::centredLeft);
        g.setColour (theme::text);
        g.setFont (F (12.f, true));
        g.drawText (value, r.reduced (14, 6).removeFromBottom (16), juce::Justification::centredRight);
    }

    NovaVocalAudioProcessor& proc;
    juce::String title;
    PowerButton powerBtn;
    juce::AudioProcessorValueTreeState::ButtonAttachment powerAtt;
    int extraH = 0;
    juce::Rectangle<int> extraArea;

private:
    void timerCallback() override { repaint (extraArea); }

    struct ChoiceItem
    {
        std::unique_ptr<ChoiceSelector> sel;
        juce::String caption;
        int weight = 1;
        juce::Rectangle<int> capArea;
    };
    std::vector<std::unique_ptr<ParamKnob>> knobs;
    std::vector<ChoiceItem> choices;
};

//==============================================================================
class EqPanel : public SectionPanel
{
public:
    explicit EqPanel (NovaVocalAudioProcessor& p) : SectionPanel (p, "EQ", ids::eqOn)
    {
        addKnob (ids::eqHp, "HP", "Hz", 0, " Hz");
        addKnob (ids::eqLow, "LOW", "SHELF 120 Hz", 1, " dB");
        addKnob (ids::eqMidF, "MID F", "Hz", 0, " Hz");
        addKnob (ids::eqMidG, "MID G", "dB", 1, " dB");
        addKnob (ids::eqMidQ, "MID Q", "Q", 2, "");
        addKnob (ids::eqPresF, "PRES F", "Hz", 0, " Hz");
        addKnob (ids::eqPresG, "PRES G", "dB", 1, " dB");
        addKnob (ids::eqPresQ, "PRES Q", "Q", 2, "");
        addKnob (ids::eqAir, "AIR", "SHELF 10 kHz", 1, " dB");
        extraH = 150;
        startLive();
        setSize (860, 410);
    }

protected:
    void paintExtra (juce::Graphics& g, juce::Rectangle<int> r) override
    {
        model.update (48000.0, P (ids::eqHp), P (ids::eqLow), P (ids::eqMidF), P (ids::eqMidG), P (ids::eqMidQ),
                      P (ids::eqPresF), P (ids::eqPresG), P (ids::eqPresQ), P (ids::eqAir));
        auto rf = r.toFloat();
        g.setColour (theme::dark);
        g.fillRoundedRectangle (rf, 8.f);
        g.setColour (theme::edge);
        g.drawRoundedRectangle (rf.reduced (0.5f), 8.f, 1.f);
        auto a = rf.reduced (12.f, 10.f);
        auto xOf = [&] (double f) { return a.getX() + (float) (std::log (f / 20.0) / std::log (1000.0)) * a.getWidth(); };
        auto yOf = [&] (double db) { return a.getCentreY() - (float) juce::jlimit (-15.0, 15.0, db) / 15.f * a.getHeight() * 0.5f; };

        g.setFont (F (9.f));
        for (double db : { -12.0, -6.0, 0.0, 6.0, 12.0 })
        {
            g.setColour (db == 0.0 ? theme::edge.brighter (0.3f) : theme::edge.withAlpha (0.5f));
            g.drawHorizontalLine ((int) yOf (db), a.getX(), a.getRight());
            g.setColour (theme::sub);
            g.drawText (juce::String ((int) db), (int) a.getX() - 2, (int) yOf (db) - 6, 22, 12, juce::Justification::left);
        }
        for (double f : { 100.0, 1000.0, 10000.0 })
        {
            g.setColour (theme::edge.withAlpha (0.5f));
            g.drawVerticalLine ((int) xOf (f), a.getY(), a.getBottom());
            g.setColour (theme::sub);
            g.drawText (f >= 1000.0 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f),
                        (int) xOf (f) + 3, (int) a.getBottom() - 12, 30, 12, juce::Justification::left);
        }

        juce::Path path;
        for (int i = 0; i <= 240; ++i)
        {
            const double f = 20.0 * std::pow (1000.0, (double) i / 240.0);
            const float x = xOf (f), y = yOf (model.responseDb (f));
            if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
        }
        g.setColour (theme::orange);
        g.strokePath (path, juce::PathStrokeType (2.2f));
    }

private:
    EQ model;
};

//==============================================================================
class CompPanel : public SectionPanel
{
public:
    explicit CompPanel (NovaVocalAudioProcessor& p) : SectionPanel (p, "Compresor", ids::compOn)
    {
        addKnob (ids::compAmount, "AMOUNT", "%", 1, "");
        addKnob (ids::compThresh, "THRESH", "dB", 1, " dB");
        addKnob (ids::compRatio, "RATIO", "ratio", 1, ":1");
        addKnob (ids::compAttack, "ATTACK", "ms", 1, " ms");
        addKnob (ids::compRelease, "RELEASE", "ms", 0, " ms");
        addKnob (ids::compMakeup, "MAKEUP", "dB", 1, " dB");
        extraH = 56;
        startLive();
        setSize (700, 330);
    }

protected:
    void paintExtra (juce::Graphics& g, juce::Rectangle<int> r) override
    {
        const float gr = proc.compGr.load();
        drawBar (g, r, -gr / 20.f, "GAIN REDUCTION", juce::String (gr, 1) + " dB");
    }
};

class DeEssPanel : public SectionPanel
{
public:
    explicit DeEssPanel (NovaVocalAudioProcessor& p) : SectionPanel (p, "De-esser", ids::deessOn)
    {
        addKnob (ids::deessAmount, "AMOUNT", "%", 1, "");
        addKnob (ids::deessFreq, "FREQ", "Hz", 0, " Hz");
        addKnob (ids::deessThresh, "THRESH", "dB", 1, " dB");
        extraH = 56;
        startLive();
        setSize (520, 330);
    }

protected:
    void paintExtra (juce::Graphics& g, juce::Rectangle<int> r) override
    {
        const float red = proc.deessGr.load();
        drawBar (g, r, red / 15.f, U ("REDUCCIÓN DE SIBILANCIA"), juce::String (-red, 1) + " dB");
    }
};

class DelayPanel : public SectionPanel
{
public:
    explicit DelayPanel (NovaVocalAudioProcessor& p) : SectionPanel (p, "Delay", ids::delayOn)
    {
        addKnob (ids::delayAmount, "AMOUNT", "%", 1, "");
        addKnob (ids::delayFb, "FEEDBACK", "%", 0, " %");
        addKnob (ids::delayLow, "CORTE GRAVES", "Hz", 0, " Hz");
        addKnob (ids::delayHigh, "CORTE AGUDOS", "Hz", 0, " Hz");
        addChoice (ids::delayDiv, SA ({ "1/16", "1/8", "1/4" }), U ("Sincronía con el tempo"));
        setSize (600, 320);
    }
};

class ReverbPanel : public SectionPanel
{
public:
    explicit ReverbPanel (NovaVocalAudioProcessor& p) : SectionPanel (p, "Reverb", ids::reverbOn)
    {
        addKnob (ids::reverbAmount, "AMOUNT", "%", 1, "");
        addKnob (ids::reverbDecay, "DECAY", "%", 0, " %");
        addKnob (ids::reverbPre, "PRE-DELAY", "ms", 0, " ms");
        addKnob (ids::reverbLow, "CORTE GRAVES", "Hz", 0, " Hz");
        addKnob (ids::reverbHigh, "CORTE AGUDOS", "Hz", 0, " Hz");
        addChoice (ids::reverbType, SA ({ "Plate", "Room", "Large" }), "Tipo");
        setSize (680, 320);
    }
};

//==============================================================================
class TunePanel : public SectionPanel
{
public:
    explicit TunePanel (NovaVocalAudioProcessor& p) : SectionPanel (p, "Autotune", ids::tuneOn)
    {
        addKnob (ids::tuneSpeed, "VELOCIDAD", "ms", 0, " ms");
        addKnob (ids::tuneAmount, "CANTIDAD", "%", 0, " %");
        juce::StringArray keys;
        for (auto* n : kNoteNames) keys.add (n);
        addChoice (ids::tuneKey, keys, "Tonalidad", 10.f);
        addChoice (ids::tuneScale, SA ({ U ("Cromática"), "Mayor", "Menor" }), "Escala");
        addChoice (ids::mode, SA ({ "Studio", "Live" }), "Modo");
        extraH = 130;
        startLive();
        setSize (700, 450);
    }

protected:
    void paintExtra (juce::Graphics& g, juce::Rectangle<int> r) override
    {
        const int key = (int) P (ids::tuneKey), scale = (int) P (ids::tuneScale);
        const float det = proc.tune.detMidi.load(), tgt = proc.tune.tgtMidi.load();
        const int detPc = det >= 0.f ? ((int) std::lround (det) % 12 + 12) % 12 : -1;
        const int tgtPc = tgt >= 0.f ? ((int) std::lround (tgt) % 12 + 12) % 12 : -1;

        auto rf = r.toFloat();
        g.setColour (theme::dark);
        g.fillRoundedRectangle (rf, 8.f);
        g.setColour (theme::edge);
        g.drawRoundedRectangle (rf.reduced (0.5f), 8.f, 1.f);

        auto area = rf.reduced (14.f, 12.f);
        auto kb = area.removeFromLeft (area.getWidth() * 0.62f);
        const float wk = kb.getWidth() / 7.f;
        static const int whitePc[7] = { 0, 2, 4, 5, 7, 9, 11 };
        auto colourFor = [&] (int pc, juce::Colour inS, juce::Colour outS)
        {
            if (pc == tgtPc) return theme::orange;
            if (pc == detPc) return theme::orange.withAlpha (0.55f);
            return inScale (pc, key, scale) ? inS : outS;
        };
        for (int i = 0; i < 7; ++i)
        {
            auto kr = juce::Rectangle<float> (kb.getX() + (float) i * wk, kb.getY(), wk - 2.f, kb.getHeight());
            g.setColour (colourFor (whitePc[i], juce::Colour (0xffdfe6ea), juce::Colour (0xff6d7882)));
            g.fillRoundedRectangle (kr, 3.f);
            g.setColour (theme::dark);
            g.setFont (F (9.f, true));
            g.drawText (kNoteNames[whitePc[i]], kr.withTrimmedTop (kr.getHeight() - 16.f).toNearestInt(), juce::Justification::centred);
        }
        static const int blackPc[5] = { 1, 3, 6, 8, 10 }, blackAfter[5] = { 0, 1, 3, 4, 5 };
        for (int i = 0; i < 5; ++i)
        {
            auto kr = juce::Rectangle<float> (kb.getX() + (float) (blackAfter[i] + 1) * wk - wk * 0.32f, kb.getY(), wk * 0.6f, kb.getHeight() * 0.6f);
            g.setColour (colourFor (blackPc[i], juce::Colour (0xff3b4853), juce::Colour (0xff0a0e11)));
            g.fillRoundedRectangle (kr, 3.f);
        }

        auto tr = area.withTrimmedLeft (18.f).toNearestInt();
        auto noteName = [] (float m)
        {
            const int mi = (int) std::lround (m);
            return juce::String (kNoteNames[((mi % 12) + 12) % 12]) + juce::String (mi / 12 - 1);
        };
        g.setColour (theme::sub);
        g.setFont (F (9.5f, true));
        g.drawText ("DETECTADA", tr.removeFromTop (14), juce::Justification::centredLeft);
        g.setColour (theme::text);
        g.setFont (F (22.f, true));
        g.drawText (det >= 0.f ? noteName (det) : juce::String ("--"), tr.removeFromTop (30), juce::Justification::centredLeft);
        g.setColour (theme::sub);
        g.setFont (F (9.5f, true));
        g.drawText ("CORREGIDA A", tr.removeFromTop (14), juce::Justification::centredLeft);
        g.setColour (theme::orange);
        g.setFont (F (22.f, true));
        const int cents = det >= 0.f ? (int) std::lround ((tgt - det) * 100.f) : 0;
        g.drawText (tgt >= 0.f ? noteName (tgt) + "  (" + (cents > 0 ? "+" : "") + juce::String (cents) + juce::String::fromUTF8 (" \xC2\xA2)") : juce::String ("--"),
                    tr.removeFromTop (30), juce::Justification::centredLeft);
    }
};

//==============================================================================
class SectionWindow : public juce::DocumentWindow
{
public:
    SectionWindow (const juce::String& name, std::unique_ptr<SectionPanel> content, juce::LookAndFeel* lf, juce::Point<int> pos)
        : juce::DocumentWindow (name, theme::bgBot, juce::DocumentWindow::closeButton)
    {
        setLookAndFeel (lf);
        setUsingNativeTitleBar (true);
        setContentOwned (content.release(), true);
        setResizable (false, false);
        setTopLeftPosition (pos);
        setAlwaysOnTop (true);
        setVisible (true);
    }
    ~SectionWindow() override { setLookAndFeel (nullptr); }
    void closeButtonPressed() override { setVisible (false); }
};

//==============================================================================
// Panel inferior de la pagina Simple: selector + sliders chicos
class FxPanel : public juce::Component
{
public:
    FxPanel (juce::AudioProcessorValueTreeState& a, const char* choiceId, const juce::StringArray& items)
        : sel (a, choiceId, items)
    {
        addAndMakeVisible (sel);
    }
    void addSlider (juce::AudioProcessorValueTreeState& a, const char* pid, const juce::String& cap)
    {
        sliders.push_back (std::make_unique<MiniSlider> (a, pid, cap));
        addAndMakeVisible (*sliders.back());
    }
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (theme::card);
        g.fillRoundedRectangle (b, 10.f);
        g.setColour (theme::edge);
        g.drawRoundedRectangle (b.reduced (0.5f), 10.f, 1.f);
    }
    void resized() override
    {
        auto b = getLocalBounds().reduced (12);
        sel.setBounds (b.removeFromTop (28));
        b.removeFromTop (12);
        if (sliders.empty()) return;
        const int gap = 12, n = (int) sliders.size(), w = (b.getWidth() - gap * (n - 1)) / n;
        for (auto& s : sliders)
        {
            s->setBounds (b.removeFromLeft (w).withHeight (36));
            b.removeFromLeft (gap);
        }
    }

private:
    ChoiceSelector sel;
    std::vector<std::unique_ptr<MiniSlider>> sliders;
};

class SimplePage : public juce::Component
{
public:
    explicit SimplePage (NovaVocalAudioProcessor& p)
        : compK (p.apvts, ids::compAmount, "COMP", "AMOUNT"),
          magicK (p.apvts, ids::magicAmount, "MAGIC", "AMOUNT"),
          reverbK (p.apvts, ids::reverbAmount, "REVERB", "AMOUNT"),
          delayK (p.apvts, ids::delayAmount, "DELAY", "AMOUNT"),
          magicP (p.apvts, ids::magicChar, SA ({ "Warm", "Agudo", U ("Saturación") })),
          reverbP (p.apvts, ids::reverbType, SA ({ "Plate", "Room", "Large" })),
          delayP (p.apvts, ids::delayDiv, SA ({ "1/16", "1/8", "1/4" }))
    {
        magicP.addSlider (p.apvts, ids::magicIntensity, "Intensidad EFX");
        reverbP.addSlider (p.apvts, ids::reverbLow, "Corte graves");
        reverbP.addSlider (p.apvts, ids::reverbHigh, "Corte agudos");
        delayP.addSlider (p.apvts, ids::delayLow, "Corte graves");
        delayP.addSlider (p.apvts, ids::delayHigh, "Corte agudos");
        for (auto* c : { (juce::Component*) &compK, (juce::Component*) &magicK, (juce::Component*) &reverbK, (juce::Component*) &delayK,
                         (juce::Component*) &magicP, (juce::Component*) &reverbP, (juce::Component*) &delayP })
            addAndMakeVisible (c);
    }

    void resized() override
    {
        const int gap = 10, knobH = 232, panelH = 106;
        auto b = getLocalBounds().withSizeKeepingCentre (getWidth(), std::min (getHeight(), knobH + 14 + panelH));
        auto bottom = b.removeFromBottom (panelH);
        b.removeFromBottom (14);
        const int kw = (b.getWidth() - gap * 3) / 4;
        for (auto* k : { &compK, &magicK, &reverbK, &delayK })
        {
            k->setBounds (b.removeFromLeft (kw));
            b.removeFromLeft (gap);
        }
        const int pw = (bottom.getWidth() - gap * 2) / 3;
        for (auto* p : { &magicP, &reverbP, &delayP })
        {
            p->setBounds (bottom.removeFromLeft (pw));
            bottom.removeFromLeft (gap);
        }
    }

private:
    ParamKnob compK, magicK, reverbK, delayK;
    FxPanel magicP, reverbP, delayP;
};

//==============================================================================
class SectionTile : public juce::Component
{
public:
    SectionTile (juce::AudioProcessorValueTreeState& a, const char* onId, const juce::String& t, const juce::String& d,
                 std::function<void()> open)
        : title (t), desc (d), powerAtt (a, onId, powerBtn)
    {
        addAndMakeVisible (powerBtn);
        addAndMakeVisible (openBtn);
        openBtn.setButtonText ("ABRIR VENTANA");
        openBtn.onClick = std::move (open);
    }
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (theme::card);
        g.fillRoundedRectangle (b, 10.f);
        g.setColour (theme::edge);
        g.drawRoundedRectangle (b.reduced (0.5f), 10.f, 1.f);
        g.setColour (theme::text);
        g.setFont (F (16.f, true));
        g.drawText (title, 16, 14, getWidth() - 70, 24, juce::Justification::centredLeft);
        g.setColour (theme::sub);
        g.setFont (F (11.f));
        g.drawFittedText (desc, 16, 46, getWidth() - 32, 44, juce::Justification::topLeft, 3);
    }
    void resized() override
    {
        powerBtn.setBounds (getWidth() - 46, 12, 30, 30);
        openBtn.setBounds (14, getHeight() - 42, getWidth() - 28, 28);
    }

private:
    juce::String title, desc;
    PowerButton powerBtn;
    juce::AudioProcessorValueTreeState::ButtonAttachment powerAtt;
    juce::TextButton openBtn;
};

class AdvancedPage : public juce::Component
{
public:
    AdvancedPage (NovaVocalAudioProcessor& p, std::function<void (int)> openFn)
    {
        struct Info { const char* on; const char* title; const char* desc; };
        const Info infos[6] = {
            { ids::eqOn, "EQ", "Filtro de graves, dos bandas paramétricas y brillo (air)." },
            { ids::compOn, "Compresor", "Control de dinamica: umbral, ratio, ataque y release." },
            { ids::deessOn, "De-esser", "Reduce la sibilancia (S, T, CH) sin apagar la voz." },
            { ids::delayOn, "Delay", "Eco sincronizado al tempo, con corte de graves y agudos." },
            { ids::reverbOn, "Reverb", "Plate, Room o Large con pre-delay y filtros." },
            { ids::tuneOn, "Autotune", "Corrección de tono según tonalidad y escala." } };
        for (int i = 0; i < 6; ++i)
        {
            tiles.push_back (std::make_unique<SectionTile> (p.apvts, infos[i].on, infos[i].title, U (infos[i].desc),
                                                            [openFn, i] { openFn (i); }));
            addAndMakeVisible (*tiles.back());
        }
    }
    void resized() override
    {
        const int gap = 12, cols = 3, rows = 2;
        const int w = (getWidth() - gap * (cols - 1)) / cols, h = 150;
        const int top = std::max (0, (getHeight() - (h * rows + gap * (rows - 1))) / 2);
        for (int i = 0; i < 6; ++i)
            tiles[(size_t) i]->setBounds ((i % cols) * (w + gap), top + (i / cols) * (h + gap), w, h);
    }

private:
    std::vector<std::unique_ptr<SectionTile>> tiles;
};

//==============================================================================
// Pagina "Vocal Assist": escucha la voz, la analiza y propone la mezcla
class AssistPage : public juce::Component, private juce::Timer
{
public:
    explicit AssistPage (NovaVocalAudioProcessor& p)
        : proc (p), styleSel (p.apvts, ids::assistStyle, SA ({ "Natural", "Rap/Trap", "Pop" }))
    {
        loadBtn.setButtonText ("Cargar");
        applyBtn.setButtonText ("Aplicar");
        resetBtn.setButtonText ("Reiniciar");
        applyBtn.setEnabled (false);
        for (auto* c : { (juce::Component*) &loadBtn, (juce::Component*) &applyBtn, (juce::Component*) &resetBtn, (juce::Component*) &styleSel })
            addAndMakeVisible (c);

        loadBtn.onClick = [this]
        {
            chooser = std::make_unique<juce::FileChooser> (U ("Elegí una toma de voz"), juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.m4a");
            juce::Component::SafePointer<AssistPage> safe (this);
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [safe] (const juce::FileChooser& fc)
                                  {
                                      const auto f = fc.getResult();
                                      if (safe != nullptr && f.existsAsFile()) safe->proc.assist.loadFile (f, safe->style());
                                  });
        };
        applyBtn.onClick = [this] { proc.applyAssistResult(); };
        resetBtn.onClick = [this] { proc.assist.reset(); };
        startTimerHz (30);
    }

    void resized() override
    {
        centre = { (float) getWidth() * 0.5f, 150.f };
        const int bw = 112, gap = 12, total = bw * 3 + gap * 2;
        juce::Rectangle<int> row ((getWidth() - total) / 2, 326, total, 30);
        loadBtn.setBounds (row.removeFromLeft (bw));  row.removeFromLeft (gap);
        applyBtn.setBounds (row.removeFromLeft (bw)); row.removeFromLeft (gap);
        resetBtn.setBounds (row.removeFromLeft (bw));
        styleSel.setBounds ((getWidth() - 280) / 2, 378, 280, 28);
    }

    void paint (juce::Graphics& g) override
    {
        using S = VocalAssist;
        const auto st = proc.assist.getState();
        const double now = juce::Time::getMillisecondCounterHiRes();
        const float lvl = juce::jlimit (0.f, 1.f, (proc.inLevel.load() + 50.f) / 50.f);
        const float progress = juce::jlimit (0.f, 1.f, proc.assist.seconds() / S::kMaxSeconds);

        g.setColour (theme::sub);
        g.setFont (F (12.f));
        g.drawText (U ("Reproducí una toma de voz sola y pulsá Escuchar"), 0, 6, getWidth(), 20, juce::Justification::centred);

        // anillo de marcas
        constexpr int ticks = 60;
        for (int i = 0; i < ticks; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / (float) ticks;
            float alpha = 0.3f;
            if (st == S::Listening) alpha = i <= (int) (progress * ticks) ? 0.55f + 0.45f * lvl : 0.f;
            else if (st == S::Analyzing)
            {
                const int head = (int) (now / 25.0) % ticks, dist = (head - i + ticks) % ticks;
                alpha = dist < 18 ? 1.f - (float) dist / 18.f : 0.f;
            }
            else if (st == S::Done) alpha = 1.f;
            const juce::Point<float> dir (std::sin (a), -std::cos (a));
            g.setColour (alpha > 0.f ? theme::orange.withAlpha (std::max (0.18f, alpha)) : juce::Colour (0xff2a3640));
            g.drawLine (juce::Line<float> (centre + dir * 84.f, centre + dir * 102.f), 3.f);
        }

        // boton central
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a4853), centre.x, centre.y - 78.f,
                                                 juce::Colour (0xff131a20), centre.x, centre.y + 78.f, false));
        g.fillEllipse (centre.x - 76.f, centre.y - 76.f, 152.f, 152.f);
        g.setColour (juce::Colour (0xff070a0d));
        g.drawEllipse (centre.x - 76.f, centre.y - 76.f, 152.f, 152.f, 2.f);

        // iconito de ecualizador
        for (int i = 0; i < 7; ++i)
        {
            const float act = st == S::Listening ? lvl : (st == S::Analyzing ? 0.6f : 0.25f);
            const float h = 6.f + 22.f * act * (0.35f + 0.65f * std::abs (std::sin ((float) i * 0.9f + (float) now / 170.f)));
            g.setColour (theme::orange);
            g.fillRoundedRectangle (centre.x - 27.f + (float) i * 9.f, centre.y - 22.f - h * 0.5f, 5.f, h, 2.5f);
        }

        juce::String label = U ("Escuchar"), timeTxt = juce::String ((int) proc.assist.seconds()) + " s";
        if (st == S::Listening) label = "Detener";
        else if (st == S::Analyzing) { label = U ("Analizando…"); timeTxt = ""; }
        else if (st == S::Done) timeTxt = "Listo";
        g.setColour (theme::text);
        g.setFont (F (16.f, true));
        g.drawText (label, juce::Rectangle<int> ((int) centre.x - 70, (int) centre.y + 6, 140, 22), juce::Justification::centred);
        g.setColour (theme::sub);
        g.setFont (F (12.f));
        g.drawText (timeTxt, juce::Rectangle<int> ((int) centre.x - 70, (int) centre.y + 30, 140, 18), juce::Justification::centred);

        // resumen del analisis
        g.setColour (theme::text.withAlpha (0.9f));
        g.setFont (F (12.f));
        g.drawFittedText (result.summary, 24, 266, getWidth() - 48, 48, juce::Justification::centred, 3);

        g.setColour (theme::sub);
        g.setFont (F (9.5f, true));
        g.drawText ("ESTILO", 0, 362, getWidth(), 14, juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.position.getDistanceFrom (centre) > 78.f) return;
        const auto st = proc.assist.getState();
        if (st == VocalAssist::Listening) proc.assist.stopListening (style());
        else if (st != VocalAssist::Analyzing) proc.assist.startListening();
    }

private:
    int style() const { return (int) proc.apvts.getRawParameterValue (ids::assistStyle)->load(); }

    void timerCallback() override
    {
        if (proc.assist.autoStopDue()) proc.assist.stopListening (style());
        const auto st = proc.assist.getState();
        if (st != lastState)
        {
            lastState = st;
            result = st == VocalAssist::Done ? proc.assist.getResult() : VocalAssist::Result();
            applyBtn.setEnabled (st == VocalAssist::Done && result.valid);
        }
        repaint();
    }

    NovaVocalAudioProcessor& proc;
    ChoiceSelector styleSel;
    juce::TextButton loadBtn, applyBtn, resetBtn;
    std::unique_ptr<juce::FileChooser> chooser;
    VocalAssist::State lastState = VocalAssist::Idle;
    VocalAssist::Result result;
    juce::Point<float> centre { 294.f, 150.f };
};
}

#include "PluginEditor.h"

using namespace nv;

NovaVocalAudioProcessorEditor::NovaVocalAudioProcessorEditor (NovaVocalAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      powerAtt (p.apvts, ids::power, powerBtn),
      modeSel (p.apvts, ids::mode, SA ({ "STUDIO", "LIVE" })),
      inStrip (p.apvts, ids::inTrim, "INPUT", p.inLevel),
      outStrip (p.apvts, ids::outGain, "OUTPUT", p.outLevel),
      assistPage (p), simplePage (p),
      advancedPage (p, [this] (int i) { openSection (i); })
{
    setLookAndFeel (&lnf);

    tuneBtn.setClickingTogglesState (true);
    tuneAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, ids::tuneOn, tuneBtn);

    tabs.labels = SA ({ "VOCAL ASSIST", "SIMPLE", "ADVANCED" });
    tabs.fontSize = 10.5f;
    tabs.onChange = [this] (int i) { showPage (i); };

    presetBox.onChange = [this]
    {
        if (presetBox.getSelectedId() >= 2) proc.loadPreset (presetBox.getText());
    };
    saveBtn.onClick = [this] { askPresetName(); };
    abBtn.onClick = [this]
    {
        proc.toggleAB();
        abBtn.setButtonText (proc.isSlotA() ? "A" : "B");
    };
    undoBtn.onClick = [this] { proc.undoLast(); };

    for (auto* c : { (juce::Component*) &powerBtn, (juce::Component*) &tuneBtn, (juce::Component*) &modeSel, (juce::Component*) &tabs,
                     (juce::Component*) &presetBox, (juce::Component*) &saveBtn, (juce::Component*) &abBtn, (juce::Component*) &undoBtn,
                     (juce::Component*) &inStrip, (juce::Component*) &outStrip })
        addAndMakeVisible (c);
    addChildComponent (assistPage);
    addChildComponent (simplePage);
    addChildComponent (advancedPage);

    refreshPresets();
    showPage (1);
    setResizable (false, false);
    setSize (780, 540);
}

NovaVocalAudioProcessorEditor::~NovaVocalAudioProcessorEditor()
{
    for (auto& w : windows) w.reset();
    setLookAndFeel (nullptr);
}

void NovaVocalAudioProcessorEditor::showPage (int index)
{
    tabs.setSelected (index);
    assistPage.setVisible (index == 0);
    simplePage.setVisible (index == 1);
    advancedPage.setVisible (index == 2);
}

void NovaVocalAudioProcessorEditor::refreshPresets()
{
    presetBox.clear (juce::dontSendNotification);
    presetBox.addItem ("Manual", 1);
    int idx = 2;
    for (auto& n : proc.listPresets()) presetBox.addItem (n, idx++);
    presetBox.setSelectedId (1, juce::dontSendNotification);
}

void NovaVocalAudioProcessorEditor::askPresetName()
{
    auto* aw = new juce::AlertWindow (U ("Guardar preset"), U ("Nombre del preset:"), juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor ("name", presetBox.getSelectedId() >= 2 ? presetBox.getText() : juce::String());
    aw->addButton ("Guardar", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancelar", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<NovaVocalAudioProcessorEditor> safe (this);
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([safe, aw] (int result)
    {
        if (result != 1 || safe == nullptr) return;
        const auto name = aw->getTextEditorContents ("name");
        if (safe->proc.savePreset (name))
        {
            safe->refreshPresets();
            safe->presetBox.setText (name, juce::dontSendNotification);
        }
    }), true);
}

void NovaVocalAudioProcessorEditor::openSection (int idx)
{
    if (idx < 0 || idx > 5) return;
    if (windows[idx] == nullptr)
    {
        std::unique_ptr<SectionPanel> panel;
        juce::String name = "Nova Vocal - ";
        switch (idx)
        {
            case 0: panel = std::make_unique<EqPanel> (proc);     name << "EQ"; break;
            case 1: panel = std::make_unique<CompPanel> (proc);   name << "Compresor"; break;
            case 2: panel = std::make_unique<DeEssPanel> (proc);  name << "De-esser"; break;
            case 3: panel = std::make_unique<DelayPanel> (proc);  name << "Delay"; break;
            case 4: panel = std::make_unique<ReverbPanel> (proc); name << "Reverb"; break;
            default: panel = std::make_unique<TunePanel> (proc);  name << "Autotune"; break;
        }
        windows[idx] = std::make_unique<SectionWindow> (name, std::move (panel), &lnf,
                                                        getScreenPosition() + juce::Point<int> (60 + idx * 28, 90 + idx * 28));
    }
    windows[idx]->setVisible (true);
    windows[idx]->toFront (true);
}

void NovaVocalAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (theme::bgTop, 0.f, 0.f, theme::bgBot, 0.f, (float) getHeight(), false));
    g.fillAll();

    g.setFont (F (25.f, true));
    g.setColour (theme::text);
    g.drawText ("NOVA", 62, 12, 80, 40, juce::Justification::centredLeft);
    g.setColour (theme::orange);
    g.drawText ("VOCAL", 134, 12, 100, 40, juce::Justification::centredLeft);
    g.setColour (theme::sub);
    g.setFont (F (10.f, true));
    g.drawText ("V1", 232, 22, 26, 14, juce::Justification::centredLeft);

    g.setColour (theme::edge.withAlpha (0.7f));
    g.drawHorizontalLine (62, 0.f, (float) getWidth());
    g.drawHorizontalLine (104, 0.f, (float) getWidth());

    g.setColour (theme::sub.withAlpha (0.7f));
    g.setFont (F (10.f));
    g.drawText ("NOVA VOCAL 1.0", 0, 522, getWidth(), 14, juce::Justification::centred);
}

void NovaVocalAudioProcessorEditor::resized()
{
    powerBtn.setBounds (16, 15, 34, 34);
    tuneBtn.setBounds (268, 18, 64, 28);
    modeSel.setBounds (340, 18, 130, 28);
    tabs.setBounds (getWidth() - 16 - 340, 18, 340, 28);

    presetBox.setBounds (16, 70, 300, 28);
    saveBtn.setBounds (324, 70, 84, 28);
    abBtn.setBounds (416, 70, 48, 28);
    undoBtn.setBounds (472, 70, 84, 28);

    inStrip.setBounds (12, 108, 72, 408);
    outStrip.setBounds (getWidth() - 12 - 72, 108, 72, 408);

    const juce::Rectangle<int> page (96, 108, getWidth() - 192, 412);
    assistPage.setBounds (page);
    simplePage.setBounds (page);
    advancedPage.setBounds (page);
}

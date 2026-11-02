#pragma once
#include "PluginProcessor.h"
#include "Panels.h"

class NovaVocalAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit NovaVocalAudioProcessorEditor (NovaVocalAudioProcessor&);
    ~NovaVocalAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void showPage (int index);
    void openSection (int index);
    void refreshPresets();
    void askPresetName();

    NovaVocalAudioProcessor& proc;
    nv::NovaLnF lnf;   // debe vivir mas que todos los componentes

    nv::PowerButton powerBtn;
    juce::AudioProcessorValueTreeState::ButtonAttachment powerAtt;
    juce::TextButton tuneBtn { "TUNE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tuneAtt;
    nv::ChoiceSelector modeSel;
    nv::SegmentedControl tabs;

    juce::ComboBox presetBox;
    juce::TextButton saveBtn { "Guardar" }, abBtn { "A" }, undoBtn { "Deshacer" };

    nv::IOStrip inStrip, outStrip;
    nv::AssistPage assistPage;
    nv::SimplePage simplePage;
    nv::AdvancedPage advancedPage;

    std::unique_ptr<nv::SectionWindow> windows[6];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NovaVocalAudioProcessorEditor)
};

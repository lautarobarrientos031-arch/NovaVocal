#pragma once
#include "Common.h"
#include "Params.h"
#include "Reverb.h"
#include "VocalAssist.h"

class NovaVocalAudioProcessor : public juce::AudioProcessor
{
public:
    NovaVocalAudioProcessor();
    ~NovaVocalAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Nova Vocal"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- API para la interfaz
    void applyAssistResult();          // aplica los valores calculados por Vocal Assist
    void undoLast();                   // revierte el ultimo Aplicar de Vocal Assist o, si no hubo, el ultimo cambio
    void resetAllParameters();
    juce::File presetDir() const;
    juce::StringArray listPresets() const;
    bool savePreset (const juce::String& name);
    bool loadPreset (const juce::String& name);
    void toggleAB();
    bool isSlotA() const { return slotIsA; }

    // IMPORTANTE: undoManager va antes que apvts (orden de inicializacion)
    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;

    nv::VocalAssist assist;
    nv::AutoTune tune;

    // medidores (los lee la interfaz)
    std::atomic<float> inLevel { -100.f }, outLevel { -100.f }, compGr { 0.f }, deessGr { 0.f };

private:
    void updateMeter (std::atomic<float>& m, float peak, int n);

    double fs = 44100.0;
    nv::EQ eq;
    nv::Compressor comp;
    nv::DeEsser deess;
    nv::Magic magic;
    nv::DelayFx delay;
    nv::ReverbFx reverb;
    std::vector<float> scratch;

    juce::ValueTree slotA, slotB, assistSnapshot;
    bool slotIsA = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NovaVocalAudioProcessor)
};

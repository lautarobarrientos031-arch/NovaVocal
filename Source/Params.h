#pragma once
#include "Common.h"

namespace nv::ids
{
#define NV_ID(name) inline constexpr const char* name = #name;
    NV_ID (power) NV_ID (inTrim) NV_ID (outGain) NV_ID (mode)
    // EQ
    NV_ID (eqOn) NV_ID (eqHp) NV_ID (eqLow) NV_ID (eqMidF) NV_ID (eqMidG) NV_ID (eqMidQ)
    NV_ID (eqPresF) NV_ID (eqPresG) NV_ID (eqPresQ) NV_ID (eqAir)
    // Compresor
    NV_ID (compOn) NV_ID (compAmount) NV_ID (compThresh) NV_ID (compRatio) NV_ID (compAttack) NV_ID (compRelease) NV_ID (compMakeup)
    // Magic
    NV_ID (magicAmount) NV_ID (magicChar) NV_ID (magicIntensity)
    // De-esser
    NV_ID (deessOn) NV_ID (deessAmount) NV_ID (deessFreq) NV_ID (deessThresh)
    // Reverb
    NV_ID (reverbOn) NV_ID (reverbAmount) NV_ID (reverbType) NV_ID (reverbDecay) NV_ID (reverbPre) NV_ID (reverbLow) NV_ID (reverbHigh)
    // Delay
    NV_ID (delayOn) NV_ID (delayAmount) NV_ID (delayDiv) NV_ID (delayFb) NV_ID (delayLow) NV_ID (delayHigh)
    // Autotune
    NV_ID (tuneOn) NV_ID (tuneKey) NV_ID (tuneScale) NV_ID (tuneSpeed) NV_ID (tuneAmount)
    // Vocal Assist
    NV_ID (assistStyle)
#undef NV_ID
}

namespace nv
{
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout L;

    auto F = [&] (const char* pid, const char* name, float lo, float hi, float def, float centre = 0.f, const char* unit = "")
    {
        NormalisableRange<float> r (lo, hi, 0.0f);
        if (centre > 0.f) r.setSkewForCentre (centre);
        L.add (std::make_unique<AudioParameterFloat> (ParameterID { pid, 1 }, name, r, def,
                                                      AudioParameterFloatAttributes().withLabel (unit)));
    };
    auto B = [&] (const char* pid, const char* name, bool def)
    {
        L.add (std::make_unique<AudioParameterBool> (ParameterID { pid, 1 }, name, def));
    };
    auto C = [&] (const char* pid, const char* name, const StringArray& items, int def)
    {
        L.add (std::make_unique<AudioParameterChoice> (ParameterID { pid, 1 }, name, items, def));
    };

    StringArray keys;
    for (auto* n : kNoteNames) keys.add (n);

    B (ids::power, "Power", true);
    F (ids::inTrim, "Input", -24.f, 24.f, 0.f, 0.f, "dB");
    F (ids::outGain, "Output", -24.f, 12.f, 0.f, 0.f, "dB");
    C (ids::mode, "Modo", SA ({ "Studio", "Live" }), 0);

    B (ids::eqOn, "EQ On", true);
    F (ids::eqHp, "EQ HP", 20.f, 300.f, 80.f, 80.f, "Hz");
    F (ids::eqLow, "EQ Low", -12.f, 12.f, 0.f, 0.f, "dB");
    F (ids::eqMidF, "EQ Mid F", 200.f, 1000.f, 350.f, 400.f, "Hz");
    F (ids::eqMidG, "EQ Mid G", -12.f, 12.f, 0.f, 0.f, "dB");
    F (ids::eqMidQ, "EQ Mid Q", 0.3f, 4.f, 1.f, 1.f, "");
    F (ids::eqPresF, "EQ Pres F", 1500.f, 8000.f, 3500.f, 3500.f, "Hz");
    F (ids::eqPresG, "EQ Pres G", -12.f, 12.f, 0.f, 0.f, "dB");
    F (ids::eqPresQ, "EQ Pres Q", 0.3f, 4.f, 1.f, 1.f, "");
    F (ids::eqAir, "EQ Air", -12.f, 12.f, 0.f, 0.f, "dB");

    B (ids::compOn, "Comp On", true);
    F (ids::compAmount, "Comp", 0.f, 100.f, 50.f, 0.f, "%");
    F (ids::compThresh, "Comp Threshold", -50.f, 0.f, -20.f, 0.f, "dB");
    F (ids::compRatio, "Comp Ratio", 1.f, 20.f, 3.f, 4.f, ":1");
    F (ids::compAttack, "Comp Attack", 0.1f, 100.f, 10.f, 10.f, "ms");
    F (ids::compRelease, "Comp Release", 10.f, 1000.f, 120.f, 150.f, "ms");
    F (ids::compMakeup, "Comp Makeup", -12.f, 24.f, 0.f, 0.f, "dB");

    F (ids::magicAmount, "Magic", 0.f, 100.f, 50.f, 0.f, "%");
    C (ids::magicChar, "Magic Caracter", SA ({ "Warm", "Agudo", U ("Saturación") }), 0);
    F (ids::magicIntensity, "Magic Intensidad", 0.f, 100.f, 50.f, 0.f, "%");

    B (ids::deessOn, "De-esser On", true);
    F (ids::deessAmount, "De-esser", 0.f, 100.f, 50.f, 0.f, "%");
    F (ids::deessFreq, "De-esser Freq", 3000.f, 12000.f, 6500.f, 6000.f, "Hz");
    F (ids::deessThresh, "De-esser Threshold", -60.f, 0.f, -30.f, 0.f, "dB");

    B (ids::reverbOn, "Reverb On", true);
    F (ids::reverbAmount, "Reverb", 0.f, 100.f, 18.f, 0.f, "%");
    C (ids::reverbType, "Reverb Tipo", SA ({ "Plate", "Room", "Large" }), 0);
    F (ids::reverbDecay, "Reverb Decay", 0.f, 100.f, 50.f, 0.f, "%");
    F (ids::reverbPre, "Reverb Pre-delay", 0.f, 100.f, 15.f, 0.f, "ms");
    F (ids::reverbLow, "Reverb Corte graves", 20.f, 1000.f, 150.f, 150.f, "Hz");
    F (ids::reverbHigh, "Reverb Corte agudos", 1000.f, 20000.f, 9000.f, 6000.f, "Hz");

    B (ids::delayOn, "Delay On", true);
    F (ids::delayAmount, "Delay", 0.f, 100.f, 17.f, 0.f, "%");
    C (ids::delayDiv, "Delay Division", SA ({ "1/16", "1/8", "1/4" }), 1);
    F (ids::delayFb, "Delay Feedback", 0.f, 90.f, 35.f, 0.f, "%");
    F (ids::delayLow, "Delay Corte graves", 20.f, 2000.f, 200.f, 300.f, "Hz");
    F (ids::delayHigh, "Delay Corte agudos", 1000.f, 20000.f, 6000.f, 5000.f, "Hz");

    B (ids::tuneOn, "Tune On", false);
    C (ids::tuneKey, "Tune Tonalidad", keys, 0);
    C (ids::tuneScale, "Tune Escala", SA ({ U ("Cromática"), "Mayor", "Menor" }), 0);
    F (ids::tuneSpeed, "Tune Velocidad", 0.f, 200.f, 20.f, 30.f, "ms");
    F (ids::tuneAmount, "Tune Cantidad", 0.f, 100.f, 100.f, 0.f, "%");

    C (ids::assistStyle, "Assist Estilo", SA ({ "Natural", "Rap/Trap", "Pop" }), 1);

    return L;
}
}

#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP.h"

namespace nv
{
    // Texto UTF-8 (acentos) -> juce::String
    inline juce::String U (const char* utf8) { return juce::String::fromUTF8 (utf8); }

    inline juce::StringArray SA (std::initializer_list<juce::String> items)
    {
        juce::StringArray a;
        for (auto& s : items) a.add (s);
        return a;
    }

    inline juce::Font F (float h, bool bold = false)
    {
        return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
    }
}

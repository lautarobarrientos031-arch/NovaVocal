#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace nv;

NovaVocalAudioProcessor::NovaVocalAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "NOVAVOCAL", createLayout())
{
}

bool NovaVocalAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return layouts.getMainInputChannelSet() == out;
}

void NovaVocalAudioProcessor::prepareToPlay (double sr, int block)
{
    fs = sr;
    tune.prepare (sr);
    eq.fs = comp.fs = deess.fs = magic.fs = sr;
    delay.prepare (sr);
    reverb.prepare (sr, block);
    assist.prepare (sr);
    scratch.assign ((size_t) std::max (block, 64), 0.f);
}

void NovaVocalAudioProcessor::updateMeter (std::atomic<float>& m, float peak, int n)
{
    const float db = linToDb (peak);
    const float fall = m.load() - 30.f * (float) n / (float) fs;
    m.store (std::max (db, fall));
}

void NovaVocalAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    auto P = [this] (const char* pid) { return apvts.getRawParameterValue (pid)->load(); };

    if (P (ids::power) < 0.5f) return;   // bypass

    const int nch = std::min (2, buffer.getNumChannels());
    float* const* d = buffer.getArrayOfWritePointers();

    buffer.applyGain (dbToLin (P (ids::inTrim)));
    updateMeter (inLevel, buffer.getMagnitude (0, n), n);

    // captura para Vocal Assist (mono)
    if (assist.getState() == VocalAssist::Listening)
    {
        const float* src = buffer.getReadPointer (0);
        if (nch > 1 && n <= (int) scratch.size())
        {
            for (int i = 0; i < n; ++i) scratch[(size_t) i] = 0.5f * (d[0][i] + d[1][i]);
            src = scratch.data();
        }
        assist.feed (src, n);
    }

    // 1) afinacion
    if (P (ids::tuneOn) > 0.5f)
        tune.process (d, nch, n, (int) P (ids::tuneKey), (int) P (ids::tuneScale), P (ids::tuneSpeed),
                      P (ids::tuneAmount) * 0.01f, (int) P (ids::mode) == 1);
    else
    {
        tune.detMidi = -1.f;
        tune.tgtMidi = -1.f;
    }

    // 2) EQ
    if (P (ids::eqOn) > 0.5f)
    {
        eq.update (fs, P (ids::eqHp), P (ids::eqLow), P (ids::eqMidF), P (ids::eqMidG), P (ids::eqMidQ),
                   P (ids::eqPresF), P (ids::eqPresG), P (ids::eqPresQ), P (ids::eqAir));
        eq.process (d, nch, n);
    }

    // 3) compresor (el "amount" escala el ratio)
    if (P (ids::compOn) > 0.5f)
    {
        const float ratio = 1.f + (P (ids::compRatio) - 1.f) * P (ids::compAmount) * 0.01f;
        comp.process (d, nch, n, P (ids::compThresh), ratio, P (ids::compAttack), P (ids::compRelease), P (ids::compMakeup));
        compGr = comp.lastGr;
    }
    else compGr = 0.f;

    // 4) de-esser
    if (P (ids::deessOn) > 0.5f && P (ids::deessAmount) > 0.5f)
    {
        deess.process (d, nch, n, P (ids::deessFreq), P (ids::deessThresh), P (ids::deessAmount) * 0.01f);
        deessGr = deess.lastRed;
    }
    else deessGr = 0.f;

    // 5) magic
    magic.process (d, nch, n, P (ids::magicAmount) * 0.01f, (int) P (ids::magicChar), P (ids::magicIntensity) * 0.01f);

    // 6) delay y reverb
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b;

    if (P (ids::delayOn) > 0.5f)
        delay.process (d, nch, n, P (ids::delayAmount) * 0.01f, (int) P (ids::delayDiv), P (ids::delayFb),
                       P (ids::delayLow), P (ids::delayHigh), bpm);

    if (P (ids::reverbOn) > 0.5f)
        reverb.process (d, nch, n, P (ids::reverbAmount) * 0.01f, (int) P (ids::reverbType), P (ids::reverbDecay) * 0.01f,
                        P (ids::reverbPre), P (ids::reverbLow), P (ids::reverbHigh));

    buffer.applyGain (dbToLin (P (ids::outGain)));
    updateMeter (outLevel, buffer.getMagnitude (0, n), n);
}

//==============================================================================
juce::AudioProcessorEditor* NovaVocalAudioProcessor::createEditor()
{
    return new NovaVocalAudioProcessorEditor (*this);
}

void NovaVocalAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void NovaVocalAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
void NovaVocalAudioProcessor::applyAssistResult()
{
    const auto r = assist.getResult();
    if (! r.valid) return;
    assistSnapshot = apvts.copyState();
    undoManager.beginNewTransaction ("Vocal Assist");
    for (auto& kv : r.params)
        if (auto* p = apvts.getParameter (kv.first))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (kv.second));
            p->endChangeGesture();
        }
}

void NovaVocalAudioProcessor::undoLast()
{
    if (assistSnapshot.isValid())
    {
        apvts.replaceState (assistSnapshot.createCopy());
        assistSnapshot = juce::ValueTree();
    }
    else undoManager.undo();
}

void NovaVocalAudioProcessor::resetAllParameters()
{
    undoManager.beginNewTransaction ("Reset");
    for (auto* p : getParameters())
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->getDefaultValue());
        p->endChangeGesture();
    }
}

juce::File NovaVocalAudioProcessor::presetDir() const
{
    auto d = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    d = d.getChildFile ("Application Support");
   #endif
    d = d.getChildFile ("NovaVocal").getChildFile ("Presets");
    d.createDirectory();
    return d;
}

juce::StringArray NovaVocalAudioProcessor::listPresets() const
{
    juce::StringArray names;
    for (auto& f : presetDir().findChildFiles (juce::File::findFiles, false, "*.nvpreset"))
        names.add (f.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

bool NovaVocalAudioProcessor::savePreset (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    auto xml = apvts.copyState().createXml();
    return xml != nullptr && presetDir().getChildFile (clean + ".nvpreset").replaceWithText (xml->toString());
}

bool NovaVocalAudioProcessor::loadPreset (const juce::String& name)
{
    const auto f = presetDir().getChildFile (name + ".nvpreset");
    if (auto xml = juce::parseXML (f))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            return true;
        }
    return false;
}

void NovaVocalAudioProcessor::toggleAB()
{
    (slotIsA ? slotA : slotB) = apvts.copyState();
    slotIsA = ! slotIsA;
    auto& next = slotIsA ? slotA : slotB;
    if (next.isValid()) apvts.replaceState (next.createCopy());
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NovaVocalAudioProcessor();
}

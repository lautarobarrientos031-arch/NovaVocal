#pragma once
#include "Common.h"

namespace nv
{
// Reverb tipo send: pre-delay -> corte de graves -> Freeverb -> corte de agudos -> suma al seco
struct ReverbFx
{
    juce::Reverb rv;
    DelayBuf pre;
    Biquad hp, lp;
    std::vector<float> tl, tr;
    double fs = 44100.0;

    void prepare (double sr, int maxBlock)
    {
        fs = sr;
        rv.setSampleRate (sr);
        rv.reset();
        pre.init ((int) (sr * 0.25));
        tl.assign ((size_t) std::max (maxBlock, 64), 0.f);
        tr = tl;
        hp.reset();
        lp.reset();
    }

    // type: 0 Plate, 1 Room, 2 Large. amount/decay en 0..1
    void process (float* const* d, int nch, int n, float amount, int type, float decay,
                  float preMs, float lowHz, float highHz)
    {
        if (amount < 0.001f) return;
        nch = std::min (nch, 2);
        if ((int) tl.size() < n) { tl.resize ((size_t) n); tr.resize ((size_t) n); }

        hp.makeHighPass (fs, lowHz);
        lp.makeLowPass (fs, highHz);
        const float pd = std::max (1.f, preMs * 0.001f * (float) fs);

        for (int i = 0; i < n; ++i)
        {
            const float m = nch > 1 ? 0.5f * (d[0][i] + d[1][i]) : d[0][i];
            pre.write (m);
            const float s = lp.process (hp.process (pre.read (pd), 0), 0);
            tl[(size_t) i] = tr[(size_t) i] = s;
        }

        static const float base[3] = { 0.55f, 0.35f, 0.88f };
        static const float damp[3] = { 0.35f, 0.60f, 0.45f };
        static const float wid[3]  = { 1.00f, 0.80f, 1.00f };
        const int t = juce::jlimit (0, 2, type);

        juce::Reverb::Parameters p;
        p.roomSize = juce::jlimit (0.05f, 0.98f, base[t] + (decay - 0.5f) * 0.4f);
        p.damping = damp[t];
        p.width = wid[t];
        p.wetLevel = 0.33f;
        p.dryLevel = 0.0f;
        p.freezeMode = 0.0f;
        rv.setParameters (p);
        rv.processStereo (tl.data(), tr.data(), n);

        const float g = std::pow (amount, 1.3f) * 1.1f;
        for (int i = 0; i < n; ++i)
        {
            if (nch > 1) { d[0][i] += tl[(size_t) i] * g; d[1][i] += tr[(size_t) i] * g; }
            else d[0][i] += 0.5f * (tl[(size_t) i] + tr[(size_t) i]) * g;
        }
    }
};
}

// Nova Vocal - nucleo DSP (C++17 puro, sin dependencias de JUCE)
#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <complex>
#include <cstring>
#include <vector>

namespace nv
{
constexpr double kPi = 3.14159265358979323846;
inline float dbToLin (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float linToDb (float lin) noexcept { return 20.0f * std::log10 (std::max (lin, 1.0e-6f)); }
inline const char* const kNoteNames[12] = { "Do", "Do#", "Re", "Re#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si" };

//==============================================================================
// Biquad (RBJ cookbook), estado por canal (max. 2)
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1[2] = { 0, 0 }, z2[2] = { 0, 0 };

    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0.0; }

    inline float process (float x, int ch) noexcept
    {
        const double y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return (float) y;
    }

    void setRaw (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }

    static double lim (double fs, double f) { return std::min (std::max (f, 10.0), fs * 0.45); }

    void makeHighPass (double fs, double f, double q = 0.7071)
    {
        f = lim (fs, f);
        const double w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        setRaw ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void makeLowPass (double fs, double f, double q = 0.7071)
    {
        f = lim (fs, f);
        const double w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        setRaw ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void makeBandPass (double fs, double f, double q)
    {
        f = lim (fs, f);
        const double w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        setRaw (al, 0, -al, 1 + al, -2 * c, 1 - al);
    }
    void makePeak (double fs, double f, double gDb, double q)
    {
        f = lim (fs, f);
        const double A = std::pow (10.0, gDb / 40.0), w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        setRaw (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void makeLowShelf (double fs, double f, double gDb, double q = 0.7071)
    {
        f = lim (fs, f);
        const double A = std::pow (10.0, gDb / 40.0), w = 2 * kPi * f / fs, c = std::cos (w);
        const double beta = 2 * std::sqrt (A) * (std::sin (w) / (2 * q));
        setRaw (A * ((A + 1) - (A - 1) * c + beta), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - beta),
                (A + 1) + (A - 1) * c + beta, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - beta);
    }
    void makeHighShelf (double fs, double f, double gDb, double q = 0.7071)
    {
        f = lim (fs, f);
        const double A = std::pow (10.0, gDb / 40.0), w = 2 * kPi * f / fs, c = std::cos (w);
        const double beta = 2 * std::sqrt (A) * (std::sin (w) / (2 * q));
        setRaw (A * ((A + 1) + (A - 1) * c + beta), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - beta),
                (A + 1) - (A - 1) * c + beta, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - beta);
    }

    double magnitudeDb (double f, double fs) const
    {
        const double w = 2.0 * kPi * f / fs;
        const std::complex<double> e1 = std::polar (1.0, -w), e2 = std::polar (1.0, -2.0 * w);
        const std::complex<double> num = b0 + b1 * e1 + b2 * e2, den = 1.0 + a1 * e1 + a2 * e2;
        return 20.0 * std::log10 (std::max (std::abs (num / den), 1.0e-9));
    }
};

//==============================================================================
struct DelayBuf
{
    std::vector<float> buf;
    int w = 0;

    void init (int n) { buf.assign ((size_t) std::max (n, 8), 0.f); w = 0; }
    inline void write (float x) noexcept { buf[(size_t) w] = x; if (++w >= (int) buf.size()) w = 0; }
    // d = retardo en muestras (>= 1)
    inline float read (float d) const noexcept
    {
        const int sz = (int) buf.size();
        float r = (float) w - d;
        while (r < 0.f) r += (float) sz;
        int i0 = (int) r;
        const float fr = r - (float) i0;
        if (i0 >= sz) i0 = 0;
        const int i1 = (i0 + 1 >= sz) ? 0 : i0 + 1;
        return buf[(size_t) i0] * (1.f - fr) + buf[(size_t) i1] * fr;
    }
};

//==============================================================================
inline bool inScale (int pc, int key, int scale)
{
    static const bool maj[12] = { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 };
    static const bool mnr[12] = { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0 };
    const int r = ((pc - key) % 12 + 12) % 12;
    return scale == 0 || (scale == 1 ? maj[r] : mnr[r]);
}

inline float quantise (float midi, int key, int scale)
{
    float best = midi, bestD = 1.0e9f;
    const int base = (int) std::floor (midi);
    for (int n = base - 3; n <= base + 4; ++n)
        if (inScale (((n % 12) + 12) % 12, key, scale))
        {
            const float dd = std::abs (midi - (float) n);
            if (dd < bestD) { bestD = dd; best = (float) n; }
        }
    return best;
}

//==============================================================================
// Detector de tono YIN
struct Yin
{
    std::vector<float> d, cm;
    void init (int tauMax) { d.assign ((size_t) tauMax + 2, 0.f); cm.assign ((size_t) tauMax + 2, 0.f); }

    // x debe tener al menos W + tauMax muestras. Devuelve f0 en Hz (0 si no hay tono).
    float detect (const float* x, double fs, int W, int tauMin, int tauMax, float& conf)
    {
        conf = 0.f;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            float s = 0.f;
            const float* b = x + tau;
            for (int j = 0; j < W; ++j) { const float df = x[j] - b[j]; s += df * df; }
            d[(size_t) tau] = s;
        }
        cm[0] = 1.f;
        float run = 0.f;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            run += d[(size_t) tau];
            cm[(size_t) tau] = run > 1.0e-12f ? d[(size_t) tau] * (float) tau / run : 1.f;
        }

        int est = -1;
        for (int tau = std::max (2, tauMin); tau < tauMax; ++tau)
            if (cm[(size_t) tau] < 0.15f)
            {
                while (tau + 1 < tauMax && cm[(size_t) tau + 1] < cm[(size_t) tau]) ++tau;
                est = tau;
                break;
            }
        if (est < 0)
        {
            int best = std::max (2, tauMin);
            for (int tau = best; tau < tauMax; ++tau)
                if (cm[(size_t) tau] < cm[(size_t) best]) best = tau;
            if (cm[(size_t) best] > 0.3f) return 0.f;
            est = best;
        }

        float tauF = (float) est;
        if (est > 1 && est < tauMax)
        {
            const float s0 = cm[(size_t) est - 1], s1 = cm[(size_t) est], s2 = cm[(size_t) est + 1];
            const float den = s0 + s2 - 2.f * s1;
            if (std::abs (den) > 1.0e-9f) tauF += 0.5f * (s0 - s2) / den;
        }
        conf = 1.f - cm[(size_t) est];
        return (float) (fs / (double) tauF);
    }
};

//==============================================================================
// Pitch shifter por linea de retardo con dos lecturas. La separacion entre lecturas
// es un multiplo del periodo de la voz, asi ambas copias suman en fase (sin efecto peine).
struct PitchShifter
{
    DelayBuf line;
    float phase = 0.f;

    void init (int n) { line.init (n); phase = 0.f; }

    inline float process (float x, float ratio, float spacing) noexcept
    {
        line.write (x);
        const float win = 2.f * spacing;
        phase += (1.f - ratio) / win;
        phase -= std::floor (phase);
        float p2 = phase + 0.5f;
        if (p2 >= 1.f) p2 -= 1.f;
        const float g1 = std::sin ((float) kPi * phase), g2 = std::sin ((float) kPi * p2);
        return g1 * g1 * line.read (phase * win + 1.f) + g2 * g2 * line.read (p2 * win + 1.f);
    }
};

class AutoTune
{
public:
    std::atomic<float> detMidi { -1.f }, tgtMidi { -1.f };

    void prepare (double sr)
    {
        fs = sr;
        W = (int) (fs * 0.025);
        tauMax = (int) (fs / 60.0);
        tauMin = (int) (fs / 1000.0);
        hop = std::max (64, (int) (fs * 0.010));
        const int need = W + tauMax + 8;
        int sz = 1;
        while (sz < need * 2) sz <<= 1;
        ring.assign ((size_t) sz, 0.f);
        mask = sz - 1; wpos = 0; hopCount = 0;
        lin.assign ((size_t) need, 0.f);
        yin.init (tauMax);
        for (auto& p : ps) p.init ((int) (fs * 0.15));
        curOffset = 0.f;
        ratioTarget = ratioSm = 1.f;
        spacingTarget = spacingSm = (float) (fs * 0.010);
    }

    void process (float* const* d, int nch, int n, int key, int scale, float speedMs, float amount01, bool live)
    {
        if (ring.empty()) return;
        nch = std::min (nch, 2);
        const float hopSec = (float) hop / (float) fs, targetMs = live ? 6.f : 12.f;
        for (int i = 0; i < n; ++i)
        {
            float m = d[0][i];
            if (nch > 1) m = 0.5f * (m + d[1][i]);
            ring[(size_t) wpos] = m;
            wpos = (wpos + 1) & mask;
            if (++hopCount >= hop) { hopCount = 0; analyse (key, scale, speedMs, amount01, hopSec, targetMs); }
            ratioSm += (ratioTarget - ratioSm) * 0.01f;
            spacingSm += (spacingTarget - spacingSm) * 0.0005f;
            for (int c = 0; c < nch; ++c) d[c][i] = ps[c].process (d[c][i], ratioSm, spacingSm);
        }
    }

private:
    void analyse (int key, int scale, float speedMs, float amount01, float hopSec, float targetMs)
    {
        const int need = W + tauMax;
        const int start = (wpos - need) & mask;
        for (int k = 0; k < need; ++k) lin[(size_t) k] = ring[(size_t) ((start + k) & mask)];

        double e = 0.0;
        for (int k = 0; k < W; ++k) e += (double) lin[(size_t) k] * lin[(size_t) k];
        const float rms = (float) std::sqrt (e / W);

        float conf = 0.f, f0 = 0.f;
        if (rms > 0.004f) f0 = yin.detect (lin.data(), fs, W, tauMin, tauMax, conf);

        if (f0 > 0.f && conf > 0.75f)
        {
            const float midi = 69.f + 12.f * std::log2 (f0 / 440.f);
            const float tgt = quantise (midi, key, scale);
            const float off = (tgt - midi) * amount01;
            if (speedMs < 1.f) curOffset = off;
            else curOffset += (off - curOffset) * (1.f - std::exp (-hopSec / (speedMs * 0.001f)));
            detMidi = midi;
            tgtMidi = tgt;
            const float T = (float) fs / f0;
            const float K = std::max (1.f, std::round (targetMs * 0.001f * (float) fs / T));
            spacingTarget = std::min (std::max (K * T, 32.f), (float) fs * 0.04f);
        }
        else
        {
            curOffset *= 0.97f;
            detMidi = -1.f;
            tgtMidi = -1.f;
        }
        ratioTarget = std::pow (2.f, curOffset / 12.f);
    }

    double fs = 44100.0;
    int W = 1024, tauMax = 700, tauMin = 44, hop = 441, mask = 0, wpos = 0, hopCount = 0;
    std::vector<float> ring, lin;
    Yin yin;
    PitchShifter ps[2];
    float curOffset = 0.f, ratioTarget = 1.f, ratioSm = 1.f, spacingTarget = 480.f, spacingSm = 480.f;
};

//==============================================================================
struct EQ
{
    Biquad hp, low, mid, pres, air;
    double fs = 44100.0;

    void update (double sr, float hpF, float lowG, float midF, float midG, float midQ,
                 float presF, float presG, float presQ, float airG)
    {
        fs = sr;
        hp.makeHighPass (fs, hpF);
        low.makeLowShelf (fs, 120.0, lowG);
        mid.makePeak (fs, midF, midG, midQ);
        pres.makePeak (fs, presF, presG, presQ);
        air.makeHighShelf (fs, 10000.0, airG);
    }

    void process (float* const* d, int nch, int n)
    {
        for (int c = 0; c < std::min (nch, 2); ++c)
        {
            float* x = d[c];
            for (int i = 0; i < n; ++i)
            {
                float v = hp.process (x[i], c);
                v = low.process (v, c);
                v = mid.process (v, c);
                v = pres.process (v, c);
                x[i] = air.process (v, c);
            }
        }
    }

    double responseDb (double f) const
    {
        return hp.magnitudeDb (f, fs) + low.magnitudeDb (f, fs) + mid.magnitudeDb (f, fs)
             + pres.magnitudeDb (f, fs) + air.magnitudeDb (f, fs);
    }
};

//==============================================================================
struct Compressor
{
    double fs = 44100.0;
    float env = 0.f, lastGr = 0.f;

    void process (float* const* d, int nch, int n, float thresh, float ratio, float atkMs, float relMs, float makeupDb)
    {
        nch = std::min (nch, 2);
        const float aA = std::exp (-1.f / (0.001f * std::max (atkMs, 0.05f) * (float) fs));
        const float aR = std::exp (-1.f / (0.001f * std::max (relMs, 1.f) * (float) fs));
        const float knee = 6.f;
        float minGr = 0.f;
        for (int i = 0; i < n; ++i)
        {
            float lvl = 0.f;
            for (int c = 0; c < nch; ++c) lvl = std::max (lvl, std::abs (d[c][i]));
            const float ldb = linToDb (lvl), over = ldb - thresh;
            float outDb;
            if (2.f * over < -knee) outDb = ldb;
            else if (2.f * std::abs (over) <= knee)
            {
                const float t = over + knee * 0.5f;
                outDb = ldb + (1.f / ratio - 1.f) * t * t / (2.f * knee);
            }
            else outDb = thresh + over / ratio;
            const float gr = outDb - ldb;
            const float co = gr < env ? aA : aR;
            env = co * env + (1.f - co) * gr;
            const float g = dbToLin (env + makeupDb);
            for (int c = 0; c < nch; ++c) d[c][i] *= g;
            minGr = std::min (minGr, env);
        }
        lastGr = minGr;
    }
};

//==============================================================================
// De-esser de banda dividida: reduce solo la banda de sibilancia
struct DeEsser
{
    Biquad bp;
    double fs = 44100.0;
    float env = 0.f, lastRed = 0.f;

    void process (float* const* d, int nch, int n, float freq, float threshDb, float amount01)
    {
        nch = std::min (nch, 2);
        bp.makeBandPass (fs, freq, 1.4);
        const float aA = std::exp (-1.f / (0.0005f * (float) fs)), aR = std::exp (-1.f / (0.03f * (float) fs));
        const float maxRed = 15.f * amount01;
        float peak = 0.f;
        for (int i = 0; i < n; ++i)
        {
            float band[2] = { 0.f, 0.f }, det = 0.f;
            for (int c = 0; c < nch; ++c) { band[c] = bp.process (d[c][i], c); det = std::max (det, std::abs (band[c])); }
            env = det > env ? aA * env + (1.f - aA) * det : aR * env + (1.f - aR) * det;
            const float red = std::min (std::max (0.f, linToDb (env) - threshDb) * 0.75f, maxRed);
            const float g = dbToLin (-red);
            for (int c = 0; c < nch; ++c) d[c][i] = (d[c][i] - band[c]) + band[c] * g;
            peak = std::max (peak, red);
        }
        lastRed = peak;
    }
};

//==============================================================================
// "Magic": color + brillo + saturacion suave
struct Magic
{
    Biquad lowS, hiS, bell;
    double fs = 44100.0;

    void process (float* const* d, int nch, int n, float amount, int type, float intensity)
    {
        if (amount < 0.001f) return;
        nch = std::min (nch, 2);
        float lowG = 0.f, hiG = 0.f, bellG = 0.f, drive = 1.f;
        double hiF = 6000.0;
        switch (type)
        {
            case 0:  lowG = 3.f * intensity; hiG = -1.5f * intensity; drive = 1.f + 1.5f * intensity; break;           // Warm
            case 1:  hiG = 6.f * intensity; hiF = 7500.0; bellG = 2.5f * intensity; drive = 1.f + 0.7f * intensity; break; // Agudo
            default: lowG = 1.5f * intensity; hiG = 2.f * intensity; drive = 1.f + 6.f * intensity; break;             // Saturacion
        }
        lowS.makeLowShelf (fs, 200.0, lowG);
        hiS.makeHighShelf (fs, hiF, hiG);
        bell.makePeak (fs, 3500.0, bellG, 0.9);
        for (int c = 0; c < nch; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float x = d[c][i];
                float w = bell.process (hiS.process (lowS.process (x, c), c), c);
                w = std::tanh (drive * w) / drive;
                d[c][i] = x + (w - x) * amount;
            }
    }
};

//==============================================================================
struct DelayFx
{
    DelayBuf line[2];
    Biquad hp, lp;
    double fs = 44100.0;
    float cur = 0.f;
    int cap = 0;

    void prepare (double sr)
    {
        fs = sr;
        cap = (int) (sr * 4.0) + 8;
        line[0].init (cap);
        line[1].init (cap);
        hp.reset(); lp.reset();
        cur = 0.f;
    }

    // div: 0 = 1/16, 1 = 1/8, 2 = 1/4
    void process (float* const* d, int nch, int n, float amount, int div, float fbPct,
                  float lowHz, float highHz, double bpm)
    {
        if (amount < 0.001f || cap == 0) return;
        nch = std::min (nch, 2);
        if (bpm < 20.0) bpm = 120.0;
        const float beats = div == 0 ? 0.25f : (div == 1 ? 0.5f : 1.0f);
        const float target = std::min ((float) (60.0 / bpm * beats * fs), (float) (cap - 4));
        if (cur <= 0.f) cur = target;
        hp.makeHighPass (fs, lowHz);
        lp.makeLowPass (fs, highHz);
        const float fb = std::min (fbPct * 0.01f, 0.92f), wet = amount * 0.9f;
        for (int i = 0; i < n; ++i)
        {
            cur += (target - cur) * 0.0004f;
            for (int c = 0; c < nch; ++c)
            {
                const float x = d[c][i];
                float y = line[c].read (std::max (cur, 4.f));
                y = lp.process (hp.process (y, c), c);
                line[c].write (x + y * fb);
                d[c][i] = x + y * wet;
            }
        }
    }
};

} // namespace nv

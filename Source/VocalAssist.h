#pragma once
#include "Common.h"
#include "Params.h"

namespace nv
{
//==============================================================================
// Vocal Assist: escucha una toma de voz (o carga un archivo), la analiza
// (nivel, dinamica, balance espectral, sibilancia, registro y tonalidad) y propone
// valores para todos los modulos. El analisis corre en un hilo aparte.
class VocalAssist : private juce::Thread
{
public:
    static constexpr float kMaxSeconds = 12.f;
    enum State { Idle = 0, Listening, Analyzing, Done };

    struct Result
    {
        bool valid = false;
        juce::String summary;
        std::vector<std::pair<juce::String, float>> params;
    };

    VocalAssist() : juce::Thread ("NovaVocal Assist") {}
    ~VocalAssist() override { stopThread (8000); }

    void prepare (double sr)
    {
        if (state.load() == Analyzing) return;
        fs = sr;
        const size_t want = (size_t) (sr * kMaxSeconds);
        if (rec.size() != want)
        {
            rec.assign (want, 0.f);
            recCount = 0;
            if (state.load() == Listening) state = Idle;
        }
    }

    // --- hilo de audio
    void feed (const float* mono, int n) noexcept
    {
        if (state.load() != Listening) return;
        const int c = recCount.load();
        const int m = std::min (n, (int) rec.size() - c);
        if (m > 0)
        {
            std::memcpy (rec.data() + c, mono, (size_t) m * sizeof (float));
            recCount.store (c + m);
        }
    }

    // --- hilo de interfaz
    void startListening()
    {
        if (state.load() == Analyzing || rec.empty()) return;
        recCount = 0;
        setResult ({});
        state = Listening;
    }

    void stopListening (int style)
    {
        if (state.load() != Listening) return;
        const int c = recCount.load();
        std::vector<float> d (rec.begin(), rec.begin() + c);
        state = Analyzing;
        launch (std::move (d), fs, style);
    }

    bool autoStopDue() const { return state.load() == Listening && recCount.load() >= (int) rec.size(); }
    float seconds() const { return fs > 0.0 ? (float) recCount.load() / (float) fs : 0.f; }
    State getState() const { return (State) state.load(); }

    Result getResult() const
    {
        const juce::ScopedLock sl (lock);
        return result;
    }

    void reset()
    {
        if (state.load() == Analyzing) return;
        recCount = 0;
        setResult ({});
        state = Idle;
    }

    void loadFile (const juce::File& f, int style)
    {
        if (state.load() == Analyzing) return;
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));

        if (r == nullptr || r->lengthInSamples <= 0)
        {
            Result bad;
            bad.summary = U ("No se pudo leer el archivo de audio.");
            setResult (std::move (bad));
            state = Done;
            return;
        }

        const int n = (int) std::min<juce::int64> (r->lengthInSamples, (juce::int64) (r->sampleRate * 30.0));
        juce::AudioBuffer<float> tmp (std::max (1, (int) std::min<unsigned int> (r->numChannels, 2u)), n);
        r->read (&tmp, 0, n, 0, true, true);
        std::vector<float> mono ((size_t) n, 0.f);
        for (int ch = 0; ch < tmp.getNumChannels(); ++ch)
            for (int i = 0; i < n; ++i) mono[(size_t) i] += tmp.getSample (ch, i) / (float) tmp.getNumChannels();

        setResult ({});
        state = Analyzing;
        launch (std::move (mono), r->sampleRate, style);
    }

private:
    void launch (std::vector<float>&& d, double sr, int style)
    {
        {
            const juce::ScopedLock sl (lock);
            job = std::move (d);
            jobFs = sr;
            jobStyle = style;
        }
        startThread();
    }

    void setResult (Result r)
    {
        const juce::ScopedLock sl (lock);
        result = std::move (r);
    }

    void run() override
    {
        std::vector<float> d;
        double sr = 48000.0;
        int style = 0;
        {
            const juce::ScopedLock sl (lock);
            d = std::move (job);
            sr = jobFs;
            style = jobStyle;
        }
        Result r = analyse (d, sr, style);
        setResult (std::move (r));
        state = Done;
    }

    //==========================================================================
    struct Bias { float pres, air, comp, deess, reverb, delay, magic, tuneSpeed, tuneAmt; };

    static float pct (const std::vector<float>& sorted, float p)
    {
        if (sorted.empty()) return 0.f;
        return sorted[(size_t) (juce::jlimit (0.f, 1.f, p) * (float) (sorted.size() - 1))];
    }

    Result analyse (const std::vector<float>& x, double sr, int style)
    {
        Result res;
        constexpr int N = 2048, hop = 1024;
        style = juce::jlimit (0, 2, style);

        if ((int) x.size() < N * 6)
        {
            res.summary = U ("Toma demasiado corta. Grabá al menos 3 segundos de voz.");
            return res;
        }

        // ---- 1) energia por frame y deteccion de actividad
        const int nFrames = ((int) x.size() - N) / hop + 1;
        std::vector<float> frameDb ((size_t) nFrames);
        float maxDb = -200.f;
        for (int f = 0; f < nFrames; ++f)
        {
            double e = 0.0;
            const float* p = &x[(size_t) f * hop];
            for (int k = 0; k < N; ++k) e += (double) p[k] * p[k];
            frameDb[(size_t) f] = (float) (10.0 * std::log10 (e / N + 1.0e-12));
            maxDb = std::max (maxDb, frameDb[(size_t) f]);
        }
        const float gate = std::max (-55.f, maxDb - 30.f);
        std::vector<int> act;
        for (int f = 0; f < nFrames; ++f)
            if (frameDb[(size_t) f] > gate) act.push_back (f);

        if (act.size() < 8)
        {
            res.summary = U ("No se detectó suficiente voz. Probá con una toma más fuerte o más larga.");
            return res;
        }

        // ---- 2) espectro medio de los frames activos
        juce::dsp::FFT fft (11);
        juce::dsp::WindowingFunction<float> win ((size_t) N, juce::dsp::WindowingFunction<float>::hann);
        std::vector<float> fftData ((size_t) N * 2);
        std::vector<double> psd ((size_t) N / 2, 0.0);
        for (int f : act)
        {
            std::fill (fftData.begin(), fftData.end(), 0.f);
            std::copy (x.begin() + (long) f * hop, x.begin() + (long) f * hop + N, fftData.begin());
            win.multiplyWithWindowingTable (fftData.data(), (size_t) N);
            fft.performFrequencyOnlyForwardTransform (fftData.data());
            for (int k = 0; k < N / 2; ++k) psd[(size_t) k] += (double) fftData[(size_t) k] * fftData[(size_t) k];
        }
        for (auto& v : psd) v /= (double) act.size();

        auto bandDb = [&] (double f1, double f2)
        {
            const int k1 = std::max (1, (int) (f1 * N / sr)), k2 = std::min (N / 2 - 1, (int) (f2 * N / sr));
            double s = 0.0; int c = 0;
            for (int k = k1; k <= k2; ++k) { s += psd[(size_t) k]; ++c; }
            return (float) (10.0 * std::log10 (s / std::max (c, 1) + 1.0e-12));
        };
        const float mid = bandDb (500, 2000);
        const float relLow = bandDb (80, 200) - mid;
        const float relLM = bandDb (200, 500) - mid;
        const float relPres = bandDb (2500, 5000) - mid;
        const float relAir = bandDb (10000, 16000) - mid;

        // frecuencia central de la sibilancia
        int kMin = (int) (4500.0 * N / sr), kMax = std::min (N / 2 - 4, (int) (9500.0 * N / sr));
        double bestP = -1.0; int bestK = kMin;
        for (int k = kMin; k <= kMax; ++k)
        {
            double s = 0.0;
            for (int j = -3; j <= 3; ++j) s += psd[(size_t) juce::jlimit (1, N / 2 - 1, k + j)];
            if (s > bestP) { bestP = s; bestK = k; }
        }
        const float sibHz = juce::jlimit (4500.f, 9500.f, (float) (bestK * sr / N));

        // ---- 3) tono (YIN) y tonalidad
        Yin yin;
        const int W = (int) (sr * 0.025), tauMax = (int) (sr / 60.0), tauMin = (int) (sr / 1000.0);
        yin.init (tauMax);
        std::vector<float> midis;
        double hist[12] = { 0 };
        for (int f : act)
        {
            if (threadShouldExit()) return res;
            const int start = f * hop;
            if (start + W + tauMax >= (int) x.size()) continue;
            float conf = 0.f;
            const float f0 = yin.detect (&x[(size_t) start], sr, W, tauMin, tauMax, conf);
            if (f0 > 0.f && conf > 0.85f)
            {
                const float m = 69.f + 12.f * std::log2 (f0 / 440.f);
                midis.push_back (m);
                hist[((int) std::lround (m) % 12 + 12) % 12] += conf;
            }
        }

        float f0med = 150.f;
        bool haveKey = midis.size() >= 5;
        if (! midis.empty())
        {
            std::sort (midis.begin(), midis.end());
            f0med = 440.f * std::pow (2.f, (midis[midis.size() / 2] - 69.f) / 12.f);
        }

        int bestKey = 0, bestScale = 1;
        if (haveKey)
        {
            static const double maj[12] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
            static const double mnr[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };
            double bestC = -2.0;
            for (int k = 0; k < 12; ++k)
                for (int sc = 1; sc <= 2; ++sc)
                {
                    const double* prof = sc == 1 ? maj : mnr;
                    double mh = 0, mp = 0;
                    for (int i = 0; i < 12; ++i) { mh += hist[i]; mp += prof[i]; }
                    mh /= 12; mp /= 12;
                    double num = 0, dh = 0, dp = 0;
                    for (int i = 0; i < 12; ++i)
                    {
                        const double h = hist[i] - mh, p = prof[(i - k + 12) % 12] - mp;
                        num += h * p; dh += h * h; dp += p * p;
                    }
                    const double c = num / std::sqrt (dh * dp + 1.0e-12);
                    if (c > bestC) { bestC = c; bestKey = k; bestScale = sc; }
                }
        }

        // ---- 4) dinamica
        std::vector<float> lv;
        double pw = 0.0;
        for (int f : act) { lv.push_back (frameDb[(size_t) f]); pw += std::pow (10.0, frameDb[(size_t) f] / 10.0); }
        std::sort (lv.begin(), lv.end());
        const float p10 = pct (lv, 0.10f), p50 = pct (lv, 0.50f), p90 = pct (lv, 0.90f);
        const float meanDb = (float) (10.0 * std::log10 (pw / (double) act.size()));
        const float range = p90 - p10;

        // ---- 5) sibilancia: envolvente en la banda, igual que el de-esser
        Biquad bp;
        bp.makeBandPass (sr, sibHz, 1.4);
        const float aA = std::exp (-1.f / (0.0005f * (float) sr)), aR = std::exp (-1.f / (0.03f * (float) sr));
        float env = 0.f;
        std::vector<float> envDb;
        for (size_t i = 0; i < x.size(); ++i)
        {
            const float b = std::abs (bp.process (x[i], 0));
            env = b > env ? aA * env + (1.f - aA) * b : aR * env + (1.f - aR) * b;
            if ((i & 63) == 0)
            {
                const int fr = (int) (i / hop);
                if (fr < nFrames && frameDb[(size_t) fr] > gate) envDb.push_back (linToDb (env));
            }
        }
        std::sort (envDb.begin(), envDb.end());
        const float e60 = pct (envDb, 0.60f), e85 = pct (envDb, 0.85f), e98 = pct (envDb, 0.98f);

        // ---- 6) decisiones
        static const Bias biases[3] = {
            { 0.0f, 0.0f, 0.0f,  0.0f, 14.f,  8.f, 35.f, 60.f,  60.f },   // Natural
            { 1.5f, 2.0f, 1.0f, 10.0f, 22.f, 18.f, 55.f,  0.f, 100.f },   // Rap/Trap
            { 1.0f, 1.5f, 0.0f,  5.0f, 20.f, 10.f, 45.f, 25.f,  80.f } }; // Pop
        const Bias& bs = biases[style];
        const bool female = f0med > 165.f;

        const float hpHz = juce::jlimit (60.f, 130.f, f0med * 0.55f);
        const float eqLow = juce::jlimit (-4.f, 2.f, -(relLow - 3.f) * 0.5f);
        const float midG = juce::jlimit (-5.f, 1.5f, -(relLM - 1.5f) * 0.6f);
        const float presG = juce::jlimit (-3.f, 5.f, (-4.f - relPres) * 0.5f + bs.pres);
        const float airDef = -16.f - relAir;
        const float airG = juce::jlimit (-2.f, 5.f, airDef * 0.4f + bs.air);

        const float ratio = juce::jlimit (2.f, 6.f, 2.f + (range - 8.f) / 3.f + bs.comp);
        const float thresh = juce::jlimit (-40.f, -8.f, p50 + 2.f);
        const float ratioEff = 1.f + (ratio - 1.f) * 0.8f;
        float grSum = 0.f;
        for (float v : lv)
        {
            const float over = (v + 3.f) - thresh;
            if (over > 0.f) grSum += over * (1.f - 1.f / ratioEff);
        }
        const float avgGR = grSum / (float) lv.size();
        const float makeup = juce::jlimit (-6.f, 15.f, -18.f - (meanDb - avgGR));

        const float dThresh = juce::jlimit (-60.f, -5.f, e85 - 1.f + (makeup - avgGR));
        const float dAmt = juce::jlimit (25.f, 100.f, 30.f + (e98 - e60) * 2.2f + bs.deess);

        auto add = [&] (const char* pid, float v) { res.params.emplace_back (juce::String (pid), v); };
        add (ids::eqOn, 1.f);  add (ids::eqHp, hpHz);  add (ids::eqLow, eqLow);
        add (ids::eqMidF, 320.f);  add (ids::eqMidG, midG);  add (ids::eqMidQ, 1.0f);
        add (ids::eqPresF, female ? 4200.f : 3500.f);  add (ids::eqPresG, presG);  add (ids::eqPresQ, 0.9f);
        add (ids::eqAir, airG);

        add (ids::compOn, 1.f);  add (ids::compAmount, 80.f);  add (ids::compThresh, thresh);
        add (ids::compRatio, ratio);  add (ids::compAttack, style == 1 ? 12.f : 8.f);
        add (ids::compRelease, style == 1 ? 90.f : 120.f);  add (ids::compMakeup, makeup);

        add (ids::deessOn, 1.f);  add (ids::deessFreq, sibHz);  add (ids::deessThresh, dThresh);  add (ids::deessAmount, dAmt);

        add (ids::magicAmount, bs.magic);  add (ids::magicChar, airDef > 3.f ? 1.f : 0.f);  add (ids::magicIntensity, 50.f);

        add (ids::reverbOn, 1.f);  add (ids::reverbAmount, bs.reverb);  add (ids::reverbType, 0.f);
        add (ids::delayOn, 1.f);  add (ids::delayAmount, bs.delay);  add (ids::delayDiv, 1.f);

        if (haveKey) { add (ids::tuneKey, (float) bestKey);  add (ids::tuneScale, (float) bestScale); }
        add (ids::tuneSpeed, bs.tuneSpeed);  add (ids::tuneAmount, bs.tuneAmt);
        if (style != 0) add (ids::tuneOn, 1.f);

        res.valid = true;
        res.summary = U (female ? "Voz aguda" : "Voz grave") + " (~" + juce::String ((int) f0med) + " Hz)";
        if (haveKey) res.summary << U ("  |  Tonalidad: ") << kNoteNames[bestKey] << (bestScale == 1 ? " mayor" : " menor");
        res.summary << "\n" << U ("Dinámica: ") << juce::String (range, 1) << " dB  |  "
                    << "Sibilancia: " << juce::String (sibHz / 1000.f, 1) << " kHz  |  "
                    << "Nivel: " << juce::String (meanDb, 1) << " dBFS";
        return res;
    }

    //==========================================================================
    double fs = 48000.0;
    std::vector<float> rec;
    std::atomic<int> recCount { 0 }, state { Idle };

    mutable juce::CriticalSection lock;
    Result result;
    std::vector<float> job;
    double jobFs = 48000.0;
    int jobStyle = 0;
};
}

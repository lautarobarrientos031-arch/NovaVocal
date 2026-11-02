// g++ -std=c++17 -O2 Tests/test_core.cpp -o test_core && ./test_core
#include "../Source/DSP.h"
#include <cstdio>

using namespace nv;

static std::vector<float> voice (double fs, double seconds, double f0, double vibCents)
{
    std::vector<float> x ((size_t) (fs * seconds));
    double ph = 0.0;
    for (size_t n = 0; n < x.size(); ++n)
    {
        const double f = f0 * std::pow (2.0, vibCents / 1200.0 * std::sin (2 * kPi * 5.5 * (double) n / fs));
        ph += 2 * kPi * f / fs;
        double s = 0;
        for (int h = 1; h <= 8; ++h) s += std::sin (h * ph) / h;
        x[n] = 0.2f * (float) s;
    }
    return x;
}

static float measureF0 (const std::vector<float>& x, double fs, size_t from)
{
    Yin yin;
    const int W = (int) (fs * 0.025), tmax = (int) (fs / 60.0), tmin = (int) (fs / 1000.0);
    yin.init (tmax);
    double sum = 0; int cnt = 0;
    for (size_t s = from; s + (size_t) (W + tmax) < x.size(); s += 2400)
    {
        float conf; const float f = yin.detect (&x[s], fs, W, tmin, tmax, conf);
        if (f > 0 && conf > 0.7f) { sum += f; ++cnt; }
    }
    return cnt ? (float) (sum / cnt) : 0.f;
}

static double rms (const std::vector<float>& x, size_t from)
{
    double e = 0; for (size_t i = from; i < x.size(); ++i) e += x[i] * x[i];
    return std::sqrt (e / (double) (x.size() - from));
}

static std::vector<float> runTune (std::vector<float> x, double fs, int key, int scale, float speed, bool live)
{
    AutoTune at; at.prepare (fs);
    for (size_t s = 0; s < x.size(); s += 512)
    {
        float* ch[1] = { &x[s] };
        at.process (ch, 1, (int) std::min<size_t> (512, x.size() - s), key, scale, speed, 1.f, live);
    }
    return x;
}

static int fails = 0;
static void check (bool ok, const char* what) { std::printf ("[%s] %s\n", ok ? "OK " : "FAIL", what); if (! ok) ++fails; }

int main()
{
    const double fs = 48000.0;

    {   // 225 Hz (+39 cents sobre La3=220) -> cromatico: debe ir a ~220 Hz
        auto in = voice (fs, 3.0, 225.0, 8.0);
        auto out = runTune (in, fs, 0, 0, 0.f, false);
        const float fi = measureF0 (in, fs, 48000), fo = measureF0 (out, fs, 48000);
        std::printf ("  entrada %.2f Hz -> salida %.2f Hz (objetivo 220)\n", fi, fo);
        check (std::abs (fo - 220.f) < 1.5f, "autotune cromatico 225 -> 220 Hz");
        check (rms (out, 48000) > 0.6 * rms (in, 48000) && rms (out, 48000) < 1.4 * rms (in, 48000), "nivel de salida estable");
    }
    {   // 231 Hz (A#3 -15c) en Do mayor -> La (220 Hz)
        auto in = voice (fs, 3.0, 231.0, 5.0);
        auto out = runTune (in, fs, 0, 1, 0.f, true);
        const float fo = measureF0 (out, fs, 48000);
        std::printf ("  entrada 231 Hz (Do mayor) -> salida %.2f Hz (objetivo 220)\n", fo);
        check (std::abs (fo - 220.f) < 2.0f, "autotune Do mayor (modo live)");
    }
    {   // Nota ya afinada: no debe mover el tono
        auto in = voice (fs, 3.0, 220.0, 0.0);
        auto out = runTune (in, fs, 0, 0, 20.f, false);
        const float fo = measureF0 (out, fs, 48000);
        check (std::abs (fo - 220.f) < 1.0f, "nota afinada se mantiene");
    }
    {   // Compresor: seno 1 kHz a -6 dBFS, umbral -20, ratio 4 => GR ~ -10.5 dB
        std::vector<float> x ((size_t) fs);
        for (size_t n = 0; n < x.size(); ++n) x[n] = 0.5f * std::sin (2 * kPi * 1000.0 * (double) n / fs);
        Compressor c; c.fs = fs;
        float* ch[1] = { x.data() };
        c.process (ch, 1, (int) x.size(), -20.f, 4.f, 5.f, 100.f, 0.f);
        std::printf ("  GR = %.2f dB\n", c.lastGr);
        check (c.lastGr < -8.f && c.lastGr > -13.f, "compresor reduce ~10 dB");
    }
    {   // De-esser: tono de 7 kHz debe atenuarse, tono de 200 Hz no
        auto test = [&] (double f) {
            std::vector<float> x ((size_t) fs);
            for (size_t n = 0; n < x.size(); ++n) x[n] = 0.3f * std::sin (2 * kPi * f * (double) n / fs);
            DeEsser d; d.fs = fs;
            float* ch[1] = { x.data() };
            d.process (ch, 1, (int) x.size(), 7000.f, -30.f, 1.f);
            return (float) (20 * std::log10 (rms (x, 24000) / (0.3 / std::sqrt (2.0))));
        };
        const float hi = test (7000.0), lo = test (200.0);
        std::printf ("  de-esser: 7 kHz %.1f dB, 200 Hz %.1f dB\n", hi, lo);
        check (hi < -6.f && lo > -0.5f, "de-esser selectivo");
    }
    {   // EQ: pico de +6 dB a 3.5 kHz
        EQ e; e.update (fs, 80, 0, 350, 0, 1, 3500, 6, 1, 0);
        const double g = e.responseDb (3500.0);
        std::printf ("  EQ @3.5k = %.2f dB\n", g);
        check (std::abs (g - 6.0) < 0.3, "EQ presencia +6 dB");
    }
    std::printf (fails ? "\n%d FALLOS\n" : "\nTodo OK\n", fails);
    return fails;
}

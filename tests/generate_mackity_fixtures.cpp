// Offline renderer for character-test fixtures. Ports Mackity's process loop
// (double path) with A=0.1, B=1.0. Omits TPDF/frexpf dither and denormal
// noise injection so refs stay usable; topology/constants match MackityProc.cpp.

#include "MackityRef.h"
#include "WavFile.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kSr = 48000.0;
constexpr float kA = 0.1f;
constexpr float kB = 1.0f;

StereoWav makeSine()
{
    StereoWav w;
    w.sampleRate = static_cast<int> (kSr);
    const int n = static_cast<int> (kSr); // 1 s
    const float amp = std::pow (10.0f, -6.0f / 20.0f);
    w.left.resize (static_cast<size_t> (n));
    w.right.resize (static_cast<size_t> (n));
    for (int i = 0; i < n; ++i)
    {
        const float s = amp * static_cast<float> (std::sin (2.0 * kPi * 1000.0 * static_cast<double> (i) / kSr));
        w.left[static_cast<size_t> (i)] = s;
        w.right[static_cast<size_t> (i)] = s;
    }
    return w;
}

uint32_t xorshift (uint32_t& s)
{
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}

float noise (uint32_t& s)
{
    return static_cast<float> (static_cast<int32_t> (xorshift (s))) / 2147483648.0f;
}

StereoWav makeDrumBurst()
{
    // Synthetic percussive stereo excerpt (not a licensed loop): kick-like
    // decaying oscillator + noise transients at 0, 0.25, 0.5 s.
    StereoWav w;
    w.sampleRate = static_cast<int> (kSr);
    const int n = static_cast<int> (kSr * 0.75);
    w.left.assign (static_cast<size_t> (n), 0.0f);
    w.right.assign (static_cast<size_t> (n), 0.0f);

    auto hit = [&] (int start, double f0, double f1, float amp, int noiseLen, uint32_t seedL, uint32_t seedR)
    {
        const int len = static_cast<int> (kSr * 0.22);
        for (int i = 0; i < len && start + i < n; ++i)
        {
            const double t = static_cast<double> (i) / kSr;
            const double env = std::exp (-t * 18.0);
            const double freq = f0 + (f1 - f0) * (1.0 - env);
            const double phase = 2.0 * kPi * freq * t;
            const float kick = static_cast<float> (amp * env * std::sin (phase));
            w.left[static_cast<size_t> (start + i)] += kick;
            w.right[static_cast<size_t> (start + i)] += kick * 0.92f;
        }
        for (int i = 0; i < noiseLen && start + i < n; ++i)
        {
            const float env = static_cast<float> (std::exp (-static_cast<double> (i) / (kSr * 0.012)));
            w.left[static_cast<size_t> (start + i)] += 0.35f * env * noise (seedL);
            w.right[static_cast<size_t> (start + i)] += 0.32f * env * noise (seedR);
        }
    };

    hit (0, 120.0, 42.0, 0.95f, 900, 0xC0FFEEu, 0xA5A5A5u);
    hit (static_cast<int> (kSr * 0.25), 280.0, 160.0, 0.55f, 1400, 0x123456u, 0x654321u);
    hit (static_cast<int> (kSr * 0.5), 90.0, 38.0, 0.85f, 700, 0x111111u, 0x222222u);

    for (int i = 0; i < n; ++i)
    {
        w.left[static_cast<size_t> (i)] = std::max (-1.0f, std::min (1.0f, w.left[static_cast<size_t> (i)]));
        w.right[static_cast<size_t> (i)] = std::max (-1.0f, std::min (1.0f, w.right[static_cast<size_t> (i)]));
    }
    return w;
}

StereoWav renderRef (const StereoWav& in)
{
    StereoWav out;
    out.sampleRate = in.sampleRate;
    out.left.resize (in.left.size());
    out.right.resize (in.right.size());
    MackityRef ref;
    ref.prepare (static_cast<double> (in.sampleRate));
    ref.process (in.left.data(), in.right.data(), out.left.data(), out.right.data(),
                 static_cast<int> (in.left.size()), static_cast<double> (in.sampleRate), kA, kB);
    return out;
}

bool writePair (const std::string& dir, const std::string& stem, const StereoWav& dry)
{
    const StereoWav wet = renderRef (dry);
    const std::string inPath = dir + "/" + stem + ".wav";
    const std::string refPath = dir + "/" + stem + "_mackity_ref.wav";
    return writeWav32fStereo (inPath, dry) && writeWav32fStereo (refPath, wet);
}
} // namespace

int main (int argc, char** argv)
{
    const std::string dir = (argc > 1) ? argv[1] : "tests/fixtures";
    if (! writePair (dir, "sine_1khz_m6dbfs_48k", makeSine()))
    {
        std::cerr << "failed to write sine fixtures\n";
        return 1;
    }
    if (! writePair (dir, "drum_loop_excerpt_48k", makeDrumBurst()))
    {
        std::cerr << "failed to write drum fixtures\n";
        return 1;
    }
    std::cout << "wrote four fixtures to " << dir << '\n';
    return 0;
}

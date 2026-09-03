#include "MackityEngine.h"
#include "WavFile.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace
{
constexpr float kADefault = 0.1f;
constexpr float kBUnity = 1.0f;

int gFails = 0;

void expect (bool cond, const char* msg)
{
    if (! cond)
    {
        std::cerr << "FAIL: " << msg << '\n';
        ++gFails;
    }
}

std::string fixturePath (const char* name)
{
#ifdef MACH1_FIXTURES_DIR
    return std::string (MACH1_FIXTURES_DIR) + "/" + name;
#else
    return std::string ("tests/fixtures/") + name;
#endif
}

void fillSine (std::vector<float>& l, std::vector<float>& r, double sr, int n, float hz, float amp)
{
    l.resize (static_cast<size_t> (n));
    r.resize (static_cast<size_t> (n));
    const double twoPi = 6.283185307179586;
    for (int i = 0; i < n; ++i)
    {
        const float s = amp * static_cast<float> (std::sin (twoPi * static_cast<double> (hz) * i / sr));
        l[static_cast<size_t> (i)] = s;
        r[static_cast<size_t> (i)] = s * 0.8f;
    }
}

bool allZero (const std::vector<float>& a)
{
    for (float v : a)
        if (v != 0.0f)
            return false;
    return true;
}

bool allFinite (const std::vector<float>& a)
{
    for (float v : a)
        if (! std::isfinite (v))
            return false;
    return true;
}

void processAll (mach1::MackityEngine& eng, std::vector<float>& l, std::vector<float>& r,
                 float A, float B, bool autoGain = false, int block = 64)
{
    const int n = static_cast<int> (l.size());
    float* ins[2] = { l.data(), r.data() };
    float* outs[2] = { l.data(), r.data() };
    int i = 0;
    while (i < n)
    {
        const int chunk = std::min (block, n - i);
        float* inb[2] = { ins[0] + i, ins[1] + i };
        float* outb[2] = { outs[0] + i, outs[1] + i };
        eng.process (inb, outb, chunk, A, B, autoGain);
        i += chunk;
    }
}

void processIO (mach1::MackityEngine& eng,
                const std::vector<float>& inL, const std::vector<float>& inR,
                std::vector<float>& outL, std::vector<float>& outR,
                float A, float B, bool autoGain, int start, int count, int block = 64)
{
    outL.resize (inL.size());
    outR.resize (inR.size());
    const int n = static_cast<int> (inL.size());
    int i = start;
    const int end = std::min (n, start + count);
    while (i < end)
    {
        const int chunk = std::min (block, end - i);
        float* inb[2] = { const_cast<float*> (inL.data()) + i, const_cast<float*> (inR.data()) + i };
        float* outb[2] = { outL.data() + i, outR.data() + i };
        eng.process (inb, outb, chunk, A, B, autoGain);
        i += chunk;
    }
}

double stereoRms (const std::vector<float>& l, const std::vector<float>& r, int start, int count)
{
    double acc = 0.0;
    for (int i = 0; i < count; ++i)
    {
        const double xL = static_cast<double> (l[static_cast<size_t> (start + i)]);
        const double xR = static_cast<double> (r[static_cast<size_t> (start + i)]);
        acc += 0.5 * (xL * xL + xR * xR);
    }
    return std::sqrt (acc / static_cast<double> (count));
}

double db20 (double x)
{
    return 20.0 * std::log10 (std::max (x, 1.0e-20));
}

void fillEqualSine (std::vector<float>& l, std::vector<float>& r, double sr, int n, float hz, float amp)
{
    l.resize (static_cast<size_t> (n));
    r.resize (static_cast<size_t> (n));
    const double twoPi = 6.283185307179586;
    for (int i = 0; i < n; ++i)
    {
        const float s = amp * static_cast<float> (std::sin (twoPi * static_cast<double> (hz) * i / sr));
        l[static_cast<size_t> (i)] = s;
        r[static_cast<size_t> (i)] = s;
    }
}

float peakAbs (const std::vector<float>& l, const std::vector<float>& r, int start, int count)
{
    float p = 0.0f;
    for (int i = 0; i < count; ++i)
    {
        p = std::max (p, std::fabs (l[static_cast<size_t> (start + i)]));
        p = std::max (p, std::fabs (r[static_cast<size_t> (start + i)]));
    }
    return p;
}

double rmsErrRatio (const std::vector<float>& engL, const std::vector<float>& engR,
                    const std::vector<float>& refL, const std::vector<float>& refR)
{
    const size_t n = engL.size();
    double err = 0.0;
    double ref = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        const double eL = static_cast<double> (engL[i]) - static_cast<double> (refL[i]);
        const double eR = static_cast<double> (engR[i]) - static_cast<double> (refR[i]);
        const double rL = static_cast<double> (refL[i]);
        const double rR = static_cast<double> (refR[i]);
        err += eL * eL + eR * eR;
        ref += rL * rL + rR * rR;
    }
    if (ref <= 0.0)
        return std::numeric_limits<double>::infinity();
    return std::sqrt (err / static_cast<double> (n * 2)) / std::sqrt (ref / static_cast<double> (n * 2));
}

void characterPair (const char* inName, const char* refName)
{
    StereoWav inWav;
    StereoWav refWav;
    const std::string inPath = fixturePath (inName);
    const std::string refPath = fixturePath (refName);
    if (! readWavStereo (inPath, inWav))
    {
        expect (false, ("missing/unreadable fixture: " + inPath).c_str());
        return;
    }
    if (! readWavStereo (refPath, refWav))
    {
        expect (false, ("missing/unreadable fixture: " + refPath).c_str());
        return;
    }
    expect (inWav.sampleRate == 48000, "fixture sample rate is 48 kHz");
    expect (inWav.left.size() == refWav.left.size() && inWav.right.size() == refWav.right.size(),
            "fixture and ref lengths match");
    if (inWav.left.size() != refWav.left.size())
        return;

    mach1::MackityEngine eng;
    eng.prepare (48000.0);
    std::vector<float> l = inWav.left;
    std::vector<float> r = inWav.right;
    processAll (eng, l, r, kADefault, kBUnity, false);

    const double ratio = rmsErrRatio (l, r, refWav.left, refWav.right);
    std::cout << "RMS(err)/RMS(ref) " << inName << " = " << ratio << '\n';
    expect (ratio < 0.15, "character RMS ratio < 0.15");
}
} // namespace

int main()
{
    expect (mach1::MackityEngine::inTrimGain (0.1f) == 1.0f, "A=0.1 yields inGain == 1.0");
    expect (mach1::MackityEngine::inTrimGain (0.0f) == 0.0f, "A=0 yields inGain 0");
    expect (mach1::MackityEngine::inTrimGain (-1.0f) == 0.0f, "A clamp low");
    expect (mach1::MackityEngine::inTrimGain (2.0f) == 100.0f, "A clamp high → (1*10)^2");

    {
        constexpr int n = 48000;
        std::vector<float> l, r;
        fillSine (l, r, 48000.0, n, 1000.0f, std::pow (10.0f, -6.0f / 20.0f));
        mach1::MackityEngine eng;
        eng.prepare (48000.0);
        processAll (eng, l, r, 0.0f, 1.0f, false);
        expect (allZero (l) && allZero (r), "A=0 sine → all-zero output");
    }

    {
        constexpr int n = 4096;
        std::vector<float> l, r;
        fillSine (l, r, 48000.0, n, 1000.0f, 0.5f);
        mach1::MackityEngine eng;
        eng.prepare (48000.0);
        processAll (eng, l, r, 0.1f, 0.0f, false);
        expect (allZero (l) && allZero (r), "B=0 → all-zero output");
    }

    {
        constexpr int n = 256;
        std::vector<float> l, r, l2, r2;
        fillSine (l, r, 48000.0, n, 440.0f, 0.4f);
        l2 = l;
        r2 = r;
        std::vector<float> outSentL (16, 123.0f);
        std::vector<float> outSentR (16, -77.0f);

        mach1::MackityEngine a, b;
        a.prepare (48000.0);
        b.prepare (48000.0);

        float* inZ[2] = { l.data(), r.data() };
        float* outZ[2] = { outSentL.data(), outSentR.data() };
        a.process (inZ, outZ, 0, 0.1f, 1.0f, false);
        expect (outSentL[0] == 123.0f && outSentR[0] == -77.0f, "numSamples==0 does not write outputs");

        processAll (a, l, r, 0.1f, 1.0f, false);
        processAll (b, l2, r2, 0.1f, 1.0f, false);
        bool match = true;
        for (int i = 0; i < n; ++i)
            if (l[static_cast<size_t> (i)] != l2[static_cast<size_t> (i)]
                || r[static_cast<size_t> (i)] != r2[static_cast<size_t> (i)])
                match = false;
        expect (match, "empty block leaves state unchanged vs engine that never saw it");
    }

    {
        constexpr int n = 512;
        std::vector<float> l (n, 0.0f), r (n, 0.0f);
        mach1::MackityEngine eng;
        eng.prepare (48000.0);
        processAll (eng, l, r, 0.1f, 1.0f, false);
        expect (allZero (l) && allZero (r), "zero in → exact zero out (no dither)");
    }

    {
        const double rates[] = { 44100.0, 48000.0, 96000.0, 192000.0 };
        for (double sr : rates)
        {
            const int n = static_cast<int> (sr * 0.05);
            std::vector<float> l, r;
            fillSine (l, r, sr, n, 1000.0f, 0.5f);
            mach1::MackityEngine eng;
            eng.prepare (sr);
            processAll (eng, l, r, 0.3f, 1.0f, false);
            expect (allFinite (l) && allFinite (r), "SR sweep output is finite");
        }
    }

    {
        constexpr int n = 128;
        std::vector<float> l (n, 0.1f), r (n, -0.1f);
        l[10] = std::numeric_limits<float>::quiet_NaN();
        r[20] = std::numeric_limits<float>::infinity();
        l[30] = -std::numeric_limits<float>::infinity();
        mach1::MackityEngine eng;
        eng.prepare (48000.0);
        float* inb[2] = { l.data(), r.data() };
        float* outb[2] = { l.data(), r.data() };
        eng.process (inb, outb, n, 0.1f, 1.0f, false);
        expect (allFinite (l) && allFinite (r), "NaN/Inf samples sanitized");

        std::vector<float> l2, r2;
        fillSine (l2, r2, 48000.0, 256, 1000.0f, 0.4f);
        processAll (eng, l2, r2, 0.1f, 1.0f, false);
        expect (allFinite (l2) && allFinite (r2), "following finite block has no NaN/Inf");
    }

    characterPair ("sine_1khz_m6dbfs_48k.wav", "sine_1khz_m6dbfs_48k_mackity_ref.wav");
    characterPair ("drum_loop_excerpt_48k.wav", "drum_loop_excerpt_48k_mackity_ref.wav");

    // --- auto-gain ---
    constexpr double kSr = 48000.0;
    constexpr float kHz = 1000.0f;
    constexpr float kAmpM6 = 0.5011872336272722f; // −6 dBFS peak
    const int oneSec = 48000;
    const int settleN = oneSec + 4800; // 1.1 s
    const int measureN = 12000;        // 250 ms post-settle window
    const int preN = 4800;             // 100 ms at A=0.1

    auto matchDbError = [] (double outRms, double dryRms, double padLin) {
        return db20 (outRms / (dryRms * padLin));
    };

    {
        const int n = preN + settleN;
        std::vector<float> inL, inR, outL, outR;
        fillEqualSine (inL, inR, kSr, n, kHz, kAmpM6);
        mach1::MackityEngine eng;
        eng.prepare (kSr);
        processIO (eng, inL, inR, outL, outR, 0.1f, 1.0f, true, 0, preN);
        processIO (eng, inL, inR, outL, outR, 0.4f, 1.0f, true, preN, settleN);
        const int measStart = n - measureN;
        const double dry = stereoRms (inL, inR, measStart, measureN);
        const double wet = stereoRms (outL, outR, measStart, measureN);
        const double errDb = matchDbError (wet, dry, 1.0);
        std::cout << "AG B=1.0 dB error = " << errDb << '\n';
        expect (std::fabs (errDb) <= 1.5, "AG on B=1: output RMS within ±1.5 dB of dry RMS");
    }

    {
        const int n = preN + settleN;
        std::vector<float> inL, inR, outL, outR;
        fillEqualSine (inL, inR, kSr, n, kHz, kAmpM6);
        mach1::MackityEngine eng;
        eng.prepare (kSr);
        processIO (eng, inL, inR, outL, outR, 0.1f, 0.5f, true, 0, preN);
        processIO (eng, inL, inR, outL, outR, 0.4f, 0.5f, true, preN, settleN);
        const int measStart = n - measureN;
        const double dry = stereoRms (inL, inR, measStart, measureN);
        const double wet = stereoRms (outL, outR, measStart, measureN);
        const double errDb = matchDbError (wet, dry, 0.5);
        std::cout << "AG B=0.5 dB error vs dry+pad = " << errDb << '\n';
        expect (std::fabs (errDb) <= 1.5, "AG on B=0.5: output within ±1.5 dB of dry RMS + 20log10(0.5)");
    }

    {
        const int n = oneSec;
        std::vector<float> inL, inR, outL, outR;
        fillEqualSine (inL, inR, kSr, n, kHz, kAmpM6);
        mach1::MackityEngine eng;
        eng.prepare (kSr);
        processIO (eng, inL, inR, outL, outR, 0.4f, 1.0f, true, 0, n);

        std::vector<float> zL (static_cast<size_t> (oneSec), 0.0f), zR = zL, zOutL, zOutR;
        processIO (eng, zL, zR, zOutL, zOutR, 0.4f, 1.0f, true, 0, oneSec);

        const float quietAmp = std::pow (10.0f, -20.0f / 20.0f);
        std::vector<float> mL, mR, mOutL, mOutR;
        fillEqualSine (mL, mR, kSr, 512, kHz, quietAmp);
        processIO (eng, mL, mR, mOutL, mOutR, 0.4f, 1.0f, true, 0, 512);
        const float spikePeak = peakAbs (mOutL, mOutR, 0, 512);
        std::cout << "AG hold after silence, -20 dBFS follow peak = " << spikePeak << '\n';
        expect (spikePeak < 0.9f, "hold: silence does not run makeup away (no full-scale spike)");
    }

    {
        const float subHoldAmp = std::pow (10.0f, -90.0f / 20.0f);
        std::vector<float> inL, inR, outL, outR;
        fillEqualSine (inL, inR, kSr, oneSec, kHz, kAmpM6);
        mach1::MackityEngine eng;
        eng.prepare (kSr);
        processIO (eng, inL, inR, outL, outR, 0.4f, 1.0f, true, 0, oneSec);
        std::vector<float> qL, qR, qOutL, qOutR;
        fillEqualSine (qL, qR, kSr, oneSec, kHz, subHoldAmp);
        processIO (eng, qL, qR, qOutL, qOutR, 0.4f, 1.0f, true, 0, oneSec);
        const float quietAmp = std::pow (10.0f, -20.0f / 20.0f);
        std::vector<float> mL, mR, mOutL, mOutR;
        fillEqualSine (mL, mR, kSr, 512, kHz, quietAmp);
        processIO (eng, mL, mR, mOutL, mOutR, 0.4f, 1.0f, true, 0, 512);
        const float spikePeak = peakAbs (mOutL, mOutR, 0, 512);
        std::cout << "AG hold after −90 dBFS, -20 dBFS follow peak = " << spikePeak << '\n';
        expect (spikePeak < 0.9f, "hold: sub −80 dBFS dry does not run makeup away");
    }

    {
        const int first50ms = 2400;
        std::vector<float> inL, inR, outL, outR;
        fillEqualSine (inL, inR, kSr, first50ms, kHz, kAmpM6);
        mach1::MackityEngine eng;
        eng.prepare (kSr);
        processIO (eng, inL, inR, outL, outR, 0.4f, 1.0f, true, 0, first50ms);
        const float p = peakAbs (outL, outR, 0, first50ms);
        std::cout << "AG start-from-silence first 50 ms peak = " << p << '\n';
        expect (p < 1.0f, "start-from-silence: first 50 ms peak |x| < 1");
    }

    {
        const int n = preN + settleN;
        std::vector<float> inL, inR, outL, outR;
        fillEqualSine (inL, inR, kSr, n, kHz, kAmpM6);
        for (float& s : inR)
            s = 0.0f;
        mach1::MackityEngine eng;
        eng.prepare (kSr);
        processIO (eng, inL, inR, outL, outR, 0.4f, 1.0f, true, 0, n);
        const int measStart = n - measureN;
        float rPeak = peakAbs (outR, outR, measStart, measureN);
        const double dry = stereoRms (inL, inR, measStart, measureN);
        const double wet = stereoRms (outL, outR, measStart, measureN);
        const double errDb = matchDbError (wet, dry, 1.0);
        std::cout << "AG hard-pan R peak = " << rPeak << " dB error = " << errDb << '\n';
        expect (rPeak < 1.0e-6f, "shared L/R: hard-panned R stays silence");
        expect (std::fabs (errDb) <= 1.5, "shared L/R: stereo match uses combined RMS");
        expect (peakAbs (outL, outL, measStart, measureN) > 0.01f, "shared L/R: L is not silent");
    }

    {
        constexpr int n = 4096;
        std::vector<float> inL, inR;
        fillSine (inL, inR, kSr, n, kHz, 0.5f);
        std::vector<float> offL = inL, offR = inR;
        std::vector<float> onL = inL, onR = inR;
        std::vector<float> defL = inL, defR = inR;
        mach1::MackityEngine offEng, onEng, defEng;
        offEng.prepare (kSr);
        onEng.prepare (kSr);
        defEng.prepare (kSr);
        processAll (offEng, offL, offR, 0.4f, 1.0f, false);
        processAll (onEng, onL, onR, 0.4f, 1.0f, true);
        processAll (defEng, defL, defR, 0.4f, 1.0f); // default autoGain=false
        bool offMatchesDefault = true;
        bool onDiffers = false;
        for (int i = 0; i < n; ++i)
        {
            if (offL[static_cast<size_t> (i)] != defL[static_cast<size_t> (i)]
                || offR[static_cast<size_t> (i)] != defR[static_cast<size_t> (i)])
                offMatchesDefault = false;
            if (onL[static_cast<size_t> (i)] != offL[static_cast<size_t> (i)]
                || onR[static_cast<size_t> (i)] != offR[static_cast<size_t> (i)])
                onDiffers = true;
        }
        expect (offMatchesDefault, "AG off matches default pad-only process()");
        expect (onDiffers, "AG on changes output vs pad-only for A=0.4");
    }

    {
        // AG off must not update detectors: after long AG-off drive, first AG-on
        // samples should still start from makeup 1 (same first-block peak as a
        // freshly reset engine on the same input), aside from filter warmup.
        // Compare two warmed-up pad-only engines, then enable AG on one only
        // after matching filter state via identical AG-off processing.
        std::vector<float> inL, inR;
        fillEqualSine (inL, inR, kSr, oneSec, kHz, kAmpM6);
        mach1::MackityEngine a, b;
        a.prepare (kSr);
        b.prepare (kSr);
        std::vector<float> aOutL, aOutR, bOutL, bOutR;
        processIO (a, inL, inR, aOutL, aOutR, 0.4f, 1.0f, false, 0, oneSec);
        processIO (b, inL, inR, bOutL, bOutR, 0.4f, 1.0f, false, 0, oneSec);

        const int n64 = 64;
        std::vector<float> blockL (inL.begin(), inL.begin() + n64);
        std::vector<float> blockR (inR.begin(), inR.begin() + n64);
        std::vector<float> a1 = blockL, a2 = blockR, b1 = blockL, b2 = blockR;
        float* ain[2] = { a1.data(), a2.data() };
        float* aout[2] = { a1.data(), a2.data() };
        float* bin[2] = { b1.data(), b2.data() };
        float* bout[2] = { b1.data(), b2.data() };
        a.process (ain, aout, n64, 0.4f, 1.0f, true);
        b.process (bin, bout, n64, 0.4f, 1.0f, false);
        // If detectors ran while AG was off, makeup would already sit near the
        // settled target (~<< 1) and the first AG-on block would be much quieter
        // than continued pad-only. With detectors skipped, makeup is still 1
        // and the first AG-on block matches pad-only until RMS/slew move.
        const float agPeak = peakAbs (a1, a2, 0, n64);
        const float padPeak = peakAbs (b1, b2, 0, n64);
        std::cout << "AG-off skip detectors: first AG-on peak " << agPeak
                  << " vs continued pad-only " << padPeak << '\n';
        expect (padPeak > 1.0e-6f, "pad-only follow block has signal");
        expect (std::fabs (agPeak - padPeak) / padPeak < 0.15f,
                "AG off skipped RMS/makeup: first AG-on block still near pad-only (makeup ~1)");
    }

    {
        mach1::MackityEngine eng;
        std::vector<float> l (8, 0.2f), r (8, -0.2f);
        std::vector<float> sentL (8, 9.0f), sentR (8, 9.0f);
        float* inb[2] = { l.data(), r.data() };
        float* outb[2] = { sentL.data(), sentR.data() };
        eng.process (inb, outb, 8, 0.4f, 1.0f, true);
        expect (sentL[0] == 9.0f && sentR[0] == 9.0f, "unprepared process is a no-op");

        eng.prepare (kSr);
        float* nullIn[2] = { nullptr, r.data() };
        eng.process (nullIn, outb, 8, 0.4f, 1.0f, true);
        expect (sentL[0] == 9.0f, "null input buffer is a no-op");
        eng.process (nullptr, outb, 8, 0.4f, 1.0f, true);
        expect (sentL[0] == 9.0f, "null in pointer is a no-op");
        eng.process (inb, nullptr, 8, 0.4f, 1.0f, true);
        expect (sentL[0] == 9.0f, "null out pointer is a no-op");
    }

    {
        mach1::MackityEngine eng;
        eng.prepare (kSr);
        std::vector<float> inL, inR, outL, outR;
        fillEqualSine (inL, inR, kSr, 2048, kHz, kAmpM6);
        processIO (eng, inL, inR, outL, outR, 0.4f, 1.0f, true, 0, 2048);
        eng.reset();
        std::vector<float> zL (256, 0.0f), zR (256, 0.0f);
        processAll (eng, zL, zR, 0.4f, 1.0f, true);
        expect (allZero (zL) && allZero (zR), "reset() clears AG state: zeros stay zeros");
    }

    {
        mach1::MackityEngine eng;
        eng.prepare (8000.0);
        std::vector<float> inL (256, 0.2f), inR (256, 0.2f);
        processAll (eng, inL, inR, 0.4f, 1.0f, false);
        expect (allFinite (inL) && allFinite (inR), "prepare(8 kHz) biquad stays finite");
    }

    {
        mach1::MackityEngine eng;
        eng.prepare (kSr);
        std::vector<float> inL (4800, 0.5f), inR (4800, 0.5f); // DC, wet RMS collapses after HP
        processAll (eng, inL, inR, 0.4f, 1.0f, true);
        expect (allFinite (inL) && allFinite (inR), "AG on DC does not explode makeup");
        float peak = 0.0f;
        for (float x : inL)
            peak = std::max (peak, std::fabs (x));
        expect (peak < 200.0f, "AG makeup clamp keeps DC output bounded");
    }

    if (gFails != 0)
    {
        std::cerr << gFails << " assertion(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "mackity engine tests passed\n";
    return EXIT_SUCCESS;
}

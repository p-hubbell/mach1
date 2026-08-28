#include "MackityEngine.h"

#include <cmath>
#include <utility>

namespace mach1
{
namespace
{
constexpr double kRefSampleRate = 44100.0;
constexpr double kIirNumA = 0.001860867;
constexpr double kIirNumB = 0.000287496;
constexpr double kLowpassHz = 19160.0;
constexpr double kQa = 0.431684981684982;
constexpr double kQb = 1.1582298;
constexpr float kShape = 0.1768f;
constexpr double kRmsWindowSec = 0.080;
constexpr double kMakeupSlewSec = 0.300;
constexpr float kDryHoldLin = 1.0e-4f; // −80 dBFS
constexpr float kWetRmsFloor = 1.0e-12f;

inline float sanitize (float x) noexcept
{
    // Finite (including 0) → x*0 == 0; NaN/Inf → NaN, comparison false.
    return (x * 0.0f == 0.0f) ? x : 0.0f;
}

inline float saturate (float x) noexcept
{
    if (x > 1.0f)
        x = 1.0f;
    if (x < -1.0f)
        x = -1.0f;
    const float x2 = x * x;
    const float x4 = x2 * x2;
    const float x5 = x4 * x;
    return x - x5 * kShape;
}

void setLowpassCoeffs (float& a0, float& a1, float& a2, float& b1, float& b2,
                       double sampleRate, double q) noexcept
{
    const double freq = kLowpassHz / sampleRate;
    const double K = std::tan (3.14159265358979323846 * freq);
    const double norm = 1.0 / (1.0 + K / q + K * K);
    const double a0d = K * K * norm;
    a0 = static_cast<float> (a0d);
    a1 = static_cast<float> (2.0 * a0d);
    a2 = static_cast<float> (a0d);
    b1 = static_cast<float> (2.0 * (K * K - 1.0) * norm);
    b2 = static_cast<float> ((1.0 - K / q + K * K) * norm);
}
} // namespace

float MackityEngine::clamp01 (float x) noexcept
{
    if (! std::isfinite (x))
        return 0.0f;
    if (x < 0.0f)
        return 0.0f;
    if (x > 1.0f)
        return 1.0f;
    return x;
}

float MackityEngine::inTrimGain (float A) noexcept
{
    const float t = clamp01 (A) * 10.0f;
    return t * t;
}

void MackityEngine::updateCoeffs (double sampleRate) noexcept
{
    const double overallscale = sampleRate / kRefSampleRate;
    iirAmountA_ = static_cast<float> (kIirNumA / overallscale);
    iirAmountB_ = static_cast<float> (kIirNumB / overallscale);
    setLowpassCoeffs (lpA_.a0, lpA_.a1, lpA_.a2, lpA_.b1, lpA_.b2, sampleRate, kQa);
    setLowpassCoeffs (lpB_.a0, lpB_.a1, lpB_.a2, lpB_.b1, lpB_.b2, sampleRate, kQb);
    rmsAlpha_ = static_cast<float> (1.0 - std::exp (-1.0 / (kRmsWindowSec * sampleRate)));
    makeupAlpha_ = static_cast<float> (1.0 - std::exp (-1.0 / (kMakeupSlewSec * sampleRate)));
}

void MackityEngine::prepare (double sampleRate)
{
    if (! (sampleRate > 0.0) || ! std::isfinite (sampleRate))
    {
        prepared_ = false;
        sampleRate_ = 0.0;
        return;
    }

    if (sampleRate != sampleRate_)
    {
        updateCoeffs (sampleRate);
        sampleRate_ = sampleRate;
    }

    prepared_ = true;
    reset();
}

void MackityEngine::reset() noexcept
{
    iirAL_ = iirAR_ = iirBL_ = iirBR_ = 0.0f;
    lpA_.reset();
    lpB_.reset();
    dryMs_ = 0.0f;
    wetMs_ = 0.0f;
    makeup_ = 1.0f;
}

void MackityEngine::process (float** in, float** out, int numSamples, float A, float B,
                             bool autoGain) noexcept
{
    if (! prepared_ || numSamples <= 0 || in == nullptr || out == nullptr)
        return;
    if (in[0] == nullptr || in[1] == nullptr || out[0] == nullptr || out[1] == nullptr)
        return;

    const float inGain = inTrimGain (A);
    const float outPad = clamp01 (B);

    float* inL = in[0];
    float* inR = in[1];
    float* outL = out[0];
    float* outR = out[1];

    const float oneMinusA = 1.0f - iirAmountA_;
    const float oneMinusB = 1.0f - iirAmountB_;
    const float iirA = iirAmountA_;
    const float iirB = iirAmountB_;
    const bool applyIn = inGain != 1.0f;
    const bool applyOut = outPad != 1.0f;

    bool blockFinite = true;
    for (int i = 0; i < numSamples; ++i)
    {
        if (inL[i] * 0.0f != 0.0f || inR[i] * 0.0f != 0.0f)
        {
            blockFinite = false;
            break;
        }
    }

    auto step = [&] (float xL, float xR) noexcept
    {
        iirAL_ = (iirAL_ * oneMinusA) + (xL * iirA);
        xL -= iirAL_;
        iirAR_ = (iirAR_ * oneMinusA) + (xR * iirA);
        xR -= iirAR_;
        if (applyIn)
        {
            xL *= inGain;
            xR *= inGain;
        }
        xL = lpA_.tickL (xL);
        xR = lpA_.tickR (xR);
        xL = saturate (xL);
        xR = saturate (xR);
        xL = lpB_.tickL (xL);
        xR = lpB_.tickR (xR);
        iirBL_ = (iirBL_ * oneMinusB) + (xL * iirB);
        xL -= iirBL_;
        iirBR_ = (iirBR_ * oneMinusB) + (xR * iirB);
        xR -= iirBR_;
        return std::pair<float, float> { xL, xR };
    };

    // Unity in-trim / unity pad / AG off / finite block: same work as Mackity's
    // taken branches at A=0.1, B=1.0 (the CPU bar), without per-sample sanitize.
    if (! autoGain && ! applyIn && ! applyOut && blockFinite)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float xL = inL[i];
            float xR = inR[i];
            iirAL_ = (iirAL_ * oneMinusA) + (xL * iirA);
            xL -= iirAL_;
            iirAR_ = (iirAR_ * oneMinusA) + (xR * iirA);
            xR -= iirAR_;
            xL = lpA_.tickL (xL);
            xR = lpA_.tickR (xR);
            xL = saturate (xL);
            xR = saturate (xR);
            xL = lpB_.tickL (xL);
            xR = lpB_.tickR (xR);
            iirBL_ = (iirBL_ * oneMinusB) + (xL * iirB);
            xL -= iirBL_;
            iirBR_ = (iirBR_ * oneMinusB) + (xR * iirB);
            xR -= iirBR_;
            outL[i] = xL;
            outR[i] = xR;
        }
        return;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        const float dryL = blockFinite ? inL[i] : sanitize (inL[i]);
        const float dryR = blockFinite ? inR[i] : sanitize (inR[i]);
        auto yr = step (dryL, dryR);
        float yL = yr.first;
        float yR = yr.second;

        if (autoGain)
        {
            const float drySq = 0.5f * (dryL * dryL + dryR * dryR);
            const float wetSq = 0.5f * (yL * yL + yR * yR);
            dryMs_ += rmsAlpha_ * (drySq - dryMs_);
            wetMs_ += rmsAlpha_ * (wetSq - wetMs_);

            const float dryRms = std::sqrt (dryMs_);
            if (dryRms >= kDryHoldLin)
            {
                const float wetRms = std::sqrt (wetMs_);
                const float denom = wetRms > kWetRmsFloor ? wetRms : kWetRmsFloor;
                const float target = dryRms / denom;
                makeup_ += makeupAlpha_ * (target - makeup_);
            }

            yL *= makeup_;
            yR *= makeup_;
        }

        if (applyOut)
        {
            yL *= outPad;
            yR *= outPad;
        }
        outL[i] = yL;
        outR[i] = yR;
    }
}

} // namespace mach1

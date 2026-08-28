#pragma once

namespace mach1
{

class MackityEngine
{
public:
    void prepare (double sampleRate);
    void reset() noexcept;

    // Stereo float buffers. A/B are In Trim / Out Pad in 0…1 (clamped).
    // autoGain: dry/wet RMS makeup after DC-B, before pad. Default off (pad-only).
    // No heap allocation. numSamples == 0 returns without writing outputs.
    void process (float** in, float** out, int numSamples, float A, float B,
                  bool autoGain = false) noexcept;

    static float clamp01 (float x) noexcept;
    static float inTrimGain (float A) noexcept;

private:
    void updateCoeffs (double sampleRate) noexcept;

    bool prepared_ = false;
    double sampleRate_ = 0.0;

    float iirAmountA_ = 0.0f;
    float iirAmountB_ = 0.0f;

    float iirAL_ = 0.0f;
    float iirAR_ = 0.0f;
    float iirBL_ = 0.0f;
    float iirBR_ = 0.0f;

    struct Biquad
    {
        float a0 = 0.0f, a1 = 0.0f, a2 = 0.0f, b1 = 0.0f, b2 = 0.0f;
        float xz1L = 0.0f, xz2L = 0.0f, yz1L = 0.0f, yz2L = 0.0f;
        float xz1R = 0.0f, xz2R = 0.0f, yz1R = 0.0f, yz2R = 0.0f;

        void reset() noexcept
        {
            xz1L = xz2L = yz1L = yz2L = 0.0f;
            xz1R = xz2R = yz1R = yz2R = 0.0f;
        }

        float tickL (float x) noexcept
        {
            const float y = a0 * x + a1 * xz1L + a2 * xz2L - b1 * yz1L - b2 * yz2L;
            xz2L = xz1L;
            xz1L = x;
            yz2L = yz1L;
            yz1L = y;
            return y;
        }

        float tickR (float x) noexcept
        {
            const float y = a0 * x + a1 * xz1R + a2 * xz2R - b1 * yz1R - b2 * yz2R;
            xz2R = xz1R;
            xz1R = x;
            yz2R = yz1R;
            yz1R = y;
            return y;
        }
    };

    Biquad lpA_;
    Biquad lpB_;

    float rmsAlpha_ = 0.0f;
    float makeupAlpha_ = 0.0f;
    float dryMs_ = 0.0f;
    float wetMs_ = 0.0f;
    float makeup_ = 1.0f;
};

} // namespace mach1

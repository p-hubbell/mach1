#pragma once

// Compiled Mackity process-loop opponent (double path), matching
// tests/generate_mackity_fixtures.cpp. Omits TPDF/frexpf dither and denormal
// noise injection so refs stay usable; topology/constants match MackityProc.cpp.

struct MackityRef
{
    double iirSampleAL = 0.0;
    double iirSampleBL = 0.0;
    double iirSampleAR = 0.0;
    double iirSampleBR = 0.0;
    double biquadA[15] {};
    double biquadB[15] {};

    void prepare (double sampleRate);
    void process (const float* inL, const float* inR, float* outL, float* outR, int n,
                  double sampleRate, float A, float B);
};

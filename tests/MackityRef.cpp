#include "MackityRef.h"

#include <cmath>

void MackityRef::prepare (double sampleRate)
{
    constexpr double kPi = 3.14159265358979323846;
    biquadB[0] = biquadA[0] = 19160.0 / sampleRate;
    biquadA[1] = 0.431684981684982;
    biquadB[1] = 1.1582298;

    double K = std::tan (kPi * biquadA[0]);
    double norm = 1.0 / (1.0 + K / biquadA[1] + K * K);
    biquadA[2] = K * K * norm;
    biquadA[3] = 2.0 * biquadA[2];
    biquadA[4] = biquadA[2];
    biquadA[5] = 2.0 * (K * K - 1.0) * norm;
    biquadA[6] = (1.0 - K / biquadA[1] + K * K) * norm;

    K = std::tan (kPi * biquadB[0]);
    norm = 1.0 / (1.0 + K / biquadB[1] + K * K);
    biquadB[2] = K * K * norm;
    biquadB[3] = 2.0 * biquadB[2];
    biquadB[4] = biquadB[2];
    biquadB[5] = 2.0 * (K * K - 1.0) * norm;
    biquadB[6] = (1.0 - K / biquadB[1] + K * K) * norm;
}

void MackityRef::process (const float* inL, const float* inR, float* outL, float* outR, int n,
                          double sampleRate, float A, float B)
{
    const double overallscale = sampleRate / 44100.0;
    double inTrim = static_cast<double> (A) * 10.0;
    const double outPad = static_cast<double> (B);
    inTrim *= inTrim;
    const double iirAmountA = 0.001860867 / overallscale;
    const double iirAmountB = 0.000287496 / overallscale;

    for (int i = 0; i < n; ++i)
    {
        double inputSampleL = static_cast<double> (inL[i]);
        double inputSampleR = static_cast<double> (inR[i]);

        if (std::fabs (iirSampleAL) < 1.18e-37)
            iirSampleAL = 0.0;
        iirSampleAL = (iirSampleAL * (1.0 - iirAmountA)) + (inputSampleL * iirAmountA);
        inputSampleL -= iirSampleAL;
        if (std::fabs (iirSampleAR) < 1.18e-37)
            iirSampleAR = 0.0;
        iirSampleAR = (iirSampleAR * (1.0 - iirAmountA)) + (inputSampleR * iirAmountA);
        inputSampleR -= iirSampleAR;

        if (inTrim != 1.0)
        {
            inputSampleL *= inTrim;
            inputSampleR *= inTrim;
        }

        double outSampleL = biquadA[2] * inputSampleL + biquadA[3] * biquadA[7] + biquadA[4] * biquadA[8]
                            - biquadA[5] * biquadA[9] - biquadA[6] * biquadA[10];
        biquadA[8] = biquadA[7];
        biquadA[7] = inputSampleL;
        inputSampleL = outSampleL;
        biquadA[10] = biquadA[9];
        biquadA[9] = inputSampleL;

        double outSampleR = biquadA[2] * inputSampleR + biquadA[3] * biquadA[11] + biquadA[4] * biquadA[12]
                            - biquadA[5] * biquadA[13] - biquadA[6] * biquadA[14];
        biquadA[12] = biquadA[11];
        biquadA[11] = inputSampleR;
        inputSampleR = outSampleR;
        biquadA[14] = biquadA[13];
        biquadA[13] = inputSampleR;

        if (inputSampleL > 1.0)
            inputSampleL = 1.0;
        if (inputSampleL < -1.0)
            inputSampleL = -1.0;
        {
            const double x2 = inputSampleL * inputSampleL;
            inputSampleL -= (x2 * x2 * inputSampleL) * 0.1768;
        }
        if (inputSampleR > 1.0)
            inputSampleR = 1.0;
        if (inputSampleR < -1.0)
            inputSampleR = -1.0;
        {
            const double x2 = inputSampleR * inputSampleR;
            inputSampleR -= (x2 * x2 * inputSampleR) * 0.1768;
        }

        outSampleL = biquadB[2] * inputSampleL + biquadB[3] * biquadB[7] + biquadB[4] * biquadB[8]
                     - biquadB[5] * biquadB[9] - biquadB[6] * biquadB[10];
        biquadB[8] = biquadB[7];
        biquadB[7] = inputSampleL;
        inputSampleL = outSampleL;
        biquadB[10] = biquadB[9];
        biquadB[9] = inputSampleL;

        outSampleR = biquadB[2] * inputSampleR + biquadB[3] * biquadB[7] + biquadB[4] * biquadB[8]
                     - biquadB[5] * biquadB[9] - biquadB[6] * biquadB[10];
        biquadB[12] = biquadB[11];
        biquadB[11] = inputSampleR;
        inputSampleR = outSampleR;
        biquadB[14] = biquadB[13];
        biquadB[13] = inputSampleR;

        if (std::fabs (iirSampleBL) < 1.18e-37)
            iirSampleBL = 0.0;
        iirSampleBL = (iirSampleBL * (1.0 - iirAmountB)) + (inputSampleL * iirAmountB);
        inputSampleL -= iirSampleBL;
        if (std::fabs (iirSampleBR) < 1.18e-37)
            iirSampleBR = 0.0;
        iirSampleBR = (iirSampleBR * (1.0 - iirAmountB)) + (inputSampleR * iirAmountB);
        inputSampleR -= iirSampleBR;

        if (outPad != 1.0)
        {
            inputSampleL *= outPad;
            inputSampleR *= outPad;
        }

        outL[i] = static_cast<float> (inputSampleL);
        outR[i] = static_cast<float> (inputSampleR);
    }
}

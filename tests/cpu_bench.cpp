#include "MackityEngine.h"
#include "MackityRef.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace
{
constexpr double kSr = 48000.0;
constexpr int kBlock = 64;
constexpr int kSeconds = 30;
constexpr int kRuns = 5;
constexpr int kSerialEngines = 24;
constexpr float kA = 0.1f;
constexpr float kB = 1.0f;
constexpr int kTotalSamples = static_cast<int> (kSr) * kSeconds;
constexpr double kPeriodBudgetSec = static_cast<double> (kBlock) / kSr;
constexpr const char* kReaperPath = "/Applications/REAPER.app";

using Clock = std::chrono::high_resolution_clock;

bool pathExists (const char* path)
{
    struct stat st;
    return ::stat (path, &st) == 0;
}

void fillSineM6dBFS (std::vector<float>& l, std::vector<float>& r)
{
    l.resize (static_cast<size_t> (kTotalSamples));
    r.resize (static_cast<size_t> (kTotalSamples));
    const float amp = std::pow (10.0f, -6.0f / 20.0f);
    constexpr double twoPi = 6.283185307179586;
    for (int i = 0; i < kTotalSamples; ++i)
    {
        const float s = amp * static_cast<float> (std::sin (twoPi * 1000.0 * static_cast<double> (i) / kSr));
        l[static_cast<size_t> (i)] = s;
        r[static_cast<size_t> (i)] = s;
    }
}

float sinkBuffers (const std::vector<float>& l, const std::vector<float>& r)
{
    float acc = 0.0f;
    for (size_t i = 0; i < l.size(); i += 64)
        acc += l[i] + r[i];
    return acc;
}

double medianSeconds (std::array<double, kRuns> times)
{
    std::sort (times.begin(), times.end());
    return times[kRuns / 2];
}

double timeMackityLoop (const std::vector<float>& inL, const std::vector<float>& inR,
                        std::vector<float>& outL, std::vector<float>& outR, float& sink)
{
    MackityRef ref;
    ref.prepare (kSr);
    const auto t0 = Clock::now();
    for (int off = 0; off < kTotalSamples; off += kBlock)
    {
        ref.process (inL.data() + off, inR.data() + off, outL.data() + off, outR.data() + off,
                     kBlock, kSr, kA, kB);
    }
    const auto t1 = Clock::now();
    sink += sinkBuffers (outL, outR);
    return std::chrono::duration<double> (t1 - t0).count();
}

double timeMach1 (const std::vector<float>& inL, const std::vector<float>& inR,
                  std::vector<float>& outL, std::vector<float>& outR, bool autoGain, float& sink)
{
    mach1::MackityEngine eng;
    eng.prepare (kSr);
    const auto t0 = Clock::now();
    for (int off = 0; off < kTotalSamples; off += kBlock)
    {
        float* inb[2] = { const_cast<float*> (inL.data()) + off, const_cast<float*> (inR.data()) + off };
        float* outb[2] = { outL.data() + off, outR.data() + off };
        eng.process (inb, outb, kBlock, kA, kB, autoGain);
    }
    const auto t1 = Clock::now();
    sink += sinkBuffers (outL, outR);
    return std::chrono::duration<double> (t1 - t0).count();
}

int runOfflineHarness()
{
    std::vector<float> inL, inR, outL, outR;
    fillSineM6dBFS (inL, inR);
    outL.resize (inL.size());
    outR.resize (inR.size());

    float sink = 0.0f;
    std::array<double, kRuns> mackityTimes {};
    std::array<double, kRuns> mach1OffTimes {};
    std::array<double, kRuns> mach1OnTimes {};

    for (int run = 0; run < kRuns; ++run)
        mackityTimes[static_cast<size_t> (run)] = timeMackityLoop (inL, inR, outL, outR, sink);
    for (int run = 0; run < kRuns; ++run)
        mach1OffTimes[static_cast<size_t> (run)] = timeMach1 (inL, inR, outL, outR, false, sink);
    for (int run = 0; run < kRuns; ++run)
        mach1OnTimes[static_cast<size_t> (run)] = timeMach1 (inL, inR, outL, outR, true, sink);

    const double medMackity = medianSeconds (mackityTimes);
    const double medOff = medianSeconds (mach1OffTimes);
    const double medOn = medianSeconds (mach1OnTimes);
    const double ratioOff = medOff > 0.0 ? medMackity / medOff : 0.0;
    const double ratioOn = medOn > 0.0 ? medMackity / medOn : 0.0;

    std::cout << "config: 48 kHz / 64-sample, A=0.1 B=1.0, 1 kHz -6 dBFS sine, "
              << kSeconds << " s audio, median of " << kRuns << " runs\n";
    std::cout << "median_time_Mackity_loop_s " << medMackity << '\n';
    std::cout << "median_time_mach1_AG_off_s " << medOff << '\n';
    std::cout << "median_time_mach1_AG_on_s " << medOn << '\n';
    std::cout << "ratio_AG_off " << ratioOff << '\n';
    std::cout << "ratio_AG_on " << ratioOn << '\n';
    std::cout << "sink " << sink << '\n';

    int fails = 0;
    if (! (ratioOff >= 2.0))
    {
        std::cerr << "FAIL: AG-off ratio " << ratioOff << " is < 2\n";
        ++fails;
    }
    if (! (ratioOn > 1.0))
    {
        std::cerr << "FAIL: AG-on ratio " << ratioOn << " is not > 1\n";
        ++fails;
    }

    // 24 serial instances: one thread, same 64-sample input block to each engine
    // before advancing. Overrun = wall time inside the 24 process() calls for that
    // block (audio callback budget), not a sleep-loop period.
    constexpr bool kSerialAutoGain = true;
    std::array<mach1::MackityEngine, kSerialEngines> engines;
    for (auto& e : engines)
        e.prepare (kSr);

    std::vector<float> scratchL (static_cast<size_t> (kBlock));
    std::vector<float> scratchR (static_cast<size_t> (kBlock));
    auto runSerialBlock = [&] (int off)
    {
        float* inb[2] = { inL.data() + off, inR.data() + off };
        float* outb[2] = { scratchL.data(), scratchR.data() };
        const auto t0 = Clock::now();
        for (int e = 0; e < kSerialEngines; ++e)
            engines[static_cast<size_t> (e)].process (inb, outb, kBlock, kA, kB, kSerialAutoGain);
        const auto t1 = Clock::now();
        sink += scratchL[0] + scratchR[0];
        return std::chrono::duration<double> (t1 - t0).count();
    };

    // Warm the 24 instances (callback already running) so cold I-cache is not
    // scored as an audio-period overrun. Then measure ≥ 30 s of audio.
    constexpr int kWarmupBlocks = 512;
    for (int b = 0; b < kWarmupBlocks; ++b)
        (void) runSerialBlock ((b * kBlock) % kTotalSamples);

    int overruns = 0;
    double maxPeriod = 0.0;
    double sumProcess = 0.0;
    const int nBlocks = kTotalSamples / kBlock;

    for (int b = 0; b < nBlocks; ++b)
    {
        const double period = runSerialBlock (b * kBlock);
        sumProcess += period;
        if (period > maxPeriod)
            maxPeriod = period;
        if (period >= kPeriodBudgetSec)
            ++overruns;
    }

    std::cout << "serial_engines " << kSerialEngines << '\n';
    std::cout << "serial_auto_gain " << (kSerialAutoGain ? "on" : "off") << '\n';
    std::cout << "period_budget_s " << kPeriodBudgetSec << '\n';
    std::cout << "max_period_s " << maxPeriod << '\n';
    std::cout << "sum_process_s " << sumProcess << '\n';
    std::cout << "audio_budget_24x_30s_s " << (30.0) << '\n';
    std::cout << "overruns " << overruns << '\n';
    std::cout << "sink_final " << sink << '\n';

    if (overruns != 0)
    {
        std::cerr << "FAIL: " << overruns << " overrun(s); any period >= " << kPeriodBudgetSec << " s\n";
        ++fails;
    }

    if (fails == 0)
        std::cout << "PASS offline+harness\n";
    return fails == 0 ? 0 : 1;
}

bool mackityPluginPresent()
{
    const char* candidates[] = {
        "/Library/Audio/Plug-Ins/VST3/Mackity.vst3",
        "/Library/Audio/Plug-Ins/Components/Mackity.component",
        "/Library/Audio/Plug-Ins/VST3/Airwindows Mackity.vst3",
        "/Library/Audio/Plug-Ins/Components/Airwindows Mackity.component",
        nullptr
    };
    const char* home = std::getenv ("HOME");
    std::string userVst3;
    std::string userAu;
    if (home != nullptr)
    {
        userVst3 = std::string (home) + "/Library/Audio/Plug-Ins/VST3/Mackity.vst3";
        userAu = std::string (home) + "/Library/Audio/Plug-Ins/Components/Mackity.component";
    }
    for (const char** p = candidates; *p != nullptr; ++p)
    {
        if (pathExists (*p))
            return true;
    }
    if (! userVst3.empty() && pathExists (userVst3.c_str()))
        return true;
    if (! userAu.empty() && pathExists (userAu.c_str()))
        return true;
    return false;
}

// Reaper E2E is only checkable when REAPER.app is installed. This case must not
// be treated as a pass for the Reaper AC when unverified.
int runReaperCase()
{
    if (! pathExists (kReaperPath))
    {
        std::cout << "FAIL-UNVERIFIED\n";
        return 2;
    }

    if (! mackityPluginPresent())
    {
        std::cerr << "FAIL: Reaper is present but Mackity is missing for the relative CPU/dropout comparison\n";
        return 1;
    }

    std::cerr << "FAIL: Reaper is present and Mackity was found, but automated 24-insert / 30 s "
                 "playback CPU-or-dropout comparison is not implemented in this harness\n";
    return 1;
}
} // namespace

int main (int argc, char** argv)
{
    if (argc > 1 && std::strcmp (argv[1], "--reaper") == 0)
        return runReaperCase();
    return runOfflineHarness();
}

# Plan: mach1

## Scope

**Mode: Hold Scope.** The brief already named a coherent v1: this slam, cheap enough to stack, plus a custom editor and auto-gain. Expanding (dry/wet, extra drive, MackEQ, WebView-as-product, other formats) would relitigate size. Cutting the editor or auto-gain would drop half of the stated problem.

**In**
- Stereo **VST3 + AU**, **macOS**, local developer install.
- **Native C++** DSP that keeps Mackity’s topology and the two mapped controls **In Trim (A)** and **Out Pad (B)**.
- **CPU rewrite** of that loop (not a new saturator): no per-sample `pow`/`frexpf` dither, no `tan()` every buffer, no duplicated float/double process paths.
- **Auto-gain**, user-defeatable, loudness match — not a compressor.
- **Custom editor**: labeled In Trim / Out Pad / auto-gain, in/out meters, dark industrial box. Not a console skin.
- **MIT attribution** in repo/About; product name is mach1, not Airwindows/Mackity branding.
- **Character gate** with auto-gain off: swap still “is that plugin.” Bit-identical is out.

**Out**
- MackEQ / desk EQ, full mixer, noise/hiss, Elementary/JS as the audio path.
- Windows, CLAP, AAX, presets, dry/wet, extra drive knob, default oversampling, dual-mono/M-S, analyzers, sidechain.

**Deferred**
- Windows/CLAP, optional oversampling, factory presets, desk-EQ sibling, Elementary for extra FX/analysis only.

**Premises to treat as engineering risk, not extra features**
- Vibe-match needs a fixed A/B protocol (auto-gain off, fixture WAVs).
- Removing dither/denormal noise may be part of Mackity’s texture; if the 15% RMS-error bound fails on transients, that is a fidelity-vs-CPU call — do not silently add dither or oversampling.
- JUCE is **GPL unless licensed**. Mackity is MIT. User must pick: JUCE paid license, GPL mach1, or a later wrapper swap. Plan proceeds on JUCE as the wrapper; license is a packaging constraint, not a DSP task.

## Architecture

**Stack freeze:** JUCE AudioProcessor + **native JUCE editor** (not WebView/React). Three controls plus meters do not justify a JS runtime, WebKit/AU bugs, or meter IPC. Visual language is `LookAndFeel`, not a web app.

**Modules**
- **DSP (`MackityEngine`)** — host-free C++. Topology: DC-block IIR A → In Trim `(A*10)²` → biquad LP ~19160 Hz (Q `0.431…`) → clip `±1` then `x − x⁵·0.1768` → biquad LP (Q `1.158…`) → DC-block IIR B → **[auto-gain makeup if on]** → Out Pad. No alloc in `process()`. Coeffs only on sample-rate change. One float realtime path. FTZ/DAZ; digital silence stays silence.
- **Processor** — JUCE adapter: buses (stereo required; **mono 1-in/1-out must not crash** — L path only, no upmix), APVTS (`In Trim`, `Out Pad`, `AutoGain`), prepare/reset/bypass, meter atomics.
- **Editor** — knobs, auto-gain, meters, About. No DSP.
- **Mackity source** — reference + attribution only. Do not link Airwindows binaries. Character tests use **repo fixture WAVs**.

**Parameter contract**

| mach1 | Mackity | Notes |
|---|---|---|
| In Trim | `A` default `0.1` | `inGain = (A*10)²` |
| Out Pad | `B` default `1.0` | linear, **always last** |
| Auto-gain | — | bool, default **on** for new instances; **off** when loading a 2-param legacy chunk |

**Auto-gain freeze:** stereo dry RMS vs wet RMS (after topology, before makeup/pad), ~80 ms leaky window, makeup `dry/max(wet, floor)` smoothed ~300 ms one-pole, shared L/R. Hold makeup when dry RMS < −80 dBFS. Off = skip detectors. Pad remains a linear offset on top of makeup.

**CPU freeze:** ≥**2×** cheaper than Mackity offline at 48 kHz / 64-sample (median of 5, auto-gain off for the fair compare). Host: **24** stereo instances in Reaper, 48 kHz / 64, no dropouts 30 s, CPU ≤ half of Mackity (or Mackity dropouts). SIMD optional; scalar rewrite is the bar.

**Audio flow (happy):** in → dry RMS? → topology → wet RMS? → makeup? → pad → out + meter peaks.

**Nil / empty / error:** null or unprepared → no-op; `numSamples==0` → return, state unchanged; NaN/Inf sample → replace with 0, do not reset the whole voice; params clamped to `0…1`.

## Test Matrix

| Area | Unit | Integration | E2E / host |
|---|---|---|---|
| Engine topology | Impulse/sine, A/B extremes, 0-length, silence stays 0, SR 44.1–192, NaN isolation, character vs fixtures (`RMS(err)/RMS(ref) < 0.15`, AG off) | — | Listening: “still that plugin” |
| Auto-gain | Settle ±1.5 dB of dry RMS; pad offset; silence hold; no blast on start; shared L/R makeup; AG-off skips RMS | — | Drive In Trim, level holds |
| Processor | — | 3-param save/load; 2-float chunk → AG off; bypass copy; reset clears; clamp | Automation in Reaper |
| CPU | Bench vs Mackity 2×; N=24 in harness; missing Mackity **fails** relative bar | — | Reaper 24 instances |
| Editor | — | — | Labels, meters, About credit, open in Logic + Reaper, close-during-play |
| Hosts | — | `auval` | Logic stereo + **mono insert**; Reaper VST3 scan/insert/save |
| Package | — | arm64 install paths, NOTICE | — |

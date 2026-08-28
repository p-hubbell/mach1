---
status: reviewed
---

# Host-free Mackity-topology engine + character fixtures

## Goal

A host-free C++ engine applies Mackity’s saturator topology (no auto-gain) at In Trim `A` and Out Pad `B`, and matches Mackity character on repo fixtures at 48 kHz within `RMS(err)/RMS(ref) < 0.15`, without Mackity’s per-buffer `tan` rebuilds or per-sample `pow`/`frexpf` dither.

## Acceptance Criteria

- [ ] A host-free engine (no JUCE types, includes, or linkage in its headers/sources) processes stereo float buffers with parameters `A` and `B` in `0…1` (clamped). It is callable from a test binary without a plugin host.
- [ ] One audio block applies stages in this order, using Mackity’s constants from `/Users/seto/projects/airwindows/plugins/MacSignedVST/Mackity/source/MackityProc.cpp`: DC-block IIR A (`iirAmountA = 0.001860867 / (sampleRate/44100)`), In Trim `inGain = (A*10)²`, biquad lowpass `fc = 19160 Hz` `Q = 0.431684981684982`, clip to `±1` then `x − x⁵·0.1768`, biquad lowpass `fc = 19160 Hz` `Q = 1.1582298`, DC-block IIR B (`iirAmountB = 0.000287496 / (sampleRate/44100)`), Out Pad `× B`. L and R have independent filter state. Auto-gain RMS/makeup is not present.
- [ ] `A = 0.1` yields `inGain == 1.0`. `B = 1.0` is a unity pad (no extra scale after DC-B). `A = 0` with a 1 kHz −6 dBFS sine at `B = 1` produces all-zero output. `B = 0` with `A = 0.1` produces all-zero output.
- [ ] `numSamples == 0` returns without writing outputs; a following non-empty block matches an engine that never saw the empty call (state unchanged).
- [ ] An all-zero input block produces an all-zero output block. The engine does not inject TPDF, `frexpf`, or other dither; digital zero stays exact zero.
- [ ] Biquad/`tan` coefficients and DC `iirAmount*` are computed on sample-rate (prepare) change only. The sample loop does not call `tan`, `pow`, or `frexpf`. Processing a non-silent sine at 44.1, 48, 96, and 192 kHz each completes without NaN/Inf in the output.
- [ ] If a sample is NaN or Inf, that sample is replaced with `0` and processing continues (filters are not reset as a whole). The next all-finite block from the same engine contains no NaN/Inf.
- [ ] Character fixtures live under `tests/fixtures/` as 48 kHz stereo WAVs, captured with Mackity `A = 0.1`, `B = 1.0` (no makeup): `tests/fixtures/sine_1khz_m6dbfs_48k.wav` + `tests/fixtures/sine_1khz_m6dbfs_48k_mackity_ref.wav` (1 kHz sine at −6 dBFS), and `tests/fixtures/drum_loop_excerpt_48k.wav` + `tests/fixtures/drum_loop_excerpt_48k_mackity_ref.wav` (short drum-loop excerpt). WAVs are committed, **or** `tests/fixtures/README.md` documents an offline renderer that produces those four files from the Mackity source of truth; the character test always runs and does not skip when files are absent.
- [ ] A unit/integration test runs the engine at 48 kHz with `A = 0.1`, `B = 1.0` on each input fixture and asserts `RMS(engine − ref) / RMS(ref) < 0.15` on both pairs. If any of the four fixture files is missing or unreadable, that test **fails** (fail closed; not skip/pass).
- [ ] `process()` (the per-block audio function) allocates no heap memory. A later auto-gain task can insert dry/wet RMS and makeup **after** DC-B and **before** Out Pad without reordering the stages above; this task does not add those detectors or makeup.

## Out of Scope

- Auto-gain (RMS windows, makeup, hold, on/off). Structure the seam only; do not implement AG.
- Wiring the engine into the JUCE `AudioProcessor` / `processBlock`, APVTS, bypass, meters, editor, or host scan.
- SIMD, oversampling, a second double-precision process path, bit-identical match to Mackity, and CPU benches vs Mackity (`cpu-bench`).
- Linking or shipping Airwindows plugin binaries; MIT NOTICE text (`license-attribution-install`).

## Constraints

- Port topology and numeric constants from `/Users/seto/projects/airwindows/plugins/MacSignedVST/Mackity/source/`; rewrite the hot path (no copy-paste of the `tan`-every-buffer + `pow`/`frexpf` loop).
- Do not add JUCE types to the engine, and do not couple the engine to `PluginProcessor`.
- Do not add SIMD or oversampling in this task, including as a “fix” if the 0.15 RMS bound fails.
- Do not reintroduce dither or denormal noise to chase the character bound; a miss is a fidelity-vs-CPU call for a later decision, not a silent DSP add-on.
- One float realtime path. Out Pad stays last. Product name remains mach1 (not Mackity/Airwindows branding in code that this task adds).

## Implementation Notes

Host-free `mach1::MackityEngine` in `dsp/` (static lib `mach1_dsp`, no JUCE). Topology matches Mackity: DC-A → In Trim `(A*10)²` → LP Q≈0.4317 → clip/`x−x⁵·0.1768` → LP Q≈1.1582 → DC-B → (comment-only auto-gain seam) → Out Pad. Coeffs/`tan` only in `prepare` on sample-rate change; process uses `x*x*x*x*x`, no dither/`frexpf`. `processBlock` is still passthrough.

Fixtures: `tests/generate_mackity_fixtures.cpp` renders 48 kHz stereo IEEE-float WAVs with a double-precision copy of Mackity’s loop at A=0.1, B=1.0, **without** dither or denormal noise. Sine is 1 s 1 kHz −6 dBFS. Drum file is a **synthetic** kick+noise burst (0.75 s), not a licensed loop — see `tests/fixtures/README.md`. Files are committed under `tests/fixtures/`.

Tests: `ctest -R mackity_engine` (or `./build/mach1_engine_test`). Observed `RMS(err)/RMS(ref)`: sine **0.026**, drum **0.059** (both < 0.15; float engine vs double ref).

## Review Log

### 2026-08-28T00:25:39Z

Reviewed the full source tree (no git repo; `commit_sha` = uncommitted). Shared pass across the implemented backlog.

Specialists dispatched: testing, maintainability, performance, security, then red-team (200+ lines + CRITICAL). Skipped: data-migration, api-contract (no schema/HTTP API). Design checklist skipped (no TSX/CSS templates).

AUTO-FIXED:
- [src/PluginProcessor.cpp:153] Heap `monoRight.assign` on the audio thread → grow only in `prepareToPlay`; `processBlock` never resizes.
- [src/PluginProcessor.cpp:146] `getWritePointer(1)` from layout counts → gate on `buffer.getNumChannels()`.
- [src/PluginProcessor.cpp:198] Exactly-8-byte blob always treated as A/B → skip if JUCE XML magic `0x21324356`.
- [tests/host_validation.sh:117] `all` always `exit 0` → hard-fail (1) fails the combined run; unverified (2) does not.
- Bypass meters frozen; magic `8` / `0.5f` / channel `2` → named constants; bypass updates peak atomics.

ASK: none (criticals were mechanical).

Left open as TODOs (`_docs/graphstack/TODOS.md`): extra negative-path tests, DSP fast-path DRY, Logic/Reaper in-app E2E, JUCE license decision. Informational; does not fail Review.

PR Quality Score: 0 (4 critical ×2 + informational cluster; all criticals auto-fixed)

VERDICT: PASS

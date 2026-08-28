---
status: reviewed
---

# Dry-RMS auto-gain in the engine

## Goal

With auto-gain on, driving In Trim no longer requires hunting Out Pad: after settle, engine output loudness stays within ±1.5 dB of dry RMS, with Out Pad as a linear offset on top of makeup. Auto-gain can be turned off, in which case detectors and makeup do not run.

## Acceptance Criteria

- [ ] Auto-gain lives in host-free `mach1::MackityEngine` (`dsp/MackityEngine.h` / `dsp/MackityEngine.cpp`). `process` still takes stereo `float**` in/out, `numSamples`, and In Trim `A` / Out Pad `B` (clamped `0…1`), plus an auto-gain enable flag (bool). No JUCE types, includes, or linkage. Callers (tests) can enable/disable without a plugin host.
- [ ] When auto-gain is on, one audio block applies stages in this order: existing Mackity topology through DC-block IIR B (unchanged), then stereo dry RMS vs wet RMS (wet = post-DC-B, pre-makeup), then shared L/R makeup, then Out Pad `× B`. Makeup is inserted at the existing seam in `MackityEngine::process` (comment: “Auto-gain (dry/wet RMS + makeup) inserts here, before Out Pad”), not after pad and not before DC-B.
- [ ] RMS uses a leaky window of about **80 ms**; makeup is `dry / max(wet, floor)` smoothed with a one-pole of about **300 ms**. One makeup gain is applied to both L and R (not independent per-channel gains).
- [ ] Auto-gain **on**, 48 kHz, stereo 1 kHz sine, Out Pad `B = 1.0`: after In Trim is set from `A = 0.1` to `A = 0.4` and at least **1 s** of audio has been processed, the stereo output RMS is within **±1.5 dB** of the stereo input (dry) RMS, measured on a post-settle window (not including the slew).
- [ ] Same setup with `B = 0.5`: after the same settle, stereo output RMS is within **±1.5 dB** of dry RMS **+ 20·log10(0.5)** (pad is a linear offset after makeup; it is not absorbed into the match).
- [ ] While auto-gain is on, if stereo dry RMS stays below **−80 dBFS**, makeup is **held** (not updated toward `dry/wet`). Feeding a long digital-silence or sub-hold-threshold block does not send makeup to a huge value; a following moderate-level block does not come out near full scale as a run-away spike.
- [ ] Start-from-silence: engine `reset()` (or first `prepare`), then auto-gain on, then a 1 kHz sine at a typical level (e.g. −6 dBFS) with `A = 0.4`, `B = 1`. Peak output in the first 50 ms is not a full-scale blast (first-block peak stays below 0 dBFS / `|x| < 1` for that −6 dBFS input); makeup ramps via the ~300 ms slew rather than jumping to an unbounded initial `dry/wet`.
- [ ] Shared L/R: auto-gain on, hard-panned 1 kHz (signal on L only, R = 0). After settle, the same makeup coefficient is applied to L and R (R remains silence aside from pad/sanitize; L is not compensated with an L-only detector). A dual-mono / per-channel auto-gain implementation fails this check.
- [ ] Auto-gain **off**: RMS detectors and makeup are skipped (no dry/wet RMS updates, no makeup multiply). For the same `A`/`B` and input, output matches the pre-auto-gain topology (DC-B then `× B` only)—same path as `mackity-engine` with auto-gain absent. Pad-only behavior is unchanged.
- [ ] `process()` allocates no heap. `numSamples == 0` / unprepared / null buffers still no-op. `reset()` clears auto-gain detector and makeup state so a following all-zero input block produces all-zero output.

## Out of Scope

- JUCE `AudioProcessor` / APVTS `AutoGain` param, save/load, legacy 2-float chunk → AG off, bypass, buses, meter atomics (`processor-params-state`).
- Editor toggle, knobs, labels, or any UI wiring (`native-editor`).
- Compressor-style dynamics, peak/LUFS matching, per-band loudness, dual-mono or M-S auto-gain.
- Changing Mackity topology, coefficients, character-fixture bounds, SIMD, oversampling, or CPU benches (`cpu-bench`).
- Host listening tests (Reaper/Logic) — engine unit/integration tests only.

## Constraints

- Frozen auto-gain rule in `_docs/graphstack/plan.md`: stereo dry RMS vs wet RMS after topology and **before** makeup/Out Pad; ~80 ms leaky RMS window; makeup `dry/max(wet, floor)` smoothed ~300 ms one-pole; **shared L/R**; **hold** makeup when dry RMS < **−80 dBFS**; **off skips detectors**; Out Pad stays last as a linear offset on top of makeup.
- Do not add JUCE to `dsp/`. Do not reorder stages around the DC-B → [AG] → pad seam established by `mackity-engine`.
- Loudness match ±**1.5 dB** after settle is the bar; auto-gain is not a compressor. Default-on for new plugin instances is a processor concern, not this task.

## Implementation Notes

Auto-gain is in `MackityEngine::process` at the existing DC-B → pad seam. `process(..., A, B, autoGain=false)` so pad-only callers and character fixtures are unchanged. When on: stereo leaky mean-square (~80 ms) of sanitized input vs post-DC-B wet, makeup `dry/max(wet, 1e-12)` one-pole ~300 ms, same gain on L and R, then `× B`. Makeup is held when dry RMS < 1e-4 (−80 dBFS) and reset/prepare initializes makeup to 1.0. Off skips RMS and makeup (detectors frozen).

`mach1_engine_test` after settle (A 0.1→0.4, 48 kHz, 1 kHz −6 dBFS): **B=1.0 dB error = 0.492 dB**; **B=0.5 dB error vs dry+pad = 0.492 dB**. Character RMS ratios unchanged (sine 0.026, drum 0.059). First-50 ms peak 0.955. JUCE processor not wired.

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

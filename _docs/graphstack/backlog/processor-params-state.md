---
status: reviewed
---

# Processor wiring, buses, state, bypass

## Goal

The JUCE processor hosts `MackityEngine` with In Trim, Out Pad, and AutoGain, session-safe state, stereo and mono buses, host bypass as a true copy, and lock-free meter peaks the editor can read later.

## Acceptance Criteria

- [ ] `Mach1AudioProcessor` owns a `juce::AudioProcessorValueTreeState` with exactly these three automatable parameters (plus any host bypass JUCE adds itself): **In Trim** (`AudioParameterFloat`, range `0…1`, default `0.1`), **Out Pad** (`AudioParameterFloat`, range `0…1`, default `1.0`), **AutoGain** (`AudioParameterBool`, default **on** / `true`). A newly constructed processor reports those defaults before any `setStateInformation`.
- [ ] Non-bypassed `processBlock` calls `mach1::MackityEngine::process (in, out, numSamples, A, B, autoGain)` using the current APVTS values (`A` = In Trim, `B` = Out Pad, `autoGain` = AutoGain). It does not reimplement Mackity topology in the processor. `prepareToPlay` calls `MackityEngine::prepare` with the host sample rate.
- [ ] With AutoGain **off**, In Trim `0.1`, Out Pad `1.0`, a prepared stereo processor’s `processBlock` output matches a separately prepared `MackityEngine` run on the same input (`process(..., 0.1f, 1.0f, false)`) within float noise (`max |Δ| ≤ 1e-5` per sample on both channels).
- [ ] `getStateInformation` / `setStateInformation` round-trip all three parameters: after changing In Trim, Out Pad, and AutoGain away from defaults, saving, constructing a new processor, and loading, the three values match the saved ones (floats within `1e-5`, AutoGain same bool).
- [ ] `setStateInformation` given a **2-float** blob (`sizeInBytes == 8`, two native `float`s: A then B) restores In Trim and Out Pad to those values (clamped to `0…1`) and sets AutoGain **off**. A subsequent `getStateInformation` + load on a third instance still has AutoGain off.
- [ ] Host bypass is a true copy: `processBlockBypassed` (and `processBlock` when the processor is bypassed, if that path is used) writes each output sample equal to the corresponding input sample. With In Trim `0` (which would silence the engine) and a non-zero input, bypassed output still equals input.
- [ ] After `prepareToPlay` → at least one non-silent `processBlock` → `reset()` (and/or `releaseResources` then `prepareToPlay` if `reset` is not overridden), an all-zero input block produces an all-zero output block (engine filter/RMS/makeup state cleared; no leftover ring).
- [ ] In Trim and Out Pad values outside `0…1` are clamped to `0…1` in the APVTS range and in the values passed into `MackityEngine::process`. Setting a value `< 0` or `> 1` (or loading a 2-float chunk with out-of-range floats) results in stored/used values of `0` or `1` respectively.
- [ ] Stereo 2-in/2-out remains supported and is the default bus layout. Matching **mono 1-in/1-out** is supported: `isBusesLayoutSupported` is true, `setBusesLayout` succeeds, and `processBlock` on a 1-channel buffer does not crash. The host still sees one input and one output channel (no stereo upmix / no extra output channel). Channel 0 is processed; the processor does not require the host to provide a second channel.
- [ ] Each processed (non-bypassed) block updates lock-free **atomic** input and output peak values (max `|sample|` that block, or equivalent peak atomics the editor can poll). The audio thread does not call the editor, allocate UI objects, or take a UI lock. Peaks are readable from the processor without constructing an editor. After a stereo block whose largest input magnitude is `0.8` and whose output is non-zero, the input-peak atomic(s) reflect at least that `0.8` (within `1e-5`) and the output-peak atomic(s) reflect the processed peak, not an untouched passthrough copy unless the DSP actually produced that copy.

## Out of Scope

- Custom LookAndFeel, knobs, labels, About, meter drawing, or replacing `GenericAudioProcessorEditor` (`native-editor`).
- Changing `MackityEngine` topology, auto-gain math, or its `process` signature (already `process(..., float A, float B, bool autoGain = false)`).
- CPU bench vs Mackity (`cpu-bench`), full `auval` / Logic / Reaper host QA (`host-validation`).
- Surround layouts as a product, dual-mono/M-S, extra parameters, presets, dry/wet.

## Constraints

- Reuse existing `dsp/MackityEngine`; do not add JUCE types to `dsp/`. Engine `process` still requires stereo `float**` (both L and R pointers non-null); for mono 1-in/1-out the processor may use an internal scratch second channel, but must not expose a second host channel.
- Surround: do not advertise or accept 5.1/7.1; if a layout somehow has extra channels, only channels 0–1 are the product path (clear unused outputs). Do not claim a surround product.
- Parameter display names stay **In Trim**, **Out Pad**, **AutoGain** so `native-editor` can bind later; keep APVTS parameter IDs stable (e.g. `inTrim`, `outPad`, `autoGain`).
- Default AutoGain **on** is a processor/APVTS concern; engine default remains `autoGain = false` for host-free callers.
- Existing `mach1_passthrough_test` currently asserts passthrough and rejects mono; update or replace it so processor tests match this task (DSP when not bypassed; mono allowed).
- Do not add a new third-party dependency.

## Implementation Notes

Wired `Mach1AudioProcessor` to `mach1_dsp` (`MackityEngine`) from both the plugin and `mach1_passthrough_test`. APVTS owns In Trim (`inTrim`, 0…1, default 0.1), Out Pad (`outPad`, 0…1, default 1.0), and AutoGain (`autoGain`, default on). `prepareToPlay` / `reset` / `releaseResources` call engine prepare/reset. Non-bypassed `processBlock` reads APVTS, clamps A/B, and calls `engine.process`; stereo is in-place L/R, mono uses a silent scratch R so the host still sees 1-in/1-out. `processBlockBypassed` copies host I/O. State is ValueTree XML; a blob of exactly 8 bytes is treated as legacy A,B with AutoGain forced off (values clamped 0…1). Per-block input/output peaks are `std::atomic<float>` readable without an editor. Processor tests in `tests/passthrough_test.cpp` cover the ACs (passthrough is no longer asserted). Assumed default JUCE bypass (no custom bypass param): hosts/tests call `processBlockBypassed`; `processBlock` also copies if `getBypassParameter()` is non-zero.

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

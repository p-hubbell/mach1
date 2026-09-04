---
status: implemented
---

# Pre-clip Color bias in MackityEngine (0 = v1)

## Goal

`MackityEngine` applies Color as a pre-clip even-harmonic DC bias. Color 0 is bit-identical to today’s Color-unaware engine (including the unity fast path); Color above 0 raises even harmonics at the same Drive/Output without replacing the existing clipper.

## Acceptance Criteria

- [ ] `MackityEngine::process` in `dsp/MackityEngine.h` grows a last parameter `float color = 0` after `bool autoGain = false`. Existing 5- and 6-argument callers still compile and behave as Color 0. Signature remains `noexcept`, host-free (`float**` in/out, no JUCE types).
- [ ] Per-sample order in both the `step` lambda and the unity fast path is unchanged except for Color: DC-A → In Trim (when `inGain != 1`) → LP-A (`lpA_`) → **Color bias** → `saturate` (anonymous `saturate` in `MackityEngine.cpp`: clip `±1` then `x − x⁵·kShape`) → LP-B → DC-B → (AG if on) → pad. Bias is not applied before LP-A or after `saturate`.
- [ ] Named constant `kColorBiasMax` is `0.12f`. Applied bias is `MackityEngine::clamp01(color) * kColorBiasMax` (not `jlimit`). When `clamp01(color) == 0`, the add is skipped (no `x += 0`).
- [ ] Color=0, A=0.1, B=1, AutoGain off, same input: output matches today’s Color-unaware `process` within existing fixture bounds `RMS(err)/RMS(ref) < 0.15` on `tests/fixtures/sine_1khz_m6dbfs_48k.wav` and `tests/fixtures/drum_loop_excerpt_48k.wav` vs their `_mackity_ref.wav` files. Defaulted `color` and explicit `color=0` are equivalent.
- [ ] Unity fast path (today: `!autoGain && !applyIn && !applyOut && blockFinite`, i.e. A=0.1 / B=1 / AG off / all-finite block) still runs when Color==0. When Color>0 that Color-blind fast path is not taken; bias is applied before `saturate`.
- [ ] Matched Drive/Output, AG off, 1 kHz sine driven into clip: `|H2|/|H1|` at Color=1 is strictly greater than at Color=0. `|H3|/|H1|` at Color=1 is at least half of `|H3|/|H1|` at Color=0 (clipper not gutted to chase even).
- [ ] Color NaN, Inf, `<0`, and `>1` are handled only via existing `MackityEngine::clamp01` (NaN/Inf/`<0` → 0 skip-add; `>1` → bias `kColorBiasMax`). Unprepared engine, `in`/`out` null (or null channel pointers), and `numSamples <= 0` remain no-ops (return without writing outputs), independent of Color.
- [ ] Digital-silence input (all-zero block) at Color=0 stays all-zero output. Color>0 on silence is **not** required to stay all-zero (bias into `saturate` may produce DC/offset).
- [ ] `process()` still allocates no heap. `saturate`’s formula and `kShape` are not changed. `reset()` / `prepare` do not grow Color-specific state.

## Out of Scope

- APVTS `color` param, `processBlock` wiring, `replaceState` / v1 XML / 8-byte legacy (`params-names-state`).
- Editor layout, Classic/Even labels, host `getText`, auval (`editor-wireframe`, `host-docs-v11`).
- Mix, Hard, Dark, MoMa Mode, AutoGain retune, oversampling, SIMD.
- Raising or retuning `kColorBiasMax` beyond `0.12f`.
- CPU 2× bar at Color>0 (`cpu-bench`); Color>0 may be slower than the Color=0 fast path.
- Changing fixture WAV files or the `0.15` character bound for Color=0.

## Constraints

- Locked architecture in `_docs/graphstack/plan.md`: Color lives only in `MackityEngine`; after LP-A and before `saturate`; `bias = clamp01(Color) * 0.12f`; Color 0 skips the add so v1 / unity fast path stay eligible; Color>0 must not use a Color-blind fast path.
- Do not gut or replace `saturate` to satisfy even-harmonic ACs. Do not insert Color in the processor in this task.
- Default last argument `0` keeps pad-only and character-fixture callers on v1 without updating every call site.
- `jlimit` is not a NaN gate; Color sanitizing is `clamp01` only.

## Implementation Notes

`MackityEngine::process` now takes `float color = 0` after `autoGain`. `kColorBiasMax` (`0.12f`) lives with the other DSP constants in `MackityEngine.cpp`. After LP-A, if `clamp01(color) != 0`, both channels add `clamp01(color) * kColorBiasMax` before the existing `saturate`; Color 0 skips the add. The unity fast path keeps the same sample order as before and is taken only when Color clamps to 0 (plus the previous AG-off / unity in-trim / unity pad / finite-block conditions). Color>0 uses the general `step` path so bias is applied. `saturate` / `kShape` / `prepare` / `reset` are unchanged; no Color state and no heap in `process`. Processor/APVTS wiring was not added.

Tests in `mackity_engine_test.cpp`: default vs explicit Color=0; Color=1 at A=0.1/B=1/AG off differs from Color=0 (fast path not used); silence Color=0 stays zeros; clipped 1 kHz H2/H1 and H3/H1 ratios; NaN/Inf/`<0`/`>1` via `clamp01`; no-ops at Color=1. Existing character fixtures still run with defaulted Color.


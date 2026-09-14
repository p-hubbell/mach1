---
status: reviewed
---

# Drive/Output/Auto Gain names, Color param, v1 state

## Goal

The processor exposes Drive, Output, Auto Gain, and Color on the existing APVTS tree. v1 sessions restore Drive/Output/Auto Gain as before and missing Color as 0; `processBlock` passes Color into `MackityEngine`.

## Acceptance Criteria

- [ ] APVTS parameter IDs stay `inTrim`, `outPad`, and `autoGain` (existing `Mach1AudioProcessor::inTrimId` / `outPadId` / `autoGainId` still resolve). A fourth parameter exists with ID `color` created as `juce::ParameterID { "color", 1 }` (version 1). A `colorId` constant matching that string is available the same way as the other IDs.
- [ ] Host/display names (`AudioProcessorParameter::name`) are exactly `Drive`, `Output`, `Auto Gain` (space, not `AutoGain`), and `Color`. Fresh-processor defaults are Drive `0.1`, Output `1.0`, Auto Gain `true`, Color `0`.
- [ ] Color is a `0…1` float. `stringFromValue`: `0` → `Classic`; `1` → `Even`; any value in `(0, 1)` → `Blend`. The returned string has no `%` suffix. `valueFromText` accepts `Classic` / `Even` / `Blend` (case as implemented, but those three tokens) and a raw `0…1` float string, mapping to `0` / `1` / a value in `(0, 1)` / the parsed float respectively.
- [ ] `getStateInformation` / `setStateInformation` XML round-trip on a new processor restores all four values (Drive, Output, Auto Gain, Color), not only the original three. ValueTree type remains `"PARAMS"`.
- [ ] XML that is valid `"PARAMS"` but has no `color` property (v1-shaped: `inTrim` / `outPad` / `autoGain` present) restores Drive/Output to the saved values and Auto Gain per the existing XML/bool rules, and Color is `0`. This holds when `setStateInformation` / `replaceState` runs on a **reused** processor whose Color was previously ≠ `0` (no sticky Color).
- [ ] 8-byte little-endian legacy A/B blob (`setStateInformation` size `8`, not JUCE XML magic) still restores Drive/Output from the two floats (clamped as today via `MackityEngine::clamp01`) and Auto Gain **off**, and sets Color to `0`. A subsequent XML save/load of that processor still has Color `0` and Auto Gain off.
- [ ] `processBlock` reads Color from APVTS and passes it into `MackityEngine::process` as the `color` argument (after A, B, autoGain). Color is sanitized with `MackityEngine::clamp01`, not `juce::jlimit`. With Auto Gain off, Drive `0.1`, Output `1.0`, stereo sine as in `tests/passthrough_test.cpp`: max `|Δ|` between `processBlock` output and a matching `MackityEngine::process` call is `≤ 1e-5` at Color `0` and again at Color `0.5`.
- [ ] Tests in `tests/passthrough_test.cpp` that currently assert display names `"In Trim"` / `"Out Pad"` / `"AutoGain"` are **superseded** (updated to Drive / Output / Auto Gain, plus Color coverage above). Those tests are not made to pass by removing DSP, bypassing `processBlock`, or deleting the existing engine-match / state / bypass / peak / mono cases.

## Out of Scope

- Editor layout, component IDs, Classic/Even slider chrome, removing a Color text box, or changing editor label collection in `passthrough_test.cpp` (that is `editor-wireframe`).
- auval, in-app Logic/Reaper/FL, Mix / Hard / Dark / MoMa Mode.
- Changing `MackityEngine` Color DSP, `kColorBiasMax`, or engine-only tests (`color-engine`).
- Renaming Auto Gain’s **ID**, deleting Output, oversampling, Windows/CLAP, presets.

## Constraints

- Keep APVTS ValueTree type `"PARAMS"`.
- Do not change IDs `inTrim` / `outPad` / `autoGain`; only display names and the new `color` param.
- Legacy 8-byte A/B: Auto Gain off and Color `0`.
- Color sanitizing in the processor uses `MackityEngine::clamp01` only (`jlimit` is not a NaN gate).
- Color audio behavior stays in `MackityEngine`; the processor only stores the param and passes `clamp01(Color)` into `process`.

## Implementation Notes

APVTS IDs stay `inTrim` / `outPad` / `autoGain`; added `colorId` `"color"` as `ParameterID { "color", 1 }`. Display names are Drive / Output / Auto Gain / Color; Color defaults to 0 with Classic/Even/Blend `stringFromValue` (no `%`) and matching `valueFromText` plus raw float parse. `processBlock` passes `MackityEngine::clamp01(Color)` into `process` (stereo and mono). After XML `replaceState`, a missing Color PARAM is forced to 0 so reused processors are not sticky; 8-byte legacy also sets Color 0. `tests/passthrough_test.cpp` covers names, text, four-param XML, v1 XML without color, legacy Color 0, and engine match at Color 0 and 0.5. Editor label collection in that file was left unchanged. Assumed Blend `valueFromText` maps to 0.5. Built and ran `mach1_passthrough_test` (Release): `processor tests passed`.

## Review Log

- **2026-09-14** — Same v1.1 branch review as sibling tasks. **AUTO-FIXED:** `jlimit` → `clamp01` on Drive/Output in `processBlock`; Color `valueFromText` trim + `equalsIgnoreCase`. **ASK approved:** case-insensitive Color tokens; mono Color 0.5 processor-vs-engine. PR Quality Score: 9.5. `VERDICT: PASS`

---
status: reviewed
---

# auval/docs/checklist for four named params

## Goal

Host validation, Color host text, and human-facing checklists/README describe the four v1.1 parameters (Drive, Output, Auto Gain, Color) so Test does not still assert the old three-knob In Trim / Out Pad / AutoGain copy.

## Acceptance Criteria

- [ ] Running `tests/host_validation.sh --auval` (rebuild AU then `auval -v aufx Mh01 Stao`) exits 0 **and** the auval parameter listing shows **exactly four** user parameters named `Drive`, `Output`, `Auto Gain` (space), and `Color` — not `In Trim`, `Out Pad`, or `AutoGain`, and not a three-parameter listing. If `auval` cannot be executed in the environment (missing binary, hang, or otherwise cannot capture that listing), the case must print **`FAIL-UNVERIFIED`** (not a skip, not a silent pass as if names were verified). A 3-param In Trim listing is a **fail**, not unverified.
- [ ] Logic in-app insert is still E2E, not implied by auval. If `/Applications/Logic Pro.app` (or `Logic Pro X.app`) is absent or the GUI cannot be driven, `tests/host_validation.sh --logic` must print **`FAIL-UNVERIFIED`** (not skip-pass). That print must not be treated as this AC passing and must not fail or skip-pass the auval four-name gate when auval itself ran.
- [ ] Color `AudioParameterFloat` host text (`getCurrentValueAsText` / `stringFromValue`): value `0` → `Classic`; value `1` → `Even`; any value in `(0, 1)` (e.g. `0.5`) → `Blend`. None of those strings contain `%`. FL Studio’s native hint bar still showing a number or percent is **not** a fail of this AC and must not be treated as one.
- [ ] `tests/MANUAL_CHECKLIST.md` uses **Drive**, **Output**, **Auto Gain**, and **Color** (not In Trim / Out Pad / AutoGain). It states Color `0` is the Mackity / v1 sound, **not** “off.” Stereo steps still describe Drive saturating, Auto Gain on holding level, Auto Gain off + Output changing loudness. Reaper steps still say automating the Drive control (APVTS ID `inTrim`) moves Drive.
- [ ] Root `README.md` documents those four control names (Drive / Output / Auto Gain / Color) and that Color `0` is the Mackity / v1 sound, not “off.” It does not list In Trim / Out Pad / AutoGain as current control names.
- [ ] Reaper VST3 E2E: scan, insert, record/play automation on APVTS ID `inTrim` — the automatable Drive parameter still moves Drive (display name Drive, ID unchanged). Session save/reload restores parameters. **Checkable only when** `/Applications/REAPER.app` exists. If that path is **absent**, `tests/host_validation.sh --reaper` must print **`FAIL-UNVERIFIED`** (not skip, not pass). That print must not fail or skip-pass auval / host-free tests. This AC must not be claimed passed while the last run printed `FAIL-UNVERIFIED` for Reaper.

## Out of Scope

- Installing Logic Pro, Reaper, or FL Studio; adding CI; writing a GUI driver for those hosts.
- Making FL’s native hint bar show Classic / Blend / Even (best-effort only; hint-bar `%` is not an AC fail).
- Changing APVTS IDs (`inTrim` / `outPad` / `autoGain` / `color`), editor wireframe layout, Mix / Hard / Dark / MoMa Mode, deleting Output, oversampling, Windows/CLAP, presets.
- Retuning Color DSP (`kColorBiasMax`, bias placement) or claiming in-app Logic/Reaper behavior from `auval` alone.

## Constraints

- Record Review / Test / evidence on the SHA that will ship (that SHA must be an ancestor of HEAD at ship time).
- Do not gut DSP, bypass `processBlock`, or delete engine-match / state / bypass tests to satisfy leftover “In Trim” docs or ACs.
- Manufacturer / plugin four-chars stay **Stao** / **Mh01** (`auval -v aufx Mh01 Stao`).
- `auval` pass = Logic-loadable + four named params when the listing is captured; in-app Logic/Reaper remain E2E with `FAIL-UNVERIFIED` when those apps are missing.
- Documenting `tests/MANUAL_CHECKLIST.md` is not a pass of Logic in-app ACs.

## Implementation Notes

Docs and host gates now describe the four v1.1 display names. `README.md` and `tests/MANUAL_CHECKLIST.md` use Drive / Output / Auto Gain / Color; Color `0` is the Mackity / v1 sound, not “off.” Stereo steps cover Drive saturating, Auto Gain on holding level, Auto Gain off + Output changing loudness. Reaper steps say automating Drive (APVTS ID `inTrim`) moves Drive. FL hint-bar `%` is called out as not a fail.

`tests/host_validation.sh --auval` rebuilds the AU, runs `auval -v aufx Mh01 Stao` with a timeout, and requires a captured listing of exactly those four names (`tests/check_auval_listing.py`). Missing/hanging auval or an unparseable listing prints `FAIL-UNVERIFIED` (exit 2). A 3-param In Trim / Out Pad / AutoGain listing is a hard fail. `--logic` / `--reaper` still print `FAIL-UNVERIFIED` when those apps are absent/unscriptable and do not skip-pass or fail the auval name gate.

Color host text was already Classic / Blend / Even in `createParameterLayout`. `tests/passthrough_test.cpp` now also checks `getText` (stringFromValue) for 0 / 0.5 / 1 with no `%`. DSP / engine-match tests were not changed.

This environment: `auval` ran exit 0, `AU VALIDATION SUCCEEDED.`, four Global Scope Parameters Color / Drive / Output / Auto Gain. Logic.app and `/Applications/REAPER.app` absent — `--logic` and `--reaper` printed `FAIL-UNVERIFIED` (exit 2); those E2E ACs are not claimed passed. `mach1_passthrough_test` passed. Listing parser self-test covers four-name ok, 3-param In Trim fail, empty listing unverified.

## Review Log

- **2026-09-14** — Same v1.1 branch review. **AUTO-FIXED:** dead combined missing-auval-and-python3 branch in `host_validation.sh`. **ASK approved:** auval self-test for count-without-names and unexpected four-name set. PR Quality Score: 9.5. `VERDICT: PASS`

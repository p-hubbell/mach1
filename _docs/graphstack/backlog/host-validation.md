---
status: reviewed
---

# Logic auval + Reaper VST3 host gates

## Goal

mach1 is Logic-loadable as an AU (`auval` full validation) and remains a working stereo/mono insert in Logic and a VST3 insert in Reaper when those apps are present; missing or unscriptable hosts do not fake a pass.

## Acceptance Criteria

- [ ] Automated command `auval -v aufx Mh01 Stao` **passes** (exit 0). This is the required Logic-AU gate: `auval` is Apple’s AU validator used by Logic. A pass means the component is Logic-loadable (opens, layouts, render tests). This AC is required even when Logic Pro is not installed. It is **not** an in-app stereo/mono insert test.
- [ ] Host-free **mono 1-in/1-out** already covered by the processor test (`mach1_passthrough_test` or equivalent): `isBusesLayoutSupported` / prepare / `processBlock` on a 1-channel buffer does not crash and keeps one in and one out. That test **must still pass**. It is the checkable mono bar without a DAW.
- [ ] Logic **in-app stereo insert** (In Trim audibly drives; AutoGain on holds level; AutoGain off + Out Pad changes loudness) is E2E, not implied by `auval`. **Checkable only when** `/Applications/Logic Pro.app` (or `Logic Pro X.app`) exists **and** the GUI can be driven (script or a human running the checklist). If Logic.app is **absent**, or present but **not scriptable**, this case must print **`FAIL-UNVERIFIED`** (not a skip, not a pass) — same pattern as `cpu-bench` Reaper. That print must not be treated as this AC passing and must not fail or skip-pass the `auval` / host-free ACs. This AC must not be claimed passed while the last run printed `FAIL-UNVERIFIED`. If Logic.app exists and GUI cannot be automated, Implementation Notes must include a **manual checklist** (insert on a stereo track, drive In Trim, AutoGain on/off + pad) for a later human; documenting the checklist is not a pass.
- [ ] Logic **in-app mono track insert** (no crash, produces audio) is the same E2E class: **not** claimed from the host-free mono test. If Logic GUI cannot be driven (or Logic.app is absent), this case prints **`FAIL-UNVERIFIED`** — not skip-pass. Host-free mono remaining green is required independently.
- [ ] Reaper **VST3** E2E (scan, insert, record/play In Trim automation, session save/reload restores params): **checkable only when** `/Applications/REAPER.app` exists. If that path exists, the case must run and meet those checks. If `/Applications/REAPER.app` is **absent**, the Reaper case must print **`FAIL-UNVERIFIED`** (not a skip, not a pass). That print must not fail or skip-pass the `auval` / host-free ACs. This AC must not be claimed passed while the last run printed `FAIL-UNVERIFIED`.

## Out of Scope

- Ableton (or any third DAW) as a blocking gate; optional smoke only if someone already has it.
- Installing Logic Pro or Reaper on this machine so E2E ACs can pass.
- Inventing a headless Logic/Reaper host or UI automation framework beyond a documented checklist / existing `FAIL-UNVERIFIED` printer.
- Windows, AAX, CLAP, native-editor in-host chrome (`native-editor`), CPU vs Mackity (`cpu-bench`).

## Constraints

- Manufacturer / plugin four-chars stay **Stao** / **Mh01** (`auval -v aufx Mh01 Stao`).
- Distinguish gates: `auval` pass = Logic-loadable; in-app stereo/mono insert = E2E; Reaper VST3 = E2E. Do not treat `auval` as proof of in-app insert behavior.
- Reuse the existing host-free mono 1-1 processor test; do not require a DAW for that bar.
- Reaper may be missing (already noted on scaffold); Logic may be missing or unscriptable. Follow the `cpu-bench` Reaper pattern: print `FAIL-UNVERIFIED`, do not skip-pass.
- Do not add a new required host or dependency to “cover” missing DAWs.

## Implementation Notes

`COPY_PLUGIN_AFTER_BUILD` was already `TRUE` on `juce_add_plugin`. Rebuilt `mach1_AU` so `~/Library/Audio/Plug-Ins/Components/mach1.component` is the saturator, then ran `auval -v aufx Mh01 Stao`.

**auval:** exit **0**, `AU VALIDATION SUCCEEDED.` Manufacturer Seto / mach1 0.1.0; 3 params (In Trim, Out Pad, AutoGain); channel caps `[1,1] [2,2]`; render tests (incl. 1-channel) PASS. This is the automated Logic-loadable gate only — not in-app insert.

**Host-free mono:** existing ctest `passthrough` (`mach1_passthrough_test`); not duplicated. Last run: PASS.

**Unverified (not claimed passed):**
- Logic in-app stereo — `FAIL-UNVERIFIED` (Logic.app absent)
- Logic in-app mono — `FAIL-UNVERIFIED` (Logic.app absent)
- Reaper VST3 — `FAIL-UNVERIFIED` (`/Applications/REAPER.app` absent)

Script: `tests/host_validation.sh` (`--auval` / `--logic` / `--reaper`). CMake: `host_auval` (must pass), `host_logic_e2e` and `host_reaper_e2e` (exit 2 when unverified, same pattern as `cpu_bench_reaper`). Human steps: `tests/MANUAL_CHECKLIST.md` (not a pass).

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

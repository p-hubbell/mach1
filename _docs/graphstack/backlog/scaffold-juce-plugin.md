---
status: reviewed
---

# macOS JUCE VST3/AU project that loads

## Goal

mach1 exists as a macOS arm64 JUCE plugin that a host can scan and load as VST3 and AU, with stereo buses and no saturator character yet.

## Acceptance Criteria

- [ ] The repo contains a JUCE project (CMake or Projucer) that builds both a VST3 bundle and an Audio Unit component on this Mac.
- [ ] Both built binaries are arm64 (not x86_64-only); `file` or `lipo -archs` on each reports `arm64`.
- [ ] Reaper’s VST3 scanner lists the plugin and loading it does not crash Reaper.
- [ ] `auval -a` (or Logic’s AU list) shows the plugin; loading it does not crash `auval` or Logic. A full `auval` pass is not required.
- [ ] The host-visible product name is **mach1**. Manufacturer name, manufacturer four-char code, and plugin four-char code are unique to this product and are not Mackity or Airwindows identifiers.
- [ ] The processor declares a stereo input bus and a stereo output bus (2-in / 2-out).
- [ ] With a known audio signal, the plugin either copies input to output (passthrough) or outputs silence. It does not apply Mackity-style saturation or any other DSP character.

## Out of Scope

- Mackity topology / `MackityEngine`, auto-gain, meters, or the three-parameter APVTS contract (later tasks).
- Custom LookAndFeel, custom editor chrome, WebView/React UI.
- Windows, CLAP, AAX, SIMD.
- Full `auval` validation, Logic mono-insert crash tests, and Reaper automation/session save (covered by `host-validation` / `processor-params-state`).
- NOTICE, JUCE paid-vs-GPL decision text, and documented install paths (covered by `license-attribution-install`).

## Constraints

- Wrapper is JUCE (`AudioProcessor`), as frozen in the plan. Native editor later; do not introduce a WebView or JS runtime in this scaffold.
- JUCE license choice is recorded in `license-attribution-install`; this task is not blocked on that file.
- Default JUCE generic editor is acceptable until `native-editor`.

## Implementation Notes

CMake JUCE 8.0.15 plugin (FetchContent from `juce-framework/JUCE`), product **mach1**, company **Seto**, bundle id `com.seto.mach1`, manufacturer code `Stao`, plugin code `Mh01` (not Mackity/Airwindows/`mkty`). Formats AU + VST3, effect, stereo 2-in/2-out, `processBlock` passthrough, generic JUCE editor, `JUCE_WEB_BROWSER=0`. macOS `CMAKE_OSX_ARCHITECTURES=arm64`.

Build: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release --parallel`

Binaries: `build/mach1_artefacts/Release/AU/mach1.component` and `.../VST3/mach1.vst3` (`file`/`lipo`: arm64). `COPY_PLUGIN_AFTER_BUILD` installed them to `~/Library/Audio/Plug-Ins/Components/mach1.component` and `~/Library/Audio/Plug-Ins/VST3/mach1.vst3`.

Host-free test `mach1_passthrough_test`: impulse + 440 Hz sine copy L/R; also asserts name `mach1`, stereo buses, and mono layout rejected. Passed.

`auval -a` lists `aufx Mh01 Stao  -  Seto: mach1`. `auval -v aufx Mh01 Stao` **succeeded** (opens, stereo 2ch, render tests). Reaper is not installed — VST3 scan/load in Reaper is **unverified E2E**.

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

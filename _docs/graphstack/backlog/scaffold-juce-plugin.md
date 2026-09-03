---
status: done
---

# macOS JUCE VST3/AU project that loads

## Goal

mach1 exists as a macOS arm64 JUCE plugin that builds as VST3 and AU, with product identity **mach1** and stereo buses. DSP character is owned by later tasks.

## Acceptance Criteria

- [ ] The repo contains a JUCE project (CMake or Projucer) that builds both a VST3 bundle and an Audio Unit component on this Mac.
- [ ] Both built binaries are arm64 (not x86_64-only); `file` or `lipo -archs` on each reports `arm64`.
- [ ] The host-visible product name is **mach1**. Manufacturer name, manufacturer four-char code, and plugin four-char code are unique to this product and are not Mackity or Airwindows identifiers.
- [ ] The processor declares a stereo input bus and a stereo output bus (2-in / 2-out) as the default layout.

## Out of Scope

- Mackity topology / `MackityEngine` character, auto-gain math, meters, or the three-parameter APVTS contract (later tasks). This task does **not** require passthrough or silence.
- Reaper VST3 scan/load and Logic in-app insert (covered by `host-validation`). Live `auval -v` is that task’s Logic-loadable gate.
- Custom LookAndFeel, custom editor chrome, WebView/React UI (`native-editor`).
- Windows, CLAP, AAX, SIMD.
- NOTICE, JUCE paid-vs-GPL decision text, and documented install paths (`license-attribution-install`).

## Constraints

- Wrapper is JUCE (`AudioProcessor`), as frozen in the plan. Do not introduce a WebView or JS runtime.
- JUCE license choice is recorded in `license-attribution-install`; this task is not blocked on that file.
- Later tasks may add DSP, mono buses, and a custom editor; this task’s ACs must remain true of the default stereo identity and build products.

## Implementation Notes

CMake JUCE 8.0.15 plugin (FetchContent from `juce-framework/JUCE`), product **mach1**, company **Seto**, bundle id `com.seto.mach1`, manufacturer code `Stao`, plugin code `Mh01` (not Mackity/Airwindows/`mkty`). Formats AU + VST3, effect, stereo 2-in/2-out, `processBlock` passthrough, generic JUCE editor, `JUCE_WEB_BROWSER=0`. macOS `CMAKE_OSX_ARCHITECTURES=arm64`.

Build: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release --parallel`

Binaries: `build/mach1_artefacts/Release/AU/mach1.component` and `.../VST3/mach1.vst3` (`file`/`lipo`: arm64). `COPY_PLUGIN_AFTER_BUILD` installed them to `~/Library/Audio/Plug-Ins/Components/mach1.component` and `~/Library/Audio/Plug-Ins/VST3/mach1.vst3`.

Host-free test `mach1_passthrough_test`: impulse + 440 Hz sine copy L/R; also asserts name `mach1`, stereo buses, and mono layout rejected. Passed.

`auval -a` lists `aufx Mh01 Stao  -  Seto: mach1`. `auval -v aufx Mh01 Stao` **succeeded** (opens, stereo 2ch, render tests). Reaper is not installed — VST3 scan/load in Reaper is **unverified E2E**.

### 2026-08-28 rework (QA fail — stopped, ACs unimplementable)

QA iteration 1 failed three ACs. No code change: satisfying the written ACs as-is is contradictory with later backlog work and with hosts that are not on this machine.

1. **Reaper VST3 scan/load.** AC requires Reaper’s scanner to list the plugin and a load that does not crash Reaper. `/Applications/REAPER.app` is not installed. Installing Reaper is not in scope for this engineer pass. Faking a scanner listing or claiming load success without the host would not meet the AC.

2. **`auval -a` / Logic load.** AC requires `auval -a` or Logic’s AU list plus a non-crashing load. Logic is absent. QA’s `auval -a` hung (PACE/iLok Thrift to a missing authorization path). This pass cannot install Logic or unstick system `auval`. Earlier Implementation Notes recorded a successful `auval -v aufx Mh01 Stao`; that does not repair the current unverifiable E2E run.

3. **Passthrough or silence; no Mackity-style saturation.** AC and Goal say the scaffold has “no saturator character yet.” Current `Mach1AudioProcessor::processBlock` calls `MackityEngine::process` (IIR, LP, saturate). `mackity-engine` (and related processor/editor tasks) require that wiring. Stripping the engine to copy/silence would green this task by reverting later work the user already forbade. Leaving DSP in place leaves the AC failed. Those two requirements cannot both be true of the same tree.

Mechanical-only path does not exist: the four passing ACs (CMake VST3+AU, arm64, names/codes, stereo buses) already hold; the three failures are host E2E and DSP character. Status left `qa-failed`. Did not set `implemented`.

### 2026-09-02 AC supersession

User directed Goal/ACs to match the current product: keep build, arm64, **mach1** identity, stereo default buses. Dropped passthrough/silence and Reaper/Logic from this task (hosts remain on `host-validation`). No application code change. Status set `implemented` so Review/Test can re-run against the superseding ACs.

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

### 2026-09-02T21:57:27Z

AC supersession only (Goal/ACs/Out of Scope/Constraints). Diff is `_docs/graphstack` markdown, not application code.

Specialists dispatched: none. Skipped: testing, maintainability, performance, security, data-migration, api-contract, red-team (docs-only; below application-code specialist threshold). Design checklist skipped.

Checklist: no SQL, races, LLM, shell, or enum issues in the spec rewrite.

AUTO-FIXED: none. ASK: none.

PR Quality Score: 10

VERDICT: PASS

## QA Log

### 2026-08-28T01:03:01Z (iteration 1)

Current product vs this task’s written ACs (later work wired `MackityEngine` into `processBlock`; judged as-is).

Commands run: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release --parallel` (succeeded with host FS access; sandbox copy-to-`~/Library` failed then recovered); `file` / `lipo -archs` on AU+VST3 Mach-O; `mach1_passthrough_test` + `mach1_engine_test` (both exit 0); host-free `MackityEngine` sine probe (`max_abs_delta=0.0613682`, 511/512 samples changed); `ls /Applications` for Reaper/Logic; `auval -a` (two processes sat at ~0% CPU for >3 min; stderr PACE/iLok Thrift to missing `/var/tmp/com.paceap.eden.licensed/authorization`); AU/VST3 plists + `moduleinfo.json` for names/codes.

| AC | Result | Notes |
| --- | --- | --- |
| JUCE project builds VST3 + AU | **PASS** | CMake JUCE 8.0.15 plugin; artefacts `build/mach1_artefacts/Release/AU/mach1.component` and `.../VST3/mach1.vst3`; `COPY_PLUGIN_AFTER_BUILD` installed both. |
| Both binaries arm64 | **PASS** | `file`: Mach-O 64-bit bundle arm64; `lipo -archs`: `arm64` (not x86_64-only). |
| Reaper VST3 scan/load, no crash | **FAIL** | **high / UX (unverifiable E2E)**: `/Applications/REAPER.app` absent; no Reaper binary. Cannot confirm scanner list or load. |
| `auval -a` or Logic lists plugin; load does not crash auval/Logic | **FAIL** | **high / Errors/Logs (unverifiable)**: Logic absent. `auval -a` did not complete (hung after PACE Thrift errors; no listing line captured this run). Could not run `auval -v aufx Mh01 Stao`. Installed AU plist still encodes `Seto: mach1` / `Stao` / `Mh01` / `aufx`, but that is not a live auval/Logic load. |
| Host-visible name mach1; unique mfr/plugin codes (not Mackity/Airwindows) | **PASS** | `PRODUCT_NAME`/`getName()`/`CFBundleName`/`moduleinfo.json` Name = `mach1`; company `Seto`; codes `Stao` / `Mh01`; bundle `com.seto.mach1`. Not Mackity / Airwindows / `mkty`. |
| Stereo 2-in / 2-out buses | **PASS** | Constructor `BusesProperties` stereo in/out; `mach1_passthrough_test` asserts default buses stereo and prepared layout 2-in/2-out. (Mono is also accepted now; does not remove the declared stereo pair.) |
| Known signal is passthrough or silence; no Mackity-style saturation / DSP character | **FAIL** | **high / Functional**: `Mach1AudioProcessor::processBlock` calls `engine.process` (`MackityEngine`: IIR, LP, `saturate`). `mach1_passthrough_test` requires processor output to match `MackityEngine` (AutoGain off), not input copy. Engine probe on 440 Hz sine (A=0.1, B=1.0, AG on): `max_abs_delta=0.0613682`, 511 samples differ, `out_peak=0.724` (not silence). Bypass path still copies; default processing is saturator character. |

Overall: **FAIL** (3 of 7 ACs failed). Status frontmatter left `reviewed`.

### 2026-08-28T02:07:57Z (iteration 2)

Delayed `auval` pipeline finished (~76 min; PACE/iLok scan delay). New evidence only; earlier DSP/Reaper fails unchanged.

| AC | Result | Notes |
| --- | --- | --- |
| `auval -a` or Logic lists plugin; load does not crash auval/Logic | **PASS** | `auval -a` listed `aufx Mh01 Stao  -  Seto: mach1` (Airwindows Mackity also listed as `aufx mkty Dthr`, distinct codes). `auval -v aufx Mh01 Stao` opened mach1 (Seto, v0.1.0), stereo 2ch default I/O, no crash through open/init/properties/UI (`head -80`; full validation not required). Logic still absent. |

Overall: **FAIL** (2 of 7 ACs still fail: Reaper E2E unverified; processBlock is Mackity saturation, not passthrough/silence). Status frontmatter left `reviewed`.

### 2026-09-02T21:59:04Z (iteration 3)

Judged only the four ACs after the 2026-09-02 supersession. Did not treat MackityEngine saturation, Reaper, Logic, or passthrough as this task’s criteria (Out of Scope). Frontmatter status left `reviewed`.

Commands run: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release --parallel` (exit 0; built `mach1_VST3` and `mach1_AU`); `file` + `lipo -archs` on AU and VST3 Mach-O; `plutil -p` on AU/VST3 Info.plist + `moduleinfo.json`; `./build/mach1_passthrough_test_artefacts/Release/mach1_passthrough_test` (exit 0, “processor tests passed”).

| AC | Result | Notes |
| --- | --- | --- |
| JUCE project builds VST3 + AU | **PASS** | CMake JUCE 8.0.15 (`juce_add_plugin` FORMATS AU VST3). Artefacts `build/mach1_artefacts/Release/AU/mach1.component` and `.../VST3/mach1.vst3`. |
| Both binaries arm64 | **PASS** | `CMAKE_OSX_ARCHITECTURES=arm64`. `file`: Mach-O 64-bit bundle arm64 on both `Contents/MacOS/mach1`. `lipo -archs`: `arm64` (not x86_64-only). |
| Host-visible name mach1; unique mfr/plugin codes (not Mackity/Airwindows) | **PASS** | Product `mach1` (`PRODUCT_NAME`, `getName()`, AU `name` `Seto: mach1`, `CFBundleName`/`CFBundleDisplayName`, VST3 `moduleinfo.json` Name). Manufacturer `Seto`; codes `Stao` / `Mh01` (`PLUGIN_MANUFACTURER_CODE`/`PLUGIN_CODE`; AU manufacturer/subtype; VST3 CID suffix `5374616F4D683031`). Bundle `com.seto.mach1`. Not Mackity / Airwindows / `mkty` / `Dthr`. DSP class `MackityEngine` is later-task character, not host identity. |
| Stereo 2-in / 2-out as default layout | **PASS** | Constructor `BusesProperties` stereo in + stereo out. `mach1_passthrough_test` asserts default bus layouts stereo and prepared 2-in/2-out. `isBusesLayoutSupported` also allows matching mono (allowed; does not replace stereo default). |

Overall: **PASS** (4 of 4 current ACs).

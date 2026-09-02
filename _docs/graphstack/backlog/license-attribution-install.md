---
status: tested
---

# Attribution, JUCE license note, local install

## Goal

A developer can build mach1 and find, in-repo, MIT attribution for Airwindows Mackity, an honest JUCE license note (GPL unless a commercial license is obtained; none recorded here yet), and the macOS user plugin folders that `COPY_PLUGIN_AFTER_BUILD` already copies into.

## Acceptance Criteria

- [ ] A committed `NOTICE` file exists at the repository root (a sibling `LICENSE` may exist in addition; it does not replace `NOTICE`). `NOTICE` attributes **Airwindows Mackity** as the saturator algorithm/source under the **MIT** license. It does not present Airwindows or Mackity as this product’s name or brand.
- [ ] The same `NOTICE` (and/or a sibling root `LICENSE`) contains a JUCE note that states all of the following, as facts, not as a mach1 product-license decision: (1) JUCE is used via the existing FetchContent pin; (2) JUCE is **GPL unless a commercial/paid license is obtained**; (3) **no paid JUCE license is recorded in this repository yet**; (4) until such a license is recorded, **JUCE’s own license applies**. The note must **not** claim that the user or this repo “chose GPL for mach1,” must **not** claim a JUCE commercial license exists, and must **not** invent a paid-license exception.
- [ ] Editor About copy remains consistent with `NOTICE`: it still credits **Airwindows Mackity** as **MIT**; it still does not brand the product as Airwindows or Mackity; host-visible product name remains **mach1**. About does not need new JUCE legal copy unless that would contradict `NOTICE` (keep them aligned if JUCE is mentioned in the UI).
- [ ] A committed repository-root `README.md` documents local install for **arm64** macOS: after a normal CMake plugin build, JUCE `COPY_PLUGIN_AFTER_BUILD` (already `TRUE` on `juce_add_plugin`) copies into the **user** folders `~/Library/Audio/Plug-Ins/VST3/mach1.vst3` and `~/Library/Audio/Plug-Ins/Components/mach1.component`. README states those paths; it does not invent a second installer or require copying by hand when that flag is on. README does not list Windows plugin paths.

## Out of Scope

- App Store, iLok, installer branding, pkg/dmg, codesign/notarization beyond what already exists.
- Windows (or other OS) plugin paths, CLAP, AAX.
- Purchasing, generating, or checking in a JUCE commercial license; relicensing mach1 itself (MIT vs GPL vs other) as a product decision.
- Changing FetchContent JUCE pin, `COPY_PLUGIN_AFTER_BUILD`, formats, product name, or DSP.
- Redesigning the editor; About already credits Mackity — only keep it consistent with `NOTICE`.

## Constraints

- JUCE 8 is already FetchContent’d (`GIT_TAG` 8.0.15 in root `CMakeLists.txt`). Reuse that; do not add a second JUCE fetch or a Projucer-only license workflow.
- Record the **factual gap**: no paid JUCE license in-repo. Do not invent “user chose GPL for mach1.”
- Product identity stays **mach1**; Mackity/Airwindows appear only as MIT attribution.
- Reuse existing `COPY_PLUGIN_AFTER_BUILD TRUE`; document the user VST3/Component paths above rather than adding a new copy/install target.
- Do not add a new third-party dependency for this task.

## Implementation Notes

Root `NOTICE` attributes Airwindows Mackity as MIT algorithm/source and states the product is **mach1** (not Airwindows/Mackity). The JUCE paragraph records FetchContent 8.0.15, GPL unless a commercial/paid license is obtained, **no paid JUCE license in this repo yet**, and that JUCE’s own license applies until then — facts only, not a mach1 GPL product decision. Root `README.md` documents the arm64 CMake build, `COPY_PLUGIN_AFTER_BUILD` user paths (`~/Library/Audio/Plug-Ins/VST3/mach1.vst3`, `~/Library/Audio/Plug-Ins/Components/mach1.component`), identity Stao/Mh01, and `auval -v aufx Mh01 Stao`. No second installer. About in `src/PluginEditor.cpp` was already consistent (Mackity MIT under mach1); left unchanged. `ctest -R license_attribution` (`tests/license_attribution_test.sh`) checks NOTICE/README/About strings.

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

## QA Log

### 2026-09-02T22:10:00Z (qa-engineer)

Status left `reviewed`. No jsonl writes. No code changes.

**What ran:** `bash tests/license_attribution_test.sh` → `PASS license-attribution-install NOTICE/README/About checks` (exit 0). `ctest -R license_attribution --output-on-failure` from `build/` → `license_attribution` Passed 0.01s (1/1). Inspected `git ls-files NOTICE README.md`, root `NOTICE`, `README.md`, `src/PluginEditor.cpp` About copy, `CMakeLists.txt` `COPY_PLUGIN_AFTER_BUILD TRUE` / `PRODUCT_NAME "mach1"` / FetchContent `GIT_TAG 8.0.15`. No root `LICENSE` (allowed; NOTICE is present).

- AC NOTICE Mackity MIT + product identity: **PASS**. Committed root `NOTICE` (`git ls-files NOTICE`). Attributes Airwindows Mackity as saturator algorithm/source under MIT; states product is mach1 and is not Airwindows / not titled Mackity.
- AC NOTICE JUCE facts (not a mach1 GPL product decision): **PASS**. NOTICE records FetchContent pin `GIT_TAG 8.0.15`, “GPL unless a commercial/paid license is obtained”, “No paid JUCE license is recorded in this repository yet”, “JUCE’s own license applies”. No “chose GPL for mach1”, no invented paid-license exception.
- AC Editor About aligned with NOTICE: **PASS**. About: `mach1. DSP algorithm from Airwindows Mackity (MIT license). This product is not Airwindows and is not titled Mackity.` Editor `setName("mach1")`; CMake `PRODUCT_NAME "mach1"`. About does not mention JUCE; no contradiction with NOTICE.
- AC README arm64 local install / user plugin folders: **PASS**. README documents arm64, `COPY_PLUGIN_AFTER_BUILD`, user paths `~/Library/Audio/Plug-Ins/VST3/mach1.vst3` and `~/Library/Audio/Plug-Ins/Components/mach1.component`; no second installer / hand-copy when the flag is on; no Windows plugin paths.

Findings: none.

VERDICT: PASS

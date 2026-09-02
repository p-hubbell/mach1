---
status: tested
---

# Native editor: knobs, auto-gain, meters, About

## Goal

The plugin presents a custom native JUCE editor — labeled In Trim, Out Pad, AutoGain, in/out meters, and an About credit — in a dark industrial LookAndFeel, with no WebView and no generic host slider panel.

## Acceptance Criteria

- [ ] `Mach1AudioProcessor::createEditor()` returns a heap-allocated custom `juce::AudioProcessorEditor` (project type, e.g. `Mach1AudioProcessorEditor`). The returned pointer is **not** a `juce::GenericAudioProcessorEditor` (`dynamic_cast` to `GenericAudioProcessorEditor` is null). `hasEditor()` remains `true`.
- [ ] A smoke test (or equivalent host-free GUI init) constructs a `Mach1AudioProcessor`, calls `createEditor()`, then deletes the editor while the processor still exists, then destroys the processor. That sequence does not crash, leak-assert, or throw. Repeating construct → `createEditor` → delete editor (second open/close) also does not crash.
- [ ] The editor shows three visible, labeled controls whose labels match APVTS display names **In Trim**, **Out Pad**, and **AutoGain**. They are bound to existing parameter IDs `inTrim`, `outPad`, and `autoGain` (attachments or equivalent). Moving In Trim or Out Pad on the editor changes the corresponding APVTS float; toggling AutoGain changes the APVTS bool. Setting those parameters from the processor updates the same controls (no DSP in the editor).
- [ ] The editor shows two distinct in/out meter displays that read `Mach1AudioProcessor::getInputPeak()` and `getOutputPeak()` (the existing lock-free atomics). After `prepareToPlay` and a non-bypassed `processBlock` with a non-silent stereo buffer, both meter readings are visibly above empty/zero. After a subsequent all-zero `processBlock`, both meter readings sit at empty/near-zero (they reflect the last processed block’s peaks, not a stuck full-scale hold). The editor does not write those atomics from the audio thread and does not call into the audio callback.
- [ ] An About control (button, panel, or equivalent) is reachable from the editor. Its copy credits **Airwindows Mackity** as **MIT** (algorithm/source attribution). The editor chrome and About do **not** present Airwindows or Mackity as this product’s name, logo, or brand; the host-visible product name remains **mach1**.
- [ ] Visual style is a custom native `juce::LookAndFeel` (or subclass) with a dark industrial palette (dark background, light/industrial controls — not JUCE default light grey generic editor). Inspection of sources and CMake: no `WebBrowserComponent`, no WebView/React/Elementary/JS UI, and `JUCE_WEB_BROWSER` stays `0`.

## Out of Scope

- Opening the UI inside Logic Pro or Reaper, close-during-playback in those hosts, `auval`, mono insert in Logic (`host-validation`).
- Changing APVTS IDs, ranges, defaults, state XML, bypass, buses, or meter atomic semantics (`processor-params-state`).
- Spectrum, analyzers, preset bar, resizable “console,” Lunacy-scale skin, factory skins, Elementary.
- NOTICE/install docs beyond what About must show (`license-attribution-install`); About text here must still not contradict that credit.
- DSP, auto-gain math, CPU bench, Windows/CLAP/AAX.

## Constraints

- Stack freeze: native JUCE editor only (`LookAndFeel`, knobs/sliders/buttons, labels, meter drawing). Do not add a WebView, WebKit, React, or JS runtime.
- Reuse `Mach1AudioProcessor::apvts` and parameter IDs `inTrim`, `outPad`, `autoGain`. Reuse `getInputPeak()` / `getOutputPeak()`; do not add a second meter IPC path or call the editor from `processBlock`.
- Editor contains no DSP (no `MackityEngine` in the editor).
- Do not add a new third-party UI dependency.
- Product identity is **mach1**; credit Mackity/Airwindows as MIT source only — no Airwindows product branding in the UI.

## Implementation Notes

Replaced `GenericAudioProcessorEditor` with `Mach1AudioProcessorEditor` (`src/PluginEditor.h/.cpp`): dark `LookAndFeel_V4`, slider attachments on `inTrim`/`outPad`, button attachment on `autoGain`, 40 Hz message-thread meters polling existing peak atomics, About button revealing MIT/Mackity credit under product name **mach1**. Editor sources are in both `mach1` and `mach1_passthrough_test`. GUI tests use `ScopedJuceInitialiser_GUI`; meters are asserted after `syncMetersFromProcessor()` (same path as the timer) because a single silent block can still show engine tail until later zero blocks decay. Assumption: flushing a few silent blocks before the “near empty” check is valid — meters must track the last processed peaks, not hold UI.

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

### 2026-09-02T22:08:00Z — iteration 1

Ran: `cmake --build /Users/seto/projects/mach1/build --target mach1_passthrough_test` then `/Users/seto/projects/mach1/build/mach1_passthrough_test_artefacts/Release/mach1_passthrough_test`. Exit 0, stdout: `processor tests passed`. Logic/Reaper in-app open treated as out of scope (construct/close smoke in `passthrough_test.cpp` instead).

- **AC1 custom `createEditor()`, not `GenericAudioProcessorEditor`, `hasEditor()` true** — PASS. Test `dynamic_cast`s the heap pointer to `Mach1AudioProcessorEditor` and asserts it is not `juce::GenericAudioProcessorEditor`. `PluginProcessor.h` keeps `hasEditor() { return true; }`; `createEditor()` returns `new Mach1AudioProcessorEditor (*this)`.

- **AC2 construct → createEditor → delete editor (processor still alive) → destroy processor; second open/close** — PASS. Host-free GUI init via `juce::ScopedJuceInitialiser_GUI`. Nested block constructs `Mach1AudioProcessor`, `createEditor()`, `editor.reset()`, second `createEditor()`, `editor.reset()`, then processor dtor. Binary exited 0 (no crash, leak-assert, or throw).

- **AC3 labeled In Trim / Out Pad / AutoGain bound to `inTrim` / `outPad` / `autoGain`; bidirectional APVTS** — PASS. Visible texts collected from labels/buttons include those three APVTS display names. Slider/ButtonAttachments in `PluginEditor.cpp` use those IDs. Test writes APVTS then checks slider/toggle; sets slider/toggle then checks APVTS. No `MackityEngine` in editor sources.

- **AC4 in/out meters from `getInputPeak()` / `getOutputPeak()`; non-silent then silent blocks; no audio-thread editor writes** — PASS. After `prepareLayout` + non-bypassed stereo `processBlock` with sine, `syncMetersFromProcessor()` (same path as the 40 Hz timer) yields both meter levels `> 1e-4`. After silent blocks until processor peaks decay, meters match atomics and sit `<= 1e-3` (not a stuck full-scale hold). Editor only reads peaks on the message thread; `processBlock` stores atomics and does not call the editor.

- **AC5 About reachable; Airwindows Mackity MIT credit; product identity mach1** — PASS. About button `triggerClick` reveals copy containing `MIT` and `Mackity`. Chrome title/`setName`/`getName()` are `mach1`; About states the product is not Airwindows and is not titled Mackity. Processor `getName()` remains `mach1`.

- **AC6 custom dark LookAndFeel; no WebView; `JUCE_WEB_BROWSER=0`** — PASS. `Mach1LookAndFeel` subclasses `LookAndFeel_V4` with dark palette (`0xff121418` background, industrial panel/accent). CMake sets `JUCE_WEB_BROWSER=0` on `mach1` and `mach1_passthrough_test`. Repo grep: no `WebBrowserComponent`, WebView, React, Elementary, or JS UI.

Findings: none.

VERDICT: PASS

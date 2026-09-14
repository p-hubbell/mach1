---
status: tested
---

# Wireframe editor: Color, hero Drive, Auto Gain under Output

## Goal

The custom editor matches the v1.1 wireframe so a first-time user can see Drive vs Output vs Color vs Auto Gain without Mackity jargon (In Trim / Out Pad / AutoGain).

## Acceptance Criteria

- [ ] After `createEditor()`, walking every `juce::Label::getText()` and `juce::Button::getButtonText()` (same pattern as `collectTexts` in `tests/passthrough_test.cpp`) includes the strings `Drive`, `Output`, `Auto Gain` (space), `Classic`, and `Even`. That walk does **not** contain `In Trim`, `Out Pad`, or `AutoGain` (no space). A visible `Color` heading next to the Color control is allowed; a `%` suffix on Color-related editor chrome is not.
- [ ] Color, Drive, and Output are rotary sliders (`getSliderStyle()` is a `juce::Slider::Rotary*` style). With the editor laid out (`resized()` applied), left-to-right by `getX()` (or left edge of `getBounds()` in editor space) the order is: input meter (`inMeter`) → Color slider (`color`) → Drive slider (`inTrim`) → Output slider (`outPad`) → output meter (`outMeter`). Input and output meters are not stacked in one right-hand column the way they are today.
- [ ] Drive is the hero control: its bounds width **and** height are each strictly greater than Color’s and Output’s corresponding dimensions (not equal).
- [ ] Auto Gain (`autoGain`) sits under the Output column: its `getY()` is greater than the Output slider’s `getY()`, and the absolute difference between Auto Gain’s horizontal center (`getBounds().getCentreX()`) and Output’s center is **strictly less** than the same difference versus Drive’s center.
- [ ] The Color slider uses `juce::Slider::NoTextBox` (no numeric text box). It is attached to APVTS id `color`. Classic is left of the Color slider (`Classic` label `getRight() <=` Color slider `getX()`, or Classic’s center-x is left of Color’s left edge); Even is right of the Color slider (Even label `getX() >=` Color slider `getRight()`, or Even’s center-x is right of Color’s right edge). Writing APVTS `color` updates the Color slider; dragging/setting the Color slider updates APVTS `color` (same bidirectional pattern as Drive/Output today). Drive, Output, and Auto Gain attachments still round-trip with APVTS as in the existing editor block of `tests/passthrough_test.cpp`.
- [ ] `findChildWithID` still resolves: Drive slider `inTrim`, Output slider `outPad`, Auto Gain `autoGain`, meters `inMeter` / `outMeter`, About button `aboutButton`, About body `about`. Color slider id is `color`. APVTS parameter IDs remain `inTrim` / `outPad` / `autoGain` / `color`.
- [ ] Existing meter and About checks in `tests/passthrough_test.cpp` still pass: About click reveals text containing MIT and Mackity (case-insensitive); editor `getName()` is `mach1` and does not contain Airwindows; after non-silent `processBlock` plus `syncMetersFromProcessor()`, both meters’ `getLevel()` are `> 1e-4`; after silence they track processor peak atomics within `1e-4` and return near empty (`<= 1e-3`). `createEditor` still returns `Mach1AudioProcessorEditor`, not `GenericAudioProcessorEditor`.
- [ ] Editor assertions in `tests/passthrough_test.cpp` that currently require visible `In Trim` / `Out Pad` / `AutoGain` are **superseded** to the labels and layout above (including Color id, no Color text box, Classic/Even, L-to-R, Drive size, AG under Output). Those tests are not made to pass by deleting the meter/About/attachment cases.

## Out of Scope

- Pixel-perfect match to a mock or FL screenshot; specific hex colors, fonts, or editor `setSize` pixels.
- Mix knob, Hard / Dark / four-seat Mode, renaming Auto Gain’s APVTS **id**, deleting Output.
- Host `stringFromValue` / auval / docs (`params-names-state`, `host-docs-v11`); Color DSP (`color-engine`).
- Changing meter component types or About copy beyond keeping the existing MIT/Mackity ACs.

## Constraints

- Keep component IDs `inTrim`, `outPad`, `autoGain`, `inMeter`, `outMeter`, `aboutButton`, `about`; Color slider id must be `color`.
- Do not change APVTS parameter IDs.
- Layout is geometric (bounds / centers), not a screenshot diff.
- No new third-party UI dependency.

## Implementation Notes

Laid out `Mach1AudioProcessorEditor` left-to-right as inMeter → Color (Classic | rotary | Even) → hero Drive (`inTrim`) → Output (`outPad`) → outMeter, with Auto Gain under Output and closer to Output’s center than Drive’s. Visible strings are Drive / Output / Auto Gain / Classic / Even (plus an allowed Color heading); In Trim / Out Pad / AutoGain are gone. Color is `RotaryHorizontalVerticalDrag` + `NoTextBox` with a `SliderAttachment` on APVTS `color`; Drive/Output stay on existing IDs. `tests/passthrough_test.cpp` now asserts those labels, rotary styles, L-to-R geometry, Drive size, AG placement, Color id/NoTextBox/Classic-Even, and Color bidirectional attach while keeping About/meter/createEditor cases. Built and ran `mach1_passthrough_test` Release (`processor tests passed`). Assumption: Drive/Output text boxes below the rotaries are allowed because only Color is required to be `NoTextBox`.

## Review Log

- **2026-09-14** — Same v1.1 branch review. No editor-specific CRITICAL findings. Shared display-string coupling noted as P3 TODO. PR Quality Score: 9.5. `VERDICT: PASS`

## QA Log

### 2026-09-14T16:46:38Z (iteration 1)

`commit_sha`: `e4e2151d6a0369dfa1e69023f2edbb3f45210389`

Status frontmatter left `reviewed`. No jsonl writes. No application or test code changes.

**What ran:** `cmake --build build --config Release --target mach1_passthrough_test --parallel && ./build/mach1_passthrough_test_artefacts/Release/mach1_passthrough_test` → `[100%] Built target mach1_passthrough_test`, `processor tests passed`, exit 0. Cross-checked `src/PluginEditor.cpp` / `src/PluginEditor.h` / `src/PluginProcessor.h` / editor block of `tests/passthrough_test.cpp`.

| AC | Result | Evidence |
|---|---|---|
| Visible strings: Drive / Output / Auto Gain / Classic / Even; no In Trim / Out Pad / AutoGain; Color heading allowed; no `%` on Color chrome | **PASS** | Same `collectTexts` walk in `passthrough_test.cpp` requires those five strings and fails if `In Trim` / `Out Pad` / `AutoGain` appear. Run did not take those fail paths. Editor labels are Drive / Output / Auto Gain / Classic / Even plus allowed `Color`; PluginEditor has no `%` on Color chrome; Color slider is `NoTextBox`. |
| Color / Drive / Output rotary; L-to-R `inMeter` → `color` → `inTrim` → `outPad` → `outMeter`; meters not stacked on the right | **PASS** | Test asserts `Rotary*` styles and `getX()` order after `resized()`. Layout takes input meter from the left, output meter from the right, then Color / Drive / Output columns in remaining space. Run did not fail `editor L-to-R order`. |
| Drive hero: width and height strictly greater than Color and Output | **PASS** | Test requires Drive `getWidth()`/`getHeight()` strictly `>` Color and Output. `resized()` sizes Drive 150 vs Color/Output 90. Run did not fail `Drive is not strictly larger`. |
| Auto Gain under Output and closer to Output’s center-x than Drive’s | **PASS** | Test: `autoGain.getY() > outPad.getY()` and `abs(agCx - padCx) < abs(agCx - driveCx)`. Button is laid out in the Output column below the Output rotary. Run did not fail those paths. |
| Color `NoTextBox`, APVTS id `color`, Classic left / Even right, bidirectional Color + Drive/Output/Auto Gain round-trip | **PASS** | Color is `RotaryHorizontalVerticalDrag` + `NoTextBox` with `SliderAttachment` on `colorId` (`"color"`). Test finds Classic/Even flanking Color, writes APVTS then checks sliders/button, then `setValue`/`setToggleState` back into APVTS including Color 0.25. Run did not fail attach or flanking asserts. |
| `findChildWithID` for `inTrim` / `outPad` / `autoGain` / `inMeter` / `outMeter` / `aboutButton` / `about` / `color`; APVTS ids unchanged | **PASS** | Test resolves all eight component IDs (sliders/button/meters/About + Color). Processor still publishes `inTrim` / `outPad` / `autoGain` / `color`. Run did not fail `editor controls not found` or `meters or About control not found`. |
| About MIT+Mackity; editor name `mach1` without Airwindows; meters after process/silence; `createEditor` is `Mach1AudioProcessorEditor` | **PASS** | Same editor block: not Generic; type is `Mach1AudioProcessorEditor`; About click copy contains MIT and Mackity; `getName() == "mach1"`; after sine `processBlock` + `syncMetersFromProcessor()` both `getLevel() > 1e-4`; after silence they track peak atomics within `1e-4` and are `<= 1e-3`. Run printed `processor tests passed`. |
| Old In Trim / Out Pad / AutoGain editor asserts superseded; meter/About/attachment cases kept | **PASS** | Editor GUI asserts now use Drive/Output/Auto Gain/Classic/Even, Color id/NoTextBox, L-to-R, Drive size, AG under Output. Meter, About, createEditor, and bidirectional attachment cases remain in the same `passthrough_test.cpp` block (not deleted to make the suite pass). Binary exit 0. |

Findings: none.

VERDICT: PASS

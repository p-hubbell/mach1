# TODOS

## Review

### Extra processor/engine negative-path tests

**What:** Cover prepare(0)/NaN, mismatched stereo/mono buses, 0-sample processBlock, empty/wrong-tag state blobs, and isolate passthrough_test cases onto fresh processors.

**Why:** Those branches exist and are only exercised indirectly.

**Context:** Testing specialist findings on `MackityEngine::prepare`, `setStateInformation`, and `tests/passthrough_test.cpp`. Not required to land Review.

**Effort:** M
**Priority:** P2
**Depends on:** None

### Deduplicate Mackity fast-path vs `step()`

**What:** One DSP kernel for the unity/AG-off fast path and the general loop.

**Why:** Two copies of the IIR/LP/saturate chain can drift; the fast path exists to hit the CPU bar.

**Context:** `dsp/MackityEngine.cpp` ~173 vs `step()`. Unifying without losing ≥2× vs Mackity needs a bench re-run.

**Effort:** M
**Priority:** P2
**Depends on:** cpu-bench still green

### Logic and Reaper in-app E2E

**What:** Manual or automated insert tests in Logic (stereo + mono) and Reaper (VST3 scan/insert/automation/save).

**Why:** Host ACs were `FAIL-UNVERIFIED` in this environment.

**Context:** `tests/MANUAL_CHECKLIST.md`. Do not treat checklist presence as a pass.

**Effort:** M
**Priority:** P1
**Depends on:** Logic and/or Reaper installed

### Record a JUCE license decision

**What:** Paid JUCE license, or an explicit product license that matches JUCE GPL, written into `NOTICE`.

**Why:** No paid JUCE license is recorded; JUCE’s own terms still apply.

**Context:** `license-attribution-install` already documents the gap; it is not a DSP defect.

**Effort:** S
**Priority:** P1
**Depends on:** None

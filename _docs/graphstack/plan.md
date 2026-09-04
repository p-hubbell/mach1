# Plan: mach1 v1.1 control language

## Scope

**Mode: Hold Scope.** Readable panel + one Color axis. Mix / Hard / Dark / MoMa Mode stay out. Cutting Color would leave a rename-only loop that does not prove “park it on many tracks this week.”

**In**
- Display names: In Trim → **Drive**, Out Pad → **Output**, Auto Gain → **Auto Gain** (space). Keep APVTS IDs `inTrim` / `outPad` / `autoGain`; add `color` default **0**. v1 sessions must not reset Drive/Output/AG; missing Color → 0.
- Wireframe: input meter | Color | Drive (hero, larger) | Output | output meter; Auto Gain under Output.
- Color: 0–1 even-harmonic mix. **0 = v1.** No `%` on the editor. Labels **Classic** / **Even**. Host text Classic / Blend / Even, not `0%`/`100%`. FL hint bar best-effort (not a Test fail).
- Offline: Color=0 matches v1; Color>0 raises even harmonics at matched Drive/Output. Do not gut DSP for stale “In Trim” ACs. Ship SHA ancestor of HEAD. Missing DAWs `FAIL-UNVERIFIED`.

**Out:** Mix, Hard, Dark, four-seat Mode, deleting Output, renaming Auto Gain, oversampling, Windows/CLAP, presets, hiss, MackEQ, CI.

**Deferred:** retuning max even (locked below); chasing FL’s native hint-bar number; Logic/Reaper in-app until those apps exist.

**Premises:** AG then Output stays; Color is one continuum (odd→even), not a Mode pack. v1 editor is not the wireframe yet. `jlimit` is not a NaN gate.

## Architecture

**Color lives only in `MackityEngine`**, immediately after LP-A and before `saturate`. `bias = clamp01(Color) * kColorBiasMax` with **`kColorBiasMax = 0.12f`**. If Color is 0, skip the add (v1 path / unity fast path still eligible). Color>0 must not use a Color-blind fast path.

Processor adds APVTS `color`, passes `clamp01(Color)` into `process`. After `replaceState`, if `color` is absent, force 0. Legacy 8-byte A/B: AG off, Color 0. Editor: no Color text box; Classic left, Even right.

```
Color audio: 0 → skip bias (v1)
             (0,1] → +bias then existing clip → DC-B / AG / pad
             NaN Color → clamp01 → 0
State:       v1 XML → Color 0; v1.1 XML → four params
```

**Critical Build traps:** (1) patch `step` but not the unity fast path; (2) sticky Color after loading v1 XML onto a reused processor.

## Test Matrix

| Area | Unit | Integration | Notes |
|---|---|---|---|
| Engine Color | Color=0 vs v1 / fixtures; H2 vs Color=0; clamp; no-op paths | — | Silence-is-zero only at Color=0 |
| Params/state | names, IDs, defaults | XML, v1 load, 8-byte, processor vs engine | Supersede In Trim / Out Pad / AutoGain strings |
| Editor | — | labels, no Color %, layout vs Drive/AG, meters/About | Not pixel-perfect mock; not FL screenshot |
| Host/docs | Color `getText` | auval 4 params if run | FL hint % not a fail; missing DAWs FAIL-UNVERIFIED |

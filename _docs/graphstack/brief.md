# Brief: mach1 control language (v1.1)

## Problem

mach1 v1 works — including in FL Studio — but the panel does not explain itself. **In Trim** and **Out Pad** are Mackity/desk jargon. Someone who has never used Mackity cannot tell which knob is drive and which is level. Auto Gain next to **Out Pad** looks like a second volume, so the useful combination (match loudness, then trim the insert) reads as a contradiction.

Meters already exist; they are not the story. Hierarchy and names are. A first-time user will not make this the first insert until the big knob is obviously “the grit” and the right-hand column is obviously “loudness after grit.”

## Target User

A producer in FL Studio (Logic/Reaper later) who would park this on many tracks **this week** if the panel matched what their ears do. Not a Mackity historian. You are the first of those users.

## Core Wedge

**Readable panel + one Color axis.** Layout from the wireframe. No v1 controls removed.

Left → right: **input meter** | **Color** | **Drive** (hero, larger) | **Output** | **output meter**. **Auto Gain** sits under Output.

1. **Display names** (host wrapper + editor match). Keep APVTS IDs or migrate state so sessions do not reset.
   - In Trim → **Drive**
   - Out Pad → **Output**
   - Auto Gain stays **Auto Gain** (match wet loudness to dry while you Drive; Output still trims after that)
2. **Color is even-harmonic mix, not “how hard it hits.”** Internally 0–1 (Plan). **0 = v1 Mackity** (odd/symmetric). Turning up adds even harmonics only — Drive is still what hits hard. Existing sessions load 0.
   - **No percentage (or 0–100) on the panel.** Users will read 0 as “off” and 100 as “max slam.” Neither is true.
   - Panel: knob + **Classic** at the left stop and **Even** at the right (or a home mark at Classic). Default looks like a starting sound, not an empty pot.
   - Host/automation text: not `0%` / `100%`. Custom strings (e.g. Classic → Even). FL’s hint bar must not reintroduce a number if we can help it.
3. **Meters stay** as in/out loudness so Auto Gain vs Output is visible.
4. **Not Mix, Hard, or Dark this loop.** Those are other axes. Four named seats (Classic / Hard / Bias / Dark) do **not** belong on a knob called Color.

Deliberately not this wedge: deleting Output, oversampling, Windows/CLAP, presets, hiss, MackEQ, a MoMa-style Mode (Glue/Mojo × even/odd codes), renaming Auto Gain, CI unless Plan adds it as infra.

## Assumptions

- Auto Gain + Output together is **correct**: Auto Gain ≈ wet RMS to dry, then Output. Confusion is naming and grouping, not topology.
- Hero size on Drive is correct; Color and Output stay smaller and symmetric.
- Auto Gain under Output is correct; do not center it under Drive.
- Four discrete “characters” made sense as a **Mode** switch. Once the control is **Color**, one continuum is honest: odd → even. Hard (clip shape) and Dark (spectrum) would fight Bias on the same 0–100% and become another unexplained knob.
- Color is not MoMa MODE. MoMa is two circuits × even/odd. mach1 Color is only the even/odd mix on this clipper.
- Learnings: do not gut DSP for stale ACs; Review/Test/evidence on the SHA you ship; missing DAWs stay `FAIL-UNVERIFIED`.

## Open Questions

- Scale: APVTS 0–1, no `%` suffix. `textFromValue` / `valueFromText` so the wrapper does not say 0% = nothing. How hard we fight FL’s native readout is Plan.
- How much even at 100% — Plan locks a max bias that still sounds like this plugin, not a different saturator. Offline fixture: Color=0 matches v1; Color>0 is audibly even at matched Drive/Output.

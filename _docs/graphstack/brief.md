# Brief: mach1

## Problem

Mackity (Airwindows, MIT) is a vintage Mackie 1202 (pre-VLZ) input-stage saturator: high-pass, drive, cheap-op-amp clip/odd-harmonic slam, ultrasonic lowpass, output pad. It is well liked and still in daily use as a channel insert — often on many tracks at once.

The published plugin is a 2010s VST2-era C++ effect with a two-knob generic UI. The DSP itself is a short stereo loop, but it pays for that loop with per-sample `pow()`, TPDF dither that calls `frexpf`/`pow` every sample, biquad coefficient rebuilds (including `tan`) on every process call, duplicated float/double paths, and no SIMD. Stack enough instances and a DAW session falls over. The original author is no longer iterating on it.

What’s missing is not “another saturator.” It’s this specific slam, cheap enough to live on every channel, with a UI and a couple of mix-workflow controls a 2026 plugin is expected to have.

## Target User

You: a producer/mixer who already reaches for Mackity as the first insert when you want that spongy, ugly-beautiful 90s desk grit. You would notice tomorrow if it vanished. The DAW crash from instance count is your actual blocker; the barebones UI is why you also want a revamp rather than a silent DSP patch.

## Core Wedge

**mach1 v1** is a stereo VST3 + AU channel saturator that:

1. Recreates Mackity’s 1202-input character (In Trim, Out Pad, same filter/clip topology) as native, realtime-safe C++ — rewritten for CPU, not re-interpreted through a general-purpose audio graph.
2. Ships with a custom, modern editor (not the host’s generic sliders): dense, premium, “boutique analog box” — meters, labeled controls, dark/industrial visual language.
3. Adds **auto-gain** so driving In Trim doesn’t require a matching Out Pad hunt; output loudness stays in the neighborhood of the dry signal while the character still changes.

Deliberately not in v1: MackEQ’s two-band EQ, a full mixer/console, oversampling as a default (only if A/B against Mackity proves aliasing is the sound), Windows, CLAP, AAX, presets marketplace, noise/hiss modeling, or a JS DSP graph as the audio path.

Suggested later stages (not this wedge): Windows/CLAP, optional oversampling, a small preset set, maybe a “desk EQ” sibling. Those wait until v1 is stable in *your* DAW with many instances.

## Assumptions

- **Licensing:** Mackity is MIT (Airwindows). mach1 may port the algorithm with attribution in the repo/About; it is a new product name and UI, not a fork that ships Airwindows branding.
- **Fidelity:** Match Mackity’s vibe closely enough that a session swapping Mackity → mach1 at equivalent In Trim / Out Pad still “is that plugin.” Bit-identical is not required; CPU and auto-gain will change the numbers.
- **Stack:** JUCE (or equivalent CMake plugin wrapper) for VST3/AU hosting, parameters, and state. **Native C++ DSP** for the audio thread. **WebView UI** (React + CSS) for the editor — this is the Lunacy-shaped part (JS for look and interaction), not Elementary as the saturator. Elementary’s declarative graph + JS runtime is a poor fit for a tight, static, many-instance clipper; it remains a later option for prototyping extra FX or analysis nodes, not the v1 audio path.
- **Platform:** macOS first (your machine). Install as a local developer build into the usual VST3/AU folders.
- **Auto-gain:** loudness matching of wet vs dry (or vs a unity-gain reference), not a compressor. User can still turn it off and use Out Pad by hand.
- **CPU success:** “many instances on a mix without the session dying” — Plan should pick a measurable bar (e.g. N stereo instances at 48 kHz / 64-sample buffer on this Mac vs Mackity).

## Open Questions

- Visual direction beyond “premium / modern / trendy”: specific references (Lunacy, Softube, Plugin Alliance, something else)?
- Auto-gain target: match dry RMS, match dry peak, or match a calibrated “unity In Trim” reference?
- Hosts that must work on day one (Logic, Ableton, Reaper, …)?
- Whether a dry/wet or “drive amount” control is needed besides In Trim / Out Pad / auto-gain.

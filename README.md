# mach1

macOS **arm64** Audio Unit + VST3 saturator. Product name is **mach1** (not Airwindows, not Mackity). Third-party attribution and the JUCE license note live in [`NOTICE`](NOTICE).

Manufacturer / plugin four-char identity: **Stao** / **Mh01**.

## Controls

Four host-facing parameters (display names). APVTS IDs are unchanged (`inTrim`, `outPad`, `autoGain`, `color`).

- **Drive** — saturation amount
- **Output** — output level
- **Auto Gain** — when on, holds loudness closer to dry while you drive
- **Color** — even-harmonic mix. **0 is the Mackity / v1 sound, not “off.”** Host text is Classic (0), Blend (in between), Even (1). An FL Studio hint bar still showing a number or `%` is not a fail of that host text.

## Build

Requires CMake 3.22+ and a C++17 toolchain. The root `CMakeLists.txt` sets `CMAKE_OSX_ARCHITECTURES` to `arm64`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

JUCE 8.0.15 is fetched automatically (FetchContent). Do not add a second JUCE pin.

## Local install (user plugin folders)

`juce_add_plugin` already has `COPY_PLUGIN_AFTER_BUILD TRUE`. After a normal CMake plugin build, JUCE copies the bundles into the **user** folders — no second installer and no extra copy step:

- `~/Library/Audio/Plug-Ins/VST3/mach1.vst3`
- `~/Library/Audio/Plug-Ins/Components/mach1.component`

## Validate AU

```sh
tests/host_validation.sh --auval
```

That rebuilds the AU and runs `auval -v aufx Mh01 Stao`. A pass requires exit 0 **and** exactly four user parameters named **Drive**, **Output**, **Auto Gain**, and **Color**. If `auval` is missing, hangs, or the listing cannot be captured, the script prints `FAIL-UNVERIFIED` (not a skip-pass). A three-parameter In Trim / Out Pad / AutoGain listing is a fail.

Logic in-app insert and Reaper VST3 E2E are separate (`--logic` / `--reaper`). Those print `FAIL-UNVERIFIED` when the apps are absent or unscriptable; that does not pass or fail the auval four-name gate. Human steps: `tests/MANUAL_CHECKLIST.md` (documenting the checklist is not a pass).

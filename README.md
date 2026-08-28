# mach1

macOS **arm64** Audio Unit + VST3 saturator. Product name is **mach1** (not Airwindows, not Mackity). Third-party attribution and the JUCE license note live in [`NOTICE`](NOTICE).

Manufacturer / plugin four-char identity: **Stao** / **Mh01**.

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
auval -v aufx Mh01 Stao
```

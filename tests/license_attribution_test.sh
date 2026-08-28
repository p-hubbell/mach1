#!/usr/bin/env bash
# Checks NOTICE / README against license-attribution-install ACs, and that
# PluginEditor About still credits Mackity MIT under product name mach1.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NOTICE="${ROOT}/NOTICE"
README="${ROOT}/README.md"
EDITOR="${ROOT}/src/PluginEditor.cpp"
fail() { echo "FAIL: $*" >&2; exit 1; }

[[ -f "${NOTICE}" ]] || fail "NOTICE missing at repository root"
[[ -f "${README}" ]] || fail "README.md missing at repository root"

notice="$(<"${NOTICE}")"
readme="$(<"${README}")"
about="$(<"${EDITOR}")"

contains() {
    local haystack="$1" needle="$2" label="$3"
    case "${haystack}" in
        *"${needle}"*) ;;
        *) fail "${label} missing required text: ${needle}" ;;
    esac
}

forbids() {
    local haystack="$1" needle="$2" label="$3"
    case "${haystack}" in
        *"${needle}"*) fail "${label} must not contain: ${needle}" ;;
    esac
}

# NOTICE: Mackity MIT attribution; product is mach1.
contains "${notice}" "Airwindows Mackity" "NOTICE"
contains "${notice}" "MIT" "NOTICE"
contains "${notice}" "mach1" "NOTICE"
contains "${notice}" "algorithm" "NOTICE"
contains "${notice}" "not this product" "NOTICE"

# NOTICE: JUCE facts (not a mach1 product-license decision).
contains "${notice}" "FetchContent" "NOTICE"
contains "${notice}" "8.0.15" "NOTICE"
contains "${notice}" "GPL unless a commercial/paid license" "NOTICE"
contains "${notice}" "No paid JUCE license is recorded in this repository yet" "NOTICE"
contains "${notice}" "JUCE’s own license applies" "NOTICE"

forbids "${notice}" "chose GPL for mach1" "NOTICE"
forbids "${notice}" "chose GPL" "NOTICE"
forbids "${notice}" "commercial license exists" "NOTICE"
forbids "${notice}" "we have a paid JUCE license" "NOTICE"

# README: cmake, COPY_PLUGIN user paths, arm64; no Windows paths / extra installer.
contains "${readme}" "arm64" "README"
contains "${readme}" "cmake -S . -B build" "README"
contains "${readme}" "COPY_PLUGIN_AFTER_BUILD" "README"
contains "${readme}" "~/Library/Audio/Plug-Ins/VST3/mach1.vst3" "README"
contains "${readme}" "~/Library/Audio/Plug-Ins/Components/mach1.component" "README"
contains "${readme}" "Stao" "README"
contains "${readme}" "Mh01" "README"
contains "${readme}" "auval -v aufx Mh01 Stao" "README"

forbids "${readme}" "Windows" "README"
forbids "${readme}" "C:\\" "README"
forbids "${readme}" "Program Files" "README"
forbids "${readme}" "pkg" "README"
forbids "${readme}" "dmg" "README"

# About in PluginEditor stays aligned with NOTICE.
contains "${about}" "Airwindows Mackity" "PluginEditor About"
contains "${about}" "MIT" "PluginEditor About"
contains "${about}" "mach1" "PluginEditor About"
contains "${about}" "not titled Mackity" "PluginEditor About"
forbids "${about}" "chose GPL" "PluginEditor About"

echo "PASS license-attribution-install NOTICE/README/About checks"

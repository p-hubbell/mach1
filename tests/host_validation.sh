#!/usr/bin/env bash
# Host gates: auval (required), host-free mono via existing ctest, Logic/Reaper E2E.
# auval pass is not in-app insert proof. Missing/unscriptable DAWs print FAIL-UNVERIFIED
# and must not skip-pass or fail the auval / passthrough bars.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${MACH1_BUILD_DIR:-${REPO_ROOT}/build}"
MODE="${1:-all}"

AUVAL_TYPE="aufx"
AUVAL_SUBTYPE="Mh01"
AUVAL_MANU="Stao"

LOGIC_CANDIDATES=(
    "/Applications/Logic Pro.app"
    "/Applications/Logic Pro X.app"
)
REAPER_APP="/Applications/REAPER.app"

usage() {
    echo "Usage: $0 [all|--auval|--logic|--reaper|--help]"
    echo "  all (default)  rebuild AU, auval, document passthrough, print unverified E2E"
    echo "  --auval        rebuild AU and require auval -v ${AUVAL_TYPE} ${AUVAL_SUBTYPE} ${AUVAL_MANU} exit 0"
    echo "  --logic        Logic in-app stereo/mono; FAIL-UNVERIFIED if absent or unscriptable (exit 2)"
    echo "  --reaper       Reaper VST3 E2E; FAIL-UNVERIFIED if ${REAPER_APP} absent (exit 2)"
}

find_logic() {
    local p
    for p in "${LOGIC_CANDIDATES[@]}"; do
        if [[ -d "$p" ]]; then
            printf '%s' "$p"
            return 0
        fi
    done
    return 1
}

# No GUI driver is in-repo (out of scope). Presence alone is not scriptable.
logic_scriptable() {
    return 1
}

rebuild_au() {
    if [[ ! -d "${BUILD_DIR}" ]]; then
        echo "FAIL: build directory missing (${BUILD_DIR}); configure CMake first" >&2
        return 1
    fi
    echo "Rebuilding mach1_AU (COPY_PLUGIN_AFTER_BUILD) so auval sees the saturator..."
    cmake --build "${BUILD_DIR}" --target mach1_AU
}

run_auval() {
    rebuild_au
    echo "Running: auval -v ${AUVAL_TYPE} ${AUVAL_SUBTYPE} ${AUVAL_MANU}"
    auval -v "${AUVAL_TYPE}" "${AUVAL_SUBTYPE}" "${AUVAL_MANU}"
}

document_passthrough() {
    echo "HOST-FREE MONO: already covered by ctest 'passthrough' (executable mach1_passthrough_test)."
    echo "Not duplicated here. That test exercises isBusesLayoutSupported / prepare / processBlock on 1-in/1-out."
}

run_logic_e2e() {
    local logic_path=""
    if logic_path="$(find_logic)"; then
        if logic_scriptable; then
            echo "FAIL: Logic is present at ${logic_path} but in-app insert automation is not implemented" >&2
            return 1
        fi
        echo "FAIL-UNVERIFIED"
        echo "FAIL-UNVERIFIED logic-in-app-stereo: ${logic_path} present but GUI is not scriptable"
        echo "FAIL-UNVERIFIED logic-in-app-mono: ${logic_path} present but GUI is not scriptable"
        echo "See tests/MANUAL_CHECKLIST.md (documenting the checklist is not a pass)."
        return 2
    fi
    echo "FAIL-UNVERIFIED"
    echo "FAIL-UNVERIFIED logic-in-app-stereo: Logic.app absent"
    echo "FAIL-UNVERIFIED logic-in-app-mono: Logic.app absent"
    echo "See tests/MANUAL_CHECKLIST.md (documenting the checklist is not a pass)."
    return 2
}

run_reaper_e2e() {
    if [[ ! -d "${REAPER_APP}" ]]; then
        echo "FAIL-UNVERIFIED"
        echo "FAIL-UNVERIFIED reaper-vst3: ${REAPER_APP} absent"
        return 2
    fi
    echo "FAIL: ${REAPER_APP} is present but automated VST3 scan/insert/automation/save-reload is not implemented" >&2
    return 1
}

case "${MODE}" in
    --help|-h)
        usage
        exit 0
        ;;
    --auval)
        run_auval
        exit $?
        ;;
    --logic)
        run_logic_e2e
        exit $?
        ;;
    --reaper)
        run_reaper_e2e
        exit $?
        ;;
    all)
        run_auval
        document_passthrough
        # Return 2 = FAIL-UNVERIFIED (do not fail this combined auval-gated run).
        # Return 1 = hard FAIL (e.g. Reaper present but automation missing) — do not green-wash.
        set +e
        run_logic_e2e
        local_logic=$?
        run_reaper_e2e
        local_reaper=$?
        set -e
        if [[ "${local_logic}" -eq 1 || "${local_reaper}" -eq 1 ]]; then
            echo "AUVAL: PASS. Combined run FAILED because a host E2E hard-failed (not unverified)." >&2
            exit 1
        fi
        echo "AUVAL: PASS (exit 0). Logic in-app stereo/mono and Reaper VST3 were not claimed passed."
        exit 0
        ;;
    *)
        usage >&2
        exit 1
        ;;
esac

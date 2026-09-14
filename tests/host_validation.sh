#!/usr/bin/env bash
# Host gates: auval (required when runnable), host-free mono via existing ctest, Logic/Reaper E2E.
# auval pass is not in-app insert proof. Missing/unscriptable DAWs print FAIL-UNVERIFIED
# and must not skip-pass or fail the auval / passthrough bars.
# Missing/hanging auval, or an unparseable parameter listing, prints FAIL-UNVERIFIED
# (not a skip-pass as if names were verified). A 3-param In Trim listing is a hard fail.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${MACH1_BUILD_DIR:-${REPO_ROOT}/build}"
MODE="${1:-all}"

AUVAL_TYPE="aufx"
AUVAL_SUBTYPE="Mh01"
AUVAL_MANU="Stao"
AUVAL_TIMEOUT_SEC="${MACH1_AUVAL_TIMEOUT:-120}"

LOGIC_CANDIDATES=(
    "/Applications/Logic Pro.app"
    "/Applications/Logic Pro X.app"
)
REAPER_APP="/Applications/REAPER.app"

usage() {
    echo "Usage: $0 [all|--auval|--logic|--reaper|--help]"
    echo "  all (default)  rebuild AU, auval four-name gate, document passthrough, print unverified E2E"
    echo "  --auval        rebuild AU and require auval listing Drive / Output / Auto Gain / Color"
    echo "  --logic        Logic in-app stereo/mono; FAIL-UNVERIFIED if absent or unscriptable (exit 2)"
    echo "  --reaper       Reaper VST3 E2E; FAIL-UNVERIFIED if ${REAPER_APP} absent (exit 2)"
}

fail_unverified() {
    echo "FAIL-UNVERIFIED"
    echo "FAIL-UNVERIFIED $*"
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

check_auval_param_listing() {
    python3 "${SCRIPT_DIR}/check_auval_listing.py" "$1"
}

run_auval_timed() {
    python3 - "$AUVAL_TIMEOUT_SEC" "$AUVAL_TYPE" "$AUVAL_SUBTYPE" "$AUVAL_MANU" <<'PY'
import subprocess
import sys

timeout_sec = float(sys.argv[1])
cmd = ["auval", "-v", sys.argv[2], sys.argv[3], sys.argv[4]]
try:
    proc = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        timeout=timeout_sec,
    )
except subprocess.TimeoutExpired as exc:
    out = (exc.stdout or "") + (exc.stderr or "")
    sys.stdout.write(out)
    sys.stderr.write(f"auval timed out after {timeout_sec:.0f}s\n")
    sys.exit(124)
except FileNotFoundError:
    sys.stderr.write("auval binary not found\n")
    sys.exit(127)

sys.stdout.write(proc.stdout or "")
sys.stderr.write(proc.stderr or "")
sys.exit(proc.returncode)
PY
}

run_auval() {
    rebuild_au

    if ! command -v python3 >/dev/null 2>&1; then
        fail_unverified "auval: cannot parse listing (python3 missing)"
        return 2
    fi
    if ! command -v auval >/dev/null 2>&1; then
        fail_unverified "auval: binary missing; four-name listing not captured"
        return 2
    fi

    echo "Running: auval -v ${AUVAL_TYPE} ${AUVAL_SUBTYPE} ${AUVAL_MANU}"
    local auval_out auval_err auval_rc parse_out parse_status parse_names
    local tmp_out tmp_err
    tmp_out="$(mktemp)"
    tmp_err="$(mktemp)"
    set +e
    run_auval_timed >"${tmp_out}" 2>"${tmp_err}"
    auval_rc=$?
    set -e
    auval_out="$(cat "${tmp_out}")"
    auval_err="$(cat "${tmp_err}")"
    rm -f "${tmp_out}" "${tmp_err}"
    printf '%s' "${auval_out}"
    if [[ -n "${auval_err}" ]]; then
        printf '%s\n' "${auval_err}" >&2
    fi

    if [[ "${auval_rc}" -eq 127 ]]; then
        fail_unverified "auval: binary missing; four-name listing not captured"
        return 2
    fi
    if [[ "${auval_rc}" -eq 124 ]]; then
        fail_unverified "auval: hung/timed out; four-name listing not captured"
        return 2
    fi

    local listing_file
    listing_file="$(mktemp)"
    printf '%s\n%s\n' "${auval_out}" "${auval_err}" >"${listing_file}"
    parse_out="$(check_auval_param_listing "${listing_file}")"
    rm -f "${listing_file}"
    parse_status="$(printf '%s\n' "${parse_out}" | sed -n '1p')"
    parse_names="$(printf '%s\n' "${parse_out}" | sed -n '2p')"

    if [[ "${parse_status}" == "fail" ]]; then
        echo "FAIL: auval parameter listing is not exactly Drive, Output, Auto Gain, Color (${parse_names})" >&2
        return 1
    fi
    if [[ "${parse_status}" != "ok" ]]; then
        fail_unverified "auval: parameter listing not captured (cannot verify Drive/Output/Auto Gain/Color)"
        return 2
    fi
    if [[ "${auval_rc}" -ne 0 ]]; then
        echo "FAIL: auval -v ${AUVAL_TYPE} ${AUVAL_SUBTYPE} ${AUVAL_MANU} exited ${auval_rc}" >&2
        return 1
    fi
    echo "AUVAL PARAMS: ${parse_names} (exactly four user parameters)"
    return 0
}

document_passthrough() {
    echo "HOST-FREE MONO: already covered by ctest 'passthrough' (executable mach1_passthrough_test)."
    echo "Not duplicated here. That test exercises isBusesLayoutSupported / prepare / processBlock on 1-in/1-out."
    echo "Color host text (Classic / Blend / Even, no %) is covered there; FL hint-bar % is not a fail."
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
        set +e
        run_auval
        local_auval=$?
        set -e
        document_passthrough
        # Return 2 = FAIL-UNVERIFIED (do not fail this combined auval-gated run).
        # Return 1 = hard FAIL (e.g. Reaper present but automation missing) — do not green-wash.
        set +e
        run_logic_e2e
        local_logic=$?
        run_reaper_e2e
        local_reaper=$?
        set -e
        if [[ "${local_auval}" -eq 1 || "${local_logic}" -eq 1 || "${local_reaper}" -eq 1 ]]; then
            echo "Combined run FAILED because auval names or a host E2E hard-failed (not unverified)." >&2
            exit 1
        fi
        if [[ "${local_auval}" -ne 0 ]]; then
            echo "AUVAL: FAIL-UNVERIFIED (four-name listing not captured). Logic/Reaper were not claimed passed."
            exit 2
        fi
        echo "AUVAL: PASS (exit 0, four params Drive/Output/Auto Gain/Color). Logic in-app stereo/mono and Reaper VST3 were not claimed passed."
        exit 0
        ;;
    *)
        usage >&2
        exit 1
        ;;
esac

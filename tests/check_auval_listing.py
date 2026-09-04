#!/usr/bin/env python3
"""Classify auval published-parameter listings for host_validation.sh."""

from __future__ import annotations

import re
import sys
from pathlib import Path

EXPECTED = {"Drive", "Output", "Auto Gain", "Color"}
BANNED = {"In Trim", "Out Pad", "AutoGain"}


def classify(text: str) -> tuple[str, str]:
    start = text.find("PUBLISHED PARAMETER INFO")
    if start < 0:
        start = 0
    rest = text[start:]
    end_m = re.search(r"\n\s*FORMAT TESTS:", rest)
    section = rest[: end_m.start()] if end_m else rest

    count = None
    cm = re.search(r"(\d+)\s+Global Scope Parameters", section, re.IGNORECASE)
    if cm is None:
        cm = re.search(r"#\s*parameters:\s*(\d+)", section, re.IGNORECASE)
    if cm is not None:
        count = int(cm.group(1))

    names = [n.strip() for n in re.findall(r"(?m)^\s*Name:\s*(.+?)\s*$", section)]
    if not names:
        names = [n.strip() for n in re.findall(r"Parameter\s+\d+:\s*([^,\n]+)", section)]

    joined = ",".join(names) if names else (f"count={count}" if count is not None else "")

    if count == 3 or (names and BANNED.intersection(names)):
        return "fail", joined

    if count is None and not names:
        return "unverified", joined

    if count is not None and not names:
        if count != 4:
            return "fail", joined
        return "unverified", joined

    if len(names) != 4 or set(names) != EXPECTED:
        return "fail", joined

    return "ok", ",".join(names)


def _self_test() -> int:
    four = """
PUBLISHED PARAMETER INFO:

# # # 4 Global Scope Parameters:

Parameter ID:0
Name: Drive
Parameter ID:1
Name: Output
Parameter ID:2
Name: Auto Gain
Parameter ID:3
Name: Color

FORMAT TESTS:
"""
    three = """
PUBLISHED PARAMETER INFO:
# # # 3 Global Scope Parameters:
Parameter 0: In Trim, 0.1, 0, 1
Parameter 1: Out Pad, 1.0, 0, 1
Parameter 2: AutoGain, 1.0, 0, 1
FORMAT TESTS:
"""
    empty = "AU VALIDATION SUCCEEDED.\n"
    status, names = classify(four)
    if status != "ok" or set(names.split(",")) != EXPECTED:
        print("self-test four-param listing failed", status, names, file=sys.stderr)
        return 1
    status, names = classify(three)
    if status != "fail" or "In Trim" not in names:
        print("self-test 3-param In Trim must fail", status, names, file=sys.stderr)
        return 1
    status, _ = classify(empty)
    if status != "unverified":
        print("self-test empty listing must be unverified", status, file=sys.stderr)
        return 1
    print("check_auval_listing self-test passed")
    return 0


def main(argv: list[str]) -> int:
    if argv == ["--self-test"]:
        return _self_test()
    if len(argv) != 1:
        print("usage: check_auval_listing.py <auval-log> | --self-test", file=sys.stderr)
        return 2
    status, names = classify(Path(argv[0]).read_text())
    print(status)
    print(names)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

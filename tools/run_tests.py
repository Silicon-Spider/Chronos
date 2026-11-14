#!/usr/bin/env python3
"""Run the copied t5d-tests runner and report pass and fail counts per suite.

The copied runner (engine/tests/test_main.cpp) has no suites: a suite here is the
source file a TEST case is defined in, found by reading engine/tests/*.cpp. The runner
itself, and every test, run unchanged.

The runner has no way to exclude a test. When the skip list names tests, this script
runs every other selected test by exact name, as name fragments, in batches, and
checks that each batch selects exactly the tests it names (the runner matches
fragments as substrings). Each skipped test must exist; a stale skip list is an error.

Usage:
  python tools/run_tests.py --exe build/engine/t5d-tests [--long] [--json out.json]
Exit status: 0 when every test that ran passed, 1 otherwise.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import time
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Tests that need a program Chronos doesn't build, with the reason (docs/gates/G0.md).
SKIP = {
    "measure_exe_turns_on_hand_written_records":
        "runs t5d-measure, which is not copied (it includes t5d/expand.hpp, spec 3.2)",
    "measure_exe_turn_summary_classifies_records":
        "runs t5d-measure, which is not copied (it includes t5d/expand.hpp, spec 3.2)",
    "measure_exe_turn_summary_ranges_at_their_edges":
        "runs t5d-measure, which is not copied (it includes t5d/expand.hpp, spec 3.2)",
}

TEST_DEF = re.compile(r"^\s*TEST\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)", re.M)
RESULT = re.compile(r"^\[( ok |FAIL)\] (\S+)")


def suites_by_name(tests_dir: Path) -> dict[str, str]:
    owner: dict[str, str] = {}
    for f in sorted(tests_dir.glob("*.cpp")):
        for name in TEST_DEF.findall(f.read_text(encoding="utf-8", errors="replace")):
            if name in owner:
                raise SystemExit(f"test {name} is defined in both {owner[name]} and {f.name}")
            owner[name] = f.name
    return owner


def run(exe: str, args: list[str]) -> subprocess.CompletedProcess:
    return subprocess.run([exe, *args], capture_output=True, text=True, errors="replace")


def batches(names: list[str], every: list[str], limit: int = 28000) -> list[list[str]]:
    """Split names into batches whose fragments select exactly themselves."""
    def selects(batch: list[str]) -> set[str]:
        return {t for t in every if any(f in t for f in batch)}

    out, cur, size = [], [], 0
    for n in names:
        if cur and size + len(n) + 1 > limit:
            out.append(cur)
            cur, size = [], 0
        cur.append(n)
        size += len(n) + 1
    if cur:
        out.append(cur)
    for b in out:
        extra = selects(b) - set(b)
        if extra:
            raise SystemExit(f"a name fragment also selects {sorted(extra)}: run them separately")
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--exe", required=True)
    ap.add_argument("--tests-dir", default=str(ROOT / "engine" / "tests"))
    ap.add_argument("--long", action="store_true", help="also run the long_ cross-checks")
    ap.add_argument("--json", help="write the per-test results here")
    a = ap.parse_args()

    exe = Path(a.exe)
    if not exe.is_file() and exe.with_suffix(".exe").is_file():
        exe = exe.with_suffix(".exe")  # Windows: CreateProcess wants the extension
    a.exe = str(exe.resolve())
    owner = suites_by_name(Path(a.tests_dir))
    flag = ["--long"] if a.long else []
    listed = run(a.exe, ["--list", *flag])
    if listed.returncode != 0:
        print(listed.stdout, listed.stderr)
        raise SystemExit("t5d-tests --list failed")
    selected = [ln.strip() for ln in listed.stdout.splitlines() if ln.strip()]
    stale = sorted(n for n in SKIP if n not in selected)
    if stale:
        raise SystemExit(f"skip list names tests the runner doesn't have: {stale}")
    to_run = [n for n in selected if n not in SKIP]

    t0 = time.monotonic()
    outputs = []
    if len(to_run) == len(selected):
        outputs.append(run(a.exe, flag))  # exactly as upstream: no selection at all
    else:
        for b in batches(to_run, selected):
            outputs.append(run(a.exe, [*flag, *b]))
    seconds = time.monotonic() - t0

    status: dict[str, str] = {}
    stderr = []
    for p in outputs:
        stderr.append(p.stderr)
        for line in p.stdout.splitlines():
            m = RESULT.match(line)
            if m:
                status[m.group(2)] = "pass" if m.group(1) == " ok " else "fail"
    missing = [n for n in to_run if n not in status]
    for n in missing:
        status[n] = "not reported"
    for n in SKIP:
        status[n] = "skipped"

    per = defaultdict(Counter)
    for n in selected:
        per[owner.get(n, "?")][status[n]] += 1
    total = Counter(status[n] for n in selected)
    width = max(len(s) for s in per) if per else 10
    print(f"{'suite (source file)':<{width}}  pass  fail  skip")
    for suite in sorted(per):
        c = per[suite]
        print(f"{suite:<{width}}  {c['pass']:4}  {c['fail'] + c['not reported']:4}  {c['skipped']:4}")
    print(f"{'total':<{width}}  {total['pass']:4}  {total['fail'] + total['not reported']:4}  "
          f"{total['skipped']:4}   ({len(selected)} selected, {seconds:.1f} s)")
    for n in SKIP:
        print(f"skipped {n}: {SKIP[n]}")
    failed = [n for n in selected if status[n] in ("fail", "not reported")]
    for n in failed:
        print(f"FAILED  {n} ({owner.get(n, '?')}): {status[n]}")
    if failed:
        print("".join(stderr)[-20000:], file=sys.stderr)
    if a.json:
        Path(a.json).write_text(json.dumps({
            "selected": len(selected), "seconds": seconds, "long": a.long,
            "tests": [{"name": n, "suite": owner.get(n, "?"), "status": status[n]} for n in selected],
            "skipped": SKIP}, indent=1) + "\n", encoding="utf-8")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

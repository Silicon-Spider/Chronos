#!/usr/bin/env python3
"""Turn-generation figures for docs/MEASUREMENTS.md from chronos-bench's JSON.

chronos-bench prints its turn-generation table over all positions. A position with no
legal turn (a mate or stalemate, or a pseudo-legal synthetic position) costs a proof of
none instead of turns, so this script splits each set into positions with at least one
legal turn and positions with none, from the per-position medians the bench writes
(per_position = [first_us, turns27_us, done_us, turns, nodes, exhausted, stopped, refused]).

Percentiles are nearest rank, as chronos-bench computes them: the p90 of n values is the
ceil(0.9 n)-th smallest. Turns/s is pooled: all turns over all time.

Usage: python tools/bench/turngen_table.py build/g0/bench.json
"""
from __future__ import annotations

import json
import math
import statistics
import sys


def p90(values: list[float]) -> float:
    v = sorted(values)
    return v[math.ceil(0.9 * len(v)) - 1]


def main() -> int:
    data = json.load(open(sys.argv[1], encoding="utf-8"))
    print("| Positions | With a legal turn | First turn, µs, median [p90] | Up to 27 turns, µs | "
          "µs per turn, average to done | µs per further turn | Turns/s | No legal turn: count, median µs to prove none |")
    print("| --- | ---: | --- | ---: | ---: | ---: | ---: | --- |")
    for s in data["turngen"]["sets"]:
        pp = s["per_position"]
        has = [p for p in pp if p[3] > 0]
        none = [p for p in pp if p[3] == 0 and p[5]]
        first = [p[0] for p in has]
        to27 = [p[1] for p in has]
        avg = [p[2] / p[3] for p in has]
        further = [(p[2] - p[0]) / (p[3] - 1) for p in has if p[3] > 1]
        tps = sum(p[3] for p in has) / sum(p[2] for p in has) * 1e6
        nz = f"{len(none)}, {statistics.median(p[2] for p in none):,.1f}" if none else "0"
        print(f"| {s['name']} | {len(has):,} | {statistics.median(first):,.1f} [{p90(first):,.1f}] | "
              f"{statistics.median(to27):,.1f} | {statistics.median(avg):.2f} | "
              f"{statistics.median(further):.2f} | {tps:,.0f} | {nz} |")
    return 0


if __name__ == "__main__":
    sys.exit(main())

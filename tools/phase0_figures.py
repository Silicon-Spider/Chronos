#!/usr/bin/env python3
"""Further figures from Phase 0's raw outputs, for docs/MEASUREMENTS.md entry 2.

Nothing is measured here: every number is read from files Phase 0 wrote.
  --bench FILE    fractal-bench's JSON (turn-generation section)
  --hist FILE     fractal-relhist's histograms over the start games (--out)
  --probe FILE    fractal-relhist's reachability table (--probe)
  --records FILE  fractal-relhist's per-record table (--records-out)
  --split FILE    data/splits/t0database-nd8.tsv

It prints:
- how many turn starts have at least as many legal turns as the bench's cap;
- relations per position by kind (attack or defence), by where the victim stands
  (a head board or a history board) and by family;
- the live rows of the relation index when defences are kept only onto head boards
  (spec 7.3), for all twelve piece kinds and for the six gen-1 kinds;
- the delta-t placement buckets in whole turns (spec 7.4);
- how the usable human games end: timelines, turns and boards.

Usage:
  python tools/phase0_figures.py --bench build/g0/bench.json --hist build/g0/start.json
         --probe build/g0/probe.json --records build/g0/records.tsv
         --split data/splits/t0database-nd8.tsv
"""
from __future__ import annotations

import argparse
import csv
import itertools
import json
import math
import statistics
import sys
from collections import Counter, defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from relations_g0 import COLOURS, GEN1, KINDS, PAWNLIKE, class_of, distance, family  # noqa: E402

# G0's edges (docs/gates/G0.md, "Bucket edges").
EDGES = {"timeline": [1, 2, 3], "time": [1, 2, 3], "both": [1, 2]}
CLASSES = ["same board", "timeline 1", "timeline 2", "timeline 3+", "time 1", "time 2", "time 3+",
           "both 1", "both 2+"]


def turn_width(bench: dict) -> None:
    cap = bench["options"]["turn_cap"]
    print(f"## Turn starts with at least {cap:,} legal turns\n")
    print("| Positions | Turn starts | With a legal turn | At the cap | Share of those with a legal turn |")
    print("| --- | ---: | ---: | ---: | ---: |")
    for s in bench["turngen"]["sets"]:
        if s["kind"] != "records":
            continue
        legal = s["positions"] - s["no_legal_turn"]
        print(f"| {s['name']} | {s['positions']:,} | {legal:,} | {s['hit_turn_cap']:,} | "
              f"{100 * s['hit_turn_cap'] / legal:.1f}% |")


def relations(hist: dict) -> None:
    fields = hist["relations"]["fields"]
    ix = {f: i for i, f in enumerate(fields)}
    rows = hist["relations"]["rows"]
    n_pos = hist["meta"]["distinct_positions"]
    total = sum(r[ix["count"]] for r in rows)
    by = Counter()
    for r in rows:
        fam = family(r[ix["dL"]], r[ix["dTt"]])
        kind = "defence" if r[ix["own"]] else "attack"
        where = "head" if r[ix["vhead"]] else "history"
        by[(fam, kind, where)] += r[ix["count"]]
    print(f"\n## Relations in the start games: {total:,} over {n_pos:,} positions, "
          f"{total / n_pos:.1f} per position\n")
    print("| Family | Attacks onto heads | Attacks onto history | Defences onto heads | Defences onto history | All |")
    print("| --- | ---: | ---: | ---: | ---: | ---: |")
    cols = [("attack", "head"), ("attack", "history"), ("defence", "head"), ("defence", "history")]
    sums = Counter()
    for fam, name in (("same", "Same board"), ("timeline", "Timeline only"), ("time", "Time only"),
                      ("both", "Both")):
        cells = [by[(fam, k, w)] for k, w in cols]
        for c, v in zip(cols, cells):
            sums[c] += v
        print(f"| {name} | " + " | ".join(f"{v / n_pos:.2f}" for v in cells)
              + f" | {sum(cells) / n_pos:.2f} ({100 * sum(cells) / total:.1f}%) |")
    print("| **All** | " + " | ".join(f"{sums[c] / n_pos:.2f}" for c in cols) + f" | {total / n_pos:.2f} |")
    kept = total - sums[("defence", "history")]
    same = sum(by[("same", k, w)] for k, w in cols)
    print(f"\nDefences onto history boards: {sums[('defence', 'history')]:,}, "
          f"{100 * sums[('defence', 'history')] / total:.1f}% of all relations.")
    print(f"Attacks, and defences onto head boards: {kept:,}, {kept / n_pos:.1f} per position; "
          f"{same / n_pos:.1f} of them on the attacker's own board and {(kept - same) / n_pos:.1f} leaving it.")
    passed = sum(r[ix["count"]] for r in rows if r[ix["passed"]])
    print(f"Read on a passed copy: {passed:,}, {100 * passed / total:.1f}%.")


def live_rows(probe: dict) -> None:
    same_reach, across = {}, defaultdict(int)
    for k in probe["kinds"]:
        key = (k["kind"], k["colour"])
        same_reach[key] = int(k["same_board"]["union"], 16)
        for pair in k["across"]["pairs"]:
            fam = family(pair["dL"], pair["dTt"])
            cl = class_of(fam, distance(fam, pair["dL"], pair["dTt"]), EDGES)
            z = pair["zones"]
            across[(key[0], key[1], cl)] |= int(z, 16) if isinstance(z, str) else int(z)
    n = Counter()
    for ak, ac, vk, vc in itertools.product(KINDS, COLOURS, KINDS, COLOURS):
        g1 = ak in GEN1 and vk in GEN1
        defence = ac == vc
        for sq in range(64):
            if not (same_reach[(ak, ac)] >> sq) & 1:
                continue
            if vk in PAWNLIKE and sq // 8 == (7 if vc == "w" else 0):
                continue
            if ak == vk and ac == "b" and vc == "w":
                continue  # the second half of a same-kind enemy pair
            n["same", "all"] += 1
            n["same", "gen1"] += g1
        for cl in CLASSES[1:]:
            zones = bin(across.get((ak, ac, cl), 0)).count("1")
            if not zones:
                continue
            if defence and cl.startswith("time "):
                n["time-only defences", "all"] += zones
                n["time-only defences", "gen1"] += zones * g1
                continue
            n["cross", "all"] += zones
            n["cross", "gen1"] += zones * g1
    print("\n## Live relation rows with defences kept only onto head boards\n")
    print("| Rows | All twelve kinds | The six gen-1 kinds |")
    print("| --- | ---: | ---: |")
    print(f"| On the attacker's own board | {n['same', 'all']:,} | {n['same', 'gen1']:,} |")
    print(f"| Across boards | {n['cross', 'all']:,} | {n['cross', 'gen1']:,} |")
    print(f"| **Live** | **{n['same', 'all'] + n['cross', 'all']:,}** | "
          f"**{n['same', 'gen1'] + n['cross', 'gen1']:,}** |")
    print(f"| Time-only defence rows, live at G0 and dropped here | {n['time-only defences', 'all']:,} | "
          f"{n['time-only defences', 'gen1']:,} |")


def delta_t(hist: dict) -> None:
    fields = hist["placement"]["fields"]
    ix = {f: i for i, f in enumerate(fields)}
    names = ["behind", "0", "1", "2", "3-4", "5+"]
    b = Counter()
    for r in hist["placement"]["rows"]:
        d, c = r[ix["dt"]], r[ix["count"]]
        if d < 0:
            b["behind"] += c
        elif d == 0:
            b["0"] += c
        else:
            t = math.ceil(d / 2)
            b["1" if t == 1 else "2" if t == 2 else "3-4" if t <= 4 else "5+"] += c
    total = sum(b.values())
    print(f"\n## Delta t to the present in whole turns, {total:,} head boards\n")
    print("| Bucket | " + " | ".join(names) + " |")
    print("| --- |" + " ---: |" * len(names))
    print("| Head boards | " + " | ".join(f"{b[k]:,}" for k in names) + " |")
    print("| Share | " + " | ".join(f"{100 * b[k] / total:.1f}%" for k in names) + " |")


def games(records: Path, split: Path) -> None:
    usable = {r["game_num"] for r in csv.DictReader(
        (line for line in split.open(encoding="utf-8") if not line.startswith("#")), delimiter="\t")
        if r["usable"] == "usable"}
    rows = [r for r in csv.DictReader(records.open(encoding="utf-8"), delimiter="\t")
            if r["game_num"] in usable]
    n = len(rows)
    tl = Counter()
    for r in rows:
        t = int(r["timelines"])
        tl["1" if t == 1 else "2" if t == 2 else "3-4" if t <= 4 else "5-8" if t <= 8 else "9+"] += 1
    turns = [int(r["actions"]) for r in rows]
    boards = sorted(int(r["boards"]) for r in rows)
    print(f"\n## How the {n} usable human games end\n")
    print("| Timelines at the end | " + " | ".join(("1", "2", "3-4", "5-8", "9+")) + " |")
    print("| --- |" + " ---: |" * 5)
    print("| Games | " + " | ".join(f"{tl[k]} ({100 * tl[k] / n:.1f}%)" for k in ("1", "2", "3-4", "5-8", "9+")) + " |")
    print(f"\nTurns: {sum(turns):,} in all; mean {sum(turns) / n:.1f}, median {statistics.median(turns):g}, "
          f"longest {max(turns)}.")
    print(f"Boards at the end: median {statistics.median(boards):g}, 95th percentile "
          f"{boards[int(0.95 * n)]}, most {boards[-1]}.")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("bench", "hist", "probe", "records", "split"):
        ap.add_argument(f"--{name}", required=True)
    a = ap.parse_args()
    hist = json.load(open(a.hist, encoding="utf-8"))
    turn_width(json.load(open(a.bench, encoding="utf-8")))
    relations(hist)
    live_rows(json.load(open(a.probe, encoding="utf-8")))
    delta_t(hist)
    games(Path(a.records), Path(a.split))
    return 0


if __name__ == "__main__":
    sys.exit(main())

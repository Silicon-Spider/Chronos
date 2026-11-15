#!/usr/bin/env python3
"""G0's relation analysis: displacement-class edges, delta-t buckets and live feature count.

Reads what fractal-relhist writes:
  --hist FILE   the histograms (--out) over the start games of the ND8 split
  --probe FILE  the engine-derived reachability table (--probe)
and prints Markdown tables for docs/gates/G0.md (and --json FILE with every number).

Definitions (docs/gates/G0.md explains the choices):
- A relation's class comes from (dL, dTt): dL the victim's timeline minus the attacker's,
  dTt the whole turns along the move. Same board: dL = 0 and dTt = 0. Timeline only: dL != 0,
  dTt = 0. Time only: dL = 0, dTt != 0. Both: dL != 0 and dTt != 0. The half-turn distance
  between the real boards (dTh) is reported beside it: for the side that moves next on the
  attacker's board it is 2 x dTt, for the other side it is 2 x dTt - 1 onto history boards,
  so a half-turn edge would sort the two colours into different classes.
- Edges: within each family the integer distances are split into contiguous buckets (3 for
  timeline only by |dL|, 3 for time only by |dTt|, 2 for both by max(|dL|, |dTt|)), choosing
  the split that maximises the entropy of the bucket counts (the most even split the
  histogram allows). Delta t to the present: 6 contiguous buckets over the signed half-turn
  values of every head board, with every negative value (an inactive head behind the present)
  kept in one bucket of its own, and the other five chosen the same way.
- Feature rows: attacker kind x attacker colour x victim kind x victim colour x location x
  class, location a square (64) on the same board and a 2x2 zone (16) across boards,
  110,592 rows. A row is impossible when the engine probe never reaches it: the attacker
  kind and colour can't capture that square on its own board, or can't reach that class and
  zone across boards; or the victim is a pawn or brawn on its promotion rank, where it can
  never stand. Symmetric same-kind pairs once: on the same board an enemy pair of one kind
  attacks each other (every same-board capture geometry is mutual between the two colours),
  so the rows with a black attacker of kind K on a white victim of kind K are dropped and
  the white-attacker rows stand for both. Own pairs of one kind are mutual too, but the index
  has no attacker square to choose one of the two by, so they stay.
"""
from __future__ import annotations

import argparse
import itertools
import json
import math
import sys
from collections import Counter, defaultdict

KINDS = ["Pawn", "Knight", "Bishop", "Rook", "Queen", "King", "Unicorn", "Dragon", "Princess",
         "Brawn", "CommonKing", "RoyalQueen"]
GEN1 = {"Pawn", "Knight", "Bishop", "Rook", "Queen", "King"}
COLOURS = ["w", "b"]
PAWNLIKE = {"Pawn", "Brawn"}


def family(dl: int, dtt: int) -> str:
    if dl == 0 and dtt == 0:
        return "same"
    if dtt == 0:
        return "timeline"
    if dl == 0:
        return "time"
    return "both"


def distance(fam: str, dl: int, dtt: int) -> int:
    return {"same": 0, "timeline": abs(dl), "time": abs(dtt), "both": max(abs(dl), abs(dtt))}[fam]


def entropy(counts: list[int]) -> float:
    n = sum(counts)
    return -sum(c / n * math.log(c / n) for c in counts if c) if n else 0.0


def best_split(hist: dict[int, int], buckets: int) -> list[int]:
    """Lower edges of `buckets` contiguous buckets over the positive integers (the first starts
    at the smallest value, the last is open-ended), maximising the entropy of their counts."""
    values = sorted(v for v in hist if hist[v] > 0)
    best, best_h = None, -1.0
    for cuts in itertools.combinations(values[1:], buckets - 1):
        edges = [values[0], *cuts]
        counts = bucket_counts(hist, edges)
        h = entropy(counts)
        if h > best_h + 1e-12:
            best, best_h = edges, h
    return best


def bucket_counts(hist: dict[int, int], edges: list[int]) -> list[int]:
    out = [0] * len(edges)
    for v, c in hist.items():
        i = max(j for j, e in enumerate(edges) if v >= e) if v >= edges[0] else 0
        out[i] += c
    return out


def label(edges: list[int], i: int, unit: str = "") -> str:
    lo = edges[i]
    if i == len(edges) - 1:
        return f"{lo}+{unit}"
    hi = edges[i + 1] - 1
    return f"{lo}{unit}" if hi == lo else f"{lo}-{hi}{unit}"


def class_of(fam: str, d: int, edges: dict[str, list[int]]) -> str:
    if fam == "same":
        return "same board"
    e = edges[fam]
    i = max(j for j, lo in enumerate(e) if d >= lo) if d >= e[0] else 0
    return f"{fam} {label(e, i)}"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--hist", required=True)
    ap.add_argument("--probe", required=True)
    ap.add_argument("--json")
    a = ap.parse_args()
    H = json.load(open(a.hist, encoding="utf-8"))
    P = json.load(open(a.probe, encoding="utf-8"))
    F = H["relations"]["fields"]
    ix = {f: i for i, f in enumerate(F)}
    rows = H["relations"]["rows"]
    out: dict = {"source": {"hist": a.hist, "probe": a.probe, "meta": H["meta"].get("records")}}

    # ---- the histograms ------------------------------------------------------------
    fam_hist = {f: Counter() for f in ("timeline", "time", "both")}
    fam_hist_half = {f: Counter() for f in ("timeline", "time", "both")}
    totals = Counter()
    by_side = Counter()
    for r in rows:
        dl, dth, dtt, n = r[ix["dL"]], r[ix["dTh"]], r[ix["dTt"]], r[ix["count"]]
        fam = family(dl, dtt)
        totals[fam] += n
        by_side[(fam, r[ix["passed"]], r[ix["own"]])] += n
        if fam != "same":
            fam_hist[fam][distance(fam, dl, dtt)] += n
        # The spec's literal reading: family and distance both from (dL, dTh), half-turns
        # between the real boards.
        fam_h = family(dl, dth)
        if fam_h != "same":
            fam_hist_half[fam_h][distance(fam_h, dl, dth)] += n
    all_rel = sum(totals.values())
    print(f"## Relations over the start games\n")
    print(f"{all_rel:,} relations: " + ", ".join(f"{k} {v:,} ({100 * v / all_rel:.1f}%)" for k, v in totals.items()))
    print()
    for fam in ("timeline", "time", "both"):
        h = fam_hist[fam]
        print(f"{fam}: " + ", ".join(f"{d}: {c:,}" for d, c in sorted(h.items())))
        hh = fam_hist_half[fam]
        print(f"{fam} (half-turns): " + ", ".join(f"{d}: {c:,}" for d, c in sorted(hh.items())))
    out["histograms"] = {f: dict(sorted(fam_hist[f].items())) for f in fam_hist}
    out["histograms_half_turns"] = {f: dict(sorted(fam_hist_half[f].items())) for f in fam_hist_half}
    out["family_totals"] = dict(totals)

    # ---- the edges -------------------------------------------------------------------
    nb = {"timeline": 3, "time": 3, "both": 2}
    edges = {f: best_split(fam_hist[f], nb[f]) for f in nb}
    spec_edges = {"timeline": [1, 2, 3], "time": [1, 2, 3], "both": [1, 2]}
    print("\n## Displacement classes\n")
    print("| Family | Unit | Edges (G0) | Counts | Share within family | Spec's edges | Counts with the spec's edges |")
    print("| --- | --- | --- | --- | --- | --- | --- |")
    for f in nb:
        unit = {"timeline": "timelines", "time": "turns", "both": "max(timelines, turns)"}[f]
        c = bucket_counts(fam_hist[f], edges[f])
        cs = bucket_counts(fam_hist[f], spec_edges[f])
        tot = sum(c)
        print(f"| {f} | {unit} | {', '.join(label(edges[f], i) for i in range(nb[f]))} | "
              f"{', '.join(f'{x:,}' for x in c)} | {', '.join(f'{100 * x / tot:.1f}%' for x in c)} | "
              f"{', '.join(label(spec_edges[f], i) for i in range(nb[f]))} | {', '.join(f'{x:,}' for x in cs)} |")
    # the spec's half-turn reading, for the record
    half_spec = {f: bucket_counts(fam_hist_half[f], spec_edges[f]) for f in nb}
    half_edges = {f: best_split(fam_hist_half[f], nb[f]) for f in nb}
    moved = sum(fam_hist["timeline"].values()) - sum(fam_hist_half["timeline"].values())
    print(f"\nThe spec's literal reading (family and distance from half-turns between the real boards): "
          f"its edges give timeline {half_spec['timeline']}, time {half_spec['time']}, both {half_spec['both']}; "
          f"the same rule picks {half_edges}; {moved:,} relations change family from timeline only to both.")
    out["half_turn_reading"] = {"spec_edge_counts": half_spec, "best_edges": half_edges,
                                "timeline_to_both": moved,
                                "family_totals": {f: sum(fam_hist_half[f].values()) for f in nb}}
    out["edges"] = edges
    out["edge_counts"] = {f: bucket_counts(fam_hist[f], edges[f]) for f in nb}
    out["spec_edge_counts_turns"] = {f: bucket_counts(fam_hist[f], spec_edges[f]) for f in nb}

    classes = ["same board"] + [f"{f} {label(edges[f], i)}" for f in nb for i in range(nb[f])]
    out["classes"] = classes
    print("\nClasses: " + "; ".join(classes))

    # ---- delta t to the present ---------------------------------------------------------
    PF = H["placement"]["fields"]
    pix = {f: i for i, f in enumerate(PF)}
    dt_hist = Counter()
    dt_role = defaultdict(Counter)
    for r in H["placement"]["rows"]:
        dt_hist[r[pix["dt"]]] += r[pix["count"]]
        dt_role[r[pix["role"]]][r[pix["dt"]]] += r[pix["count"]]
    neg = sum(c for v, c in dt_hist.items() if v < 0)
    nonneg = {v: c for v, c in dt_hist.items() if v >= 0}
    # five buckets over 0, 1, 2, ... (shift by one so best_split's positive values work)
    shifted = {v + 1: c for v, c in nonneg.items()}
    e5 = [e - 1 for e in best_split(shifted, 5)]
    dt_edges = ["<0"] + [label(e5, i) for i in range(5)]
    dt_counts = [neg] + bucket_counts(nonneg, e5)
    heads = sum(dt_counts)
    print("\n## Delta t to the present (head boards, half-turns)\n")
    print("Histogram: " + ", ".join(f"{v}: {c:,}" for v, c in sorted(dt_hist.items()) if -6 <= v <= 16)
          + f" (and {sum(c for v, c in dt_hist.items() if v < -6):,} below -6, "
          f"{sum(c for v, c in dt_hist.items() if v > 16):,} above 16)")
    for role, h in sorted(dt_role.items()):
        print(f"  {role}: " + ", ".join(f"{v}: {c:,}" for v, c in sorted(h.items()) if -4 <= v <= 10))
    print("\n| Bucket | " + " | ".join(dt_edges) + " |")
    print("| --- |" + " --- |" * 6)
    print("| Head boards | " + " | ".join(f"{c:,}" for c in dt_counts) + " |")
    print("| Share | " + " | ".join(f"{100 * c / heads:.1f}%" for c in dt_counts) + " |")
    out["dt_histogram"] = dict(sorted(dt_hist.items()))
    out["dt_buckets"] = dt_edges
    out["dt_bucket_counts"] = dt_counts

    # ---- reachability from the probe ------------------------------------------------------
    same_reach = {}
    across_reach = defaultdict(int)  # (kind, colour, class) -> zone mask
    bounds = P["meta"]["probe_bounds"]
    for k in P["kinds"]:
        key = (k["kind"], k["colour"])
        same_reach[key] = int(k["same_board"]["union"], 16)
        for pair in k["across"]["pairs"]:
            dl, dtt = pair["dL"], pair["dTt"]
            fam = family(dl, dtt)
            cl = class_of(fam, distance(fam, dl, dtt), edges)
            z = pair["zones"]
            across_reach[(key[0], key[1], cl)] |= int(z, 16) if isinstance(z, str) else int(z)
    # every open-ended class must start inside the probe's bounds
    for f in nb:
        top = edges[f][-1]
        lim = {"timeline": bounds["dL_max"], "time": -bounds["dTt_min"],
               "both": min(bounds["dL_max"], bounds["dTt_max"])}[f]
        assert top <= lim, f"the probe's bounds don't reach {f} {top}+"

    def promotion_rank(colour: str) -> int:
        return 7 if colour == "w" else 0

    full = impossible = symmetric = 0
    live_rows = set()
    dead_reason = Counter()
    gen1_full = gen1_live = 0
    cross_classes = classes[1:]
    for ak, ac, vk, vc in itertools.product(KINDS, COLOURS, KINDS, COLOURS):
        g1 = ak in GEN1 and vk in GEN1
        for sq in range(64):
            full += 1
            gen1_full += g1
            row = (ak, ac, vk, vc, "same board", sq)
            if not (same_reach[(ak, ac)] >> sq) & 1:
                impossible += 1; dead_reason["attacker can't capture that square on its own board"] += 1; continue
            if vk in PAWNLIKE and sq // 8 == promotion_rank(vc):
                impossible += 1; dead_reason["victim pawn or brawn on its promotion rank"] += 1; continue
            if ak == vk and ac == "b" and vc == "w":
                symmetric += 1; dead_reason["symmetric same-kind enemy pair, counted once"] += 1; continue
            live_rows.add(row)
            gen1_live += g1
        for cl in cross_classes:
            mask = across_reach.get((ak, ac, cl), 0)
            for z in range(16):
                full += 1
                gen1_full += g1
                if not (mask >> z) & 1:
                    impossible += 1
                    dead_reason["attacker can't reach that class and zone" if mask else
                                "attacker can't reach that class at all"] += 1
                    continue
                live_rows.add((ak, ac, vk, vc, cl, z))
                gen1_live += g1
    assert full == 110592, full
    live = len(live_rows)
    # rows that fired in the start games
    seen = set()
    seen_rel = Counter()
    for r in rows:
        dl, dtt = r[ix["dL"]], r[ix["dTt"]]
        fam = family(dl, dtt)
        cl = class_of(fam, distance(fam, dl, dtt), edges)
        row = (r[ix["akind"]], r[ix["acol"]], r[ix["vkind"]], r[ix["vcol"]], cl, r[ix["loc"]])
        seen.add(row)
        seen_rel[row in live_rows] += r[ix["count"]]
    seen_dead = [s for s in seen if s not in live_rows]
    # A row dropped as one half of a symmetric pair is expected to fire (its mirror stands
    # for it); a row dropped as impossible must never fire, or the probe is wrong.
    seen_symmetric = [s for s in seen_dead if s[4] == "same board" and s[0] == s[2] and s[1] == "b" and s[3] == "w"]
    seen_impossible = [s for s in seen_dead if s not in seen_symmetric]
    print("\n## Feature rows\n")
    print(f"| Count | Rows |\n| --- | ---: |")
    print(f"| Full index | {full:,} |")
    print(f"| Impossible (engine probe, promotion ranks) | {impossible:,} |")
    print(f"| Symmetric same-kind enemy pairs, counted once | {symmetric:,} |")
    print(f"| **Live after trimming** | **{live:,}** |")
    print(f"| Live, gen-1 pieces only (P N B R Q K; of {gen1_full:,}) | {gen1_live:,} |")
    print(f"| Live rows that fire at least once in the start games | {len(seen & live_rows):,} |")
    print(f"| Index rows that fire, counting the dropped mirrored halves | {len(seen):,} |")
    for why, n in dead_reason.most_common():
        print(f"- {why}: {n:,}")
    print(f"\nRelations in the start games on a dropped row: {seen_rel[False]:,} of {sum(seen_rel.values()):,}, "
          f"on {len(seen_symmetric)} rows dropped as the mirrored half of a symmetric pair (expected) "
          f"and {len(seen_impossible)} rows dropped as impossible (must be 0)")
    for row in sorted(seen_impossible)[:20]:
        print("  IMPOSSIBLE BUT SEEN:", row)
    assert not seen_impossible, "a row the probe calls impossible fired in the data"
    out["rows"] = {"full": full, "impossible": impossible, "symmetric_once": symmetric, "live": live,
                   "gen1_full": gen1_full, "gen1_live": gen1_live, "fired_in_start_games": len(seen), "live_fired_in_start_games": len(seen & live_rows),
                   "dead_reasons": dict(dead_reason), "relations_on_dropped_rows": seen_rel[False],
                   "seen_symmetric_rows": len(seen_symmetric), "seen_impossible_rows": len(seen_impossible)}
    # reachability summary per kind
    print("\n## Reachability by kind (engine probe), classes reached across boards\n")
    for k in KINDS:
        cls = [cl for cl in cross_classes if across_reach.get((k, "w", cl), 0)]
        sb = bin(same_reach[(k, "w")]).count("1")
        print(f"- {k}: same board {sb} victim squares; across: {', '.join(cls) if cls else 'none'}")
    if a.json:
        json.dump(out, open(a.json, "w", encoding="utf-8"), indent=1, default=str)
    return 0


if __name__ == "__main__":
    sys.exit(main())

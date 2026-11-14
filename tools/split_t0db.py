#!/usr/bin/env python3
"""Split t0database by game into start games and held-out games (ND8), with a fixed seed.

A "game" is a cluster of rows that record the same game, so that no game sits on both
sides of the split:
- rows whose move text is identical once clock comments ({m:ss}) and whitespace are
  removed;
- complete records whose sequences of turn-start positions are equal, or one a prefix
  of the other (a game recorded twice, or cut short), read from the records file that
  chronos-relhist writes (--records-out).
Clusters are ordered by SHA-256 of "<seed>:<smallest GameNum>"; they fill the start set
in that order until it holds two thirds of the games, and the rest are held out.

Usage:
  python tools/split_t0db.py --csv data/t0database/Data.csv --records build/relhist/records.tsv
                             --out data/splits/t0database-nd8.tsv [--seed 20261108]
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import io
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

PINNED_SHA256 = "37fb7c1ce46efb27d42a8b9d0b230df13faff1b36bef24af8c15ba5c8a6c7a4e"
CLOCK = re.compile(r"\{[^}]*\}")


def normalised(text: str) -> str:
    return " ".join(CLOCK.sub(" ", text).split())


class Clusters:
    def __init__(self, items):
        self.parent = {i: i for i in items}

    def find(self, i):
        while self.parent[i] != i:
            self.parent[i] = self.parent[self.parent[i]]
            i = self.parent[i]
        return i

    def union(self, a, b):
        ra, rb = self.find(a), self.find(b)
        if ra != rb:
            self.parent[max(ra, rb)] = min(ra, rb)


def read_records(path: Path) -> dict[int, dict]:
    out = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        cols = line.split("\t")
        if not cols[0].isdigit():
            continue  # a header row
        out[int(cols[0])] = {"status": cols[1], "keys": [k for k in cols[-1].split(",") if k]}
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--csv", required=True)
    ap.add_argument("--records", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--seed", type=int, default=20261108)
    a = ap.parse_args()

    raw = Path(a.csv).read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != PINNED_SHA256:
        raise SystemExit(f"Data.csv has SHA-256 {digest}, not the pinned {PINNED_SHA256}")
    rows = list(csv.DictReader(io.StringIO(raw.decode("utf-8").replace("\r\n", "\n").replace("\r", "\n"))))
    games = {int(r["GameNum"]): r for r in rows if (r.get("Game") or "").strip()}
    records = read_records(Path(a.records))
    missing = sorted(set(games) - set(records))
    if missing:
        raise SystemExit(f"the records file has no line for games {missing[:10]}...")

    cl = Clusters(sorted(games))
    by_text = defaultdict(list)
    for g, r in games.items():
        by_text[normalised(r["Game"])].append(g)
    text_pairs = 0
    for members in by_text.values():
        for g in members[1:]:
            cl.union(members[0], g)
            text_pairs += 1
    complete = {g: tuple(records[g]["keys"]) for g in games if records[g]["status"] == "complete"}
    # Equal sequences are one game.
    by_keys = defaultdict(list)
    for g, s in complete.items():
        by_keys[s].append(g)
    equal_links = 0
    for gs in by_keys.values():
        for g in gs[1:]:
            cl.union(gs[0], g)
            equal_links += 1
    # A proper prefix (a game cut short) joins the game it is a prefix of, but only
    # when that game is unambiguous: a short record that opens several different
    # games is a shared opening, and linking it would chain those games together.
    seqs = sorted(by_keys, key=len)
    prefix_links, ambiguous = 0, 0
    for i, s in enumerate(seqs):
        longer = [t for t in seqs[i + 1:] if len(t) > len(s) and t[: len(s)] == s]
        if not longer:
            continue
        targets = {cl.find(by_keys[t][0]) for t in longer}
        if len(targets) == 1:
            cl.union(by_keys[s][0], by_keys[longer[0]][0])
            prefix_links += 1
        else:
            ambiguous += 1

    members = defaultdict(list)
    for g in sorted(games):
        members[cl.find(g)].append(g)
    keyed = sorted(members.values(), key=lambda m: hashlib.sha256(f"{a.seed}:{m[0]}".encode()).hexdigest())
    target = round(len(games) * 2 / 3)
    side = {}
    n_start = 0
    for m in keyed:
        s = "start" if n_start < target else "heldout"
        if s == "start":
            n_start += len(m)
        for g in m:
            side[g] = s

    # Usable records, by Temporal-5D's strict rule (M7): longest first, a complete record
    # that equals or is a prefix of a record already kept is a duplicate.
    usable = {g: "refused" for g in games}
    kept: list[tuple] = []
    for g in sorted(complete, key=lambda g: (-len(complete[g]), g)):
        s = complete[g]
        usable[g] = "duplicate" if any(t[: len(s)] == s for t in kept) else "usable"
        if usable[g] == "usable":
            kept.append(s)

    lines = [
        "# t0database split by game (spec ND8): start games and held-out games.",
        f"# Written by tools/split_t0db.py, seed {a.seed}, from data/t0database/Data.csv (SHA-256 {PINNED_SHA256})",
        "# and the parse-mode replay of every record by chronos-relhist.",
        "# cluster: the smallest GameNum of the rows that record the same game (identical move text",
        "# without clocks, or equal or prefix sequences of turn-start positions); a cluster is never split.",
        "# usable: usable = a complete record that neither equals nor is a prefix of a longer one",
        "# (Temporal-5D's rule for its 692 games); duplicate = complete, but equal to or a prefix of",
        "# a record kept before it, longest first; refused = does not replay to the end in parse mode.",
        "game_num\tset\tcluster\tusable",
    ]
    for g in sorted(games):
        lines.append(f"{g}\t{side[g]}\t{cl.find(g)}\t{usable[g]}")
    out = Path(a.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")

    count = Counter((side[g], usable[g]) for g in games)
    clusters_multi = [m for m in members.values() if len(m) > 1]
    print(f"games {len(games)}; clusters {len(members)} ({len(clusters_multi)} with more than one row; "
          f"{text_pairs} text-identical links, {equal_links} equal-position links, {prefix_links} prefix links; "
          f"{ambiguous} short records open several games and stay apart)")
    for s in ("start", "heldout"):
        n = sum(1 for g in games if side[g] == s)
        print(f"{s:8} games {n:4}  usable {count[(s, 'usable')]:4}  duplicate {count[(s, 'duplicate')]:3}  "
              f"refused {count[(s, 'refused')]:3}")
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

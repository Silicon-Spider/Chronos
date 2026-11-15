#!/usr/bin/env python3
"""Consistency checks between docs/SPEC.md and docs/DECISIONS.md.

- Counts the register's rows by section and status, and compares them with the
  counts the register states in its own prose (the header line and each section's
  first line).
- Lists every bracketed or bare ID the spec cites that has no row in the register,
  and every register row the spec never cites.
- Lists Spec-column section references (such as 7.3) that name no heading in the spec.
- Checks that every relative link and image in the docs resolves.

It needs nothing but the repository.
Usage: python tools/check_docs.py [--root DIR]
Exit status 0 when nothing is wrong, 1 otherwise.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ID = re.compile(r"\b(NS|NR|NB|NT|NA|NI|NN|ND|NL|NE|NM|NX|NF|NQ|P)(\d+)\b")
RANGE = re.compile(r"\b(NS|NR|NB|NT|NA|NI|NN|ND|NL|NE|NM|NX|NF|NQ|P)(\d+)\s*[–-]\s*(?:(NS|NR|NB|NT|NA|NI|NN|ND|NL|NE|NM|NX|NF|NQ|P))?(\d+)\b")
WORDS = {"one": 1, "two": 2, "three": 3, "four": 4, "five": 5, "six": 6, "seven": 7, "eight": 8,
         "nine": 9, "ten": 10, "eleven": 11, "twelve": 12, "thirteen": 13, "fourteen": 14,
         "fifteen": 15, "sixteen": 16, "seventeen": 17, "eighteen": 18, "nineteen": 19,
         "twenty": 20, "thirty": 30, "forty": 40, "fifty": 50}


def number(text: str) -> int | None:
    text = text.strip().lower().replace(",", "")
    if text.isdigit():
        return int(text)
    total = 0
    for part in re.split(r"[-\s]+", text):
        if part not in WORDS:
            return None
        total += WORDS[part]
    return total


def ids_in(text: str) -> set[str]:
    found = {m.group(1) + m.group(2) for m in ID.finditer(text)}
    for m in RANGE.finditer(text):
        prefix, lo, prefix2, hi = m.group(1), int(m.group(2)), m.group(3), int(m.group(4))
        if prefix2 and prefix2 != prefix:
            continue
        if lo < hi <= lo + 40:
            found |= {f"{prefix}{k}" for k in range(lo, hi + 1)}
    return found


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--root", default=str(Path(__file__).resolve().parent.parent))
    repo = Path(ap.parse_args().root)
    spec = (repo / "docs/SPEC.md").read_text(encoding="utf-8")
    reg = (repo / "docs/DECISIONS.md").read_text(encoding="utf-8")
    problems: list[str] = []

    # ---- the register's rows, by section ---------------------------------------
    sections: list[tuple[str, list[str]]] = []
    for line in reg.splitlines():
        if line.startswith("## "):
            sections.append((line[3:].strip(), []))
        elif sections:
            sections[-1][1].append(line)
    rows: dict[str, dict] = {}
    decisions = committed = provisional = 0
    counts = {}
    for title, lines in sections:
        sec_rows = []
        for line in lines:
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            if len(cells) >= 3 and re.fullmatch(r"(NS|NR|NB|NT|NA|NI|NN|ND|NL|NE|NM|NX|NF|NQ|P)\d+", cells[0]):
                if cells[0] in rows:
                    problems.append(f"register: {cells[0]} has two rows")
                rows[cells[0]] = {"section": title, "cells": cells}
                sec_rows.append(cells)
        n_c = sum(1 for c in sec_rows if len(c) > 2 and c[2] == "Committed")
        n_p = sum(1 for c in sec_rows if len(c) > 2 and c[2] == "Provisional")
        counts[title] = (len(sec_rows), n_c, n_p)
        if title not in ("Arms", "Refusals", "Predictions") and sec_rows:
            decisions += len(sec_rows)
            committed += n_c
            provisional += n_p
            if n_c + n_p != len(sec_rows):
                problems.append(f"register: section '{title}' has rows that are neither Committed nor Provisional")
            first = next((ln for ln in lines if ln.strip()), "")
            m = re.match(r"([A-Za-z-]+) rows?: (\d+) committed and (\d+) provisional", first)
            if not m:
                problems.append(f"register: section '{title}' has no 'N rows: C committed and P provisional' line")
            else:
                stated = (number(m.group(1)), int(m.group(2)), int(m.group(3)))
                if stated != (len(sec_rows), n_c, n_p):
                    problems.append(f"register: section '{title}' says {stated}, has {(len(sec_rows), n_c, n_p)}")
        print(f"{title:42s} rows {len(sec_rows):3d}  committed {n_c:3d}  provisional {n_p:3d}")
    arms, refusals, predictions = counts.get("Arms", (0,))[0], counts.get("Refusals", (0,))[0], counts.get("Predictions", (0,))[0]
    print(f"decisions {decisions} (committed {committed}, provisional {provisional}); arms {arms}; refusals {refusals}; predictions {predictions}")
    head = re.search(r"(\d+) decisions \((\d+) committed, (\d+) provisional\), plus (\d+) arms, (\d+) refusals and (\d+) predictions", reg)
    if not head:
        problems.append("register: no header count line")
    else:
        stated = tuple(int(g) for g in head.groups())
        actual = (decisions, committed, provisional, arms, refusals, predictions)
        if stated != actual:
            problems.append(f"register: header says {stated}, rows give {actual}")
    for title, word in (("Arms", "arms"), ("Refusals", "refusals"), ("Predictions", "predictions")):
        lines = dict(sections).get(title, [])
        first = next((ln for ln in lines if ln.strip()), "")
        m = re.match(r"([A-Za-z-]+) " + word, first)
        if m and number(m.group(1)) != counts[title][0]:
            problems.append(f"register: section '{title}' says {m.group(1)}, has {counts[title][0]}")

    # ---- IDs cited in the spec against the register ------------------------------
    cited = ids_in(spec)
    for missing in sorted(cited - set(rows), key=lambda s: (re.sub(r"\d", "", s), int(re.sub(r"\D", "", s)))):
        problems.append(f"spec cites {missing}, which has no register row")
    for unused in sorted(set(rows) - cited, key=lambda s: (re.sub(r"\d", "", s), int(re.sub(r"\D", "", s)))):
        problems.append(f"register row {unused} is never cited in the spec")
    reg_cited = ids_in(re.sub(r"^\|\s*(?:NS|NR|NB|NT|NA|NI|NN|ND|NL|NE|NM|NX|NF|NQ|P)\d+\s*\|", "|", reg, flags=re.M))
    for missing in sorted(reg_cited - set(rows)):
        problems.append(f"register cites {missing}, which has no row")

    # ---- section references --------------------------------------------------------
    headings = set()
    for line in spec.splitlines():
        m = re.match(r"#{2,4} (\d+(?:\.\d+)*)\.? ", line)
        if m:
            headings.add(m.group(1))
    for doc, text in (("spec", spec), ("register", reg)):
        for m in re.finditer(r"§(\d+(?:\.\d+)*)", text):
            if m.group(1) not in headings:
                problems.append(f"{doc} refers to §{m.group(1)}, which is not a heading of the spec")

    # ---- links and images ----------------------------------------------------------
    for name in ("docs/SPEC.md", "docs/DECISIONS.md", "docs/BUILD_PROMPT.md", "README.md", "docs/gates/G0.md",
                 "docs/gates/G0-rulings.md", "docs/MEASUREMENTS.md"):
        path = repo / name
        if not path.is_file():
            continue
        for m in re.finditer(r"\]\(([^)#\s]+)(?:#[^)]*)?\)", path.read_text(encoding="utf-8")):
            target = m.group(1)
            if re.match(r"[a-z]+://", target) or target.startswith("mailto:"):
                continue
            if not (path.parent / target).exists():
                problems.append(f"{name}: link target {target} does not exist")

    print()
    for p in problems:
        print("PROBLEM ", p)
    print(f"{len(problems)} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

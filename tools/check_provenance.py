#!/usr/bin/env python3
"""Check every file Chronos copied from Temporal-5D against PROVENANCE.md.

For each row of the "Copied files" table:
- a "record only" file must hash to its source SHA-256;
- a "header" file must start with its provenance header (after a shebang, where
  there is one), and the bytes below the header must hash to its source SHA-256,
  unless the file is listed under "Changes since the copy", whose table gives the
  SHA-256 the bytes below the header have now.

It needs nothing but the repository: no clone of Temporal-5D (NS2).
Usage: python tools/check_provenance.py [--root DIR]
Exit status 0 when every file checks, 1 otherwise.
"""
from __future__ import annotations

import argparse
import hashlib
import re
import sys
from pathlib import Path

PIN = "c8a9c490bfde650466609531f8083d69513c6036"
ROW = re.compile(r"^\| `([^`]+)` \| (header|record only) \| (\d+) \| `([0-9a-f]{64})` \|")
CHANGED_ROW = re.compile(r"^\| `([^`]+)` \| [^|]+ \| `([0-9a-f]{64})` \|")


def sections(text: str) -> dict[str, list[str]]:
    out: dict[str, list[str]] = {}
    current = None
    for line in text.splitlines():
        if line.startswith("## "):
            current = line[3:].strip()
            out[current] = []
        elif current is not None:
            out[current].append(line)
    return out


def body_below_header(path: str, data: bytes) -> tuple[bytes | None, str]:
    """The bytes below the provenance header, or None and why not."""
    lines = data.split(b"\n")
    if path.endswith(".md"):
        if len(lines) < 8 or lines[0] != b"<!--" or lines[6] != b"-->":
            return None, "no Markdown provenance header"
        header, body = lines[1:6], lines[7:]
        keep: list[bytes] = []
        prefix = b""
    else:
        k = 1 if lines and lines[0].startswith(b"#!") else 0
        if len(lines) < k + 5:
            return None, "too short for a provenance header"
        header, body, keep = lines[k:k + 5], lines[k + 5:], lines[:k]
        prefix = b"// " if path.endswith((".cpp", ".hpp", ".h")) else b"# "
        if not all(h.startswith(prefix) for h in header):
            return None, "no provenance header"
        header = [h[len(prefix):] for h in header]
    expect = [b"Chronos provenance: copied from Temporal-5D",
              b"  source: " + path.encode(), b"  commit: " + PIN.encode(), b"  copied: ", b"  state:  "]
    for got, want in zip(header, expect):
        if not got.startswith(want):
            return None, f"header line {got!r} does not start {want!r}"
    return b"\n".join(keep + body), ""


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--root", default=str(Path(__file__).resolve().parent.parent))
    a = ap.parse_args()
    root = Path(a.root)
    sec = sections((root / "PROVENANCE.md").read_text(encoding="utf-8"))
    rows = [m.groups() for line in sec.get("Copied files", []) if (m := ROW.match(line))]
    changed = {m.group(1): m.group(2) for line in sec.get("Changes since the copy", [])
               if (m := CHANGED_ROW.match(line))}
    failures = 0
    n_unchanged = n_changed = 0
    for path, mode, size, digest in rows:
        f = root / path
        if not f.is_file():
            print(f"MISSING  {path}")
            failures += 1
            continue
        data = f.read_bytes()
        if mode == "record only":
            ok = len(data) == int(size) and hashlib.sha256(data).hexdigest() == digest
            why = "bytes differ from the source"
        else:
            body, why = body_below_header(path, data)
            want = changed.get(path, digest)
            ok = body is not None and hashlib.sha256(body).hexdigest() == want
            if body is not None and not ok:
                why = "bytes below the header differ from " + ("the recorded change" if path in changed else "the source")
        if not ok:
            print(f"FAIL     {path}: {why}")
            failures += 1
        elif path in changed:
            n_changed += 1
        else:
            n_unchanged += 1
    for path in changed:
        if path not in {r[0] for r in rows}:
            print(f"FAIL     {path}: listed as changed but not in the table of copied files")
            failures += 1
    print(f"provenance: {len(rows)} copied files; {n_unchanged} unchanged, {n_changed} changed as recorded, "
          f"{failures} failing")
    return 1 if failures or not rows else 0


if __name__ == "__main__":
    sys.exit(main())

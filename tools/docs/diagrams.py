#!/usr/bin/env python3
"""Draws the spec's three diagrams as SVG, from the standard library alone.

docs/img/architecture.svg, network.svg and roadmap.svg are this script's output, and the
PNGs beside them, which docs/SPEC.md shows, are renders of those SVGs. When the design
changes, change the text here, run the script, and render each SVG again with a headless
browser at the size in its first line, for example:

  msedge --headless=new --hide-scrollbars --force-device-scale-factor=1 \
         --window-size=1344,1258 --screenshot=docs/img/network.png docs/img/network.svg

Usage: python tools/docs/diagrams.py [OUTDIR]     (default: docs/img)
"""
from __future__ import annotations

import sys
from html import escape
from pathlib import Path

INK = "#141413"
GREY = "#5e5d59"
LINE = "#c4c2b8"
BLUE = "#2a78d6"
BLUE_FILL = "#e4eefa"
SHADE = "#ececea"
FONT = "'Segoe UI', 'Helvetica Neue', Helvetica, Arial, sans-serif"


class Diagram:
    def __init__(self, width: int, height: int, title: str) -> None:
        self.w, self.h = width, height
        self.parts: list[str] = []
        self.text(44, 60, title, size=26, weight=600, anchor="start")

    def text(self, x: float, y: float, s: str, size: int = 20, weight: int = 400,
             fill: str = INK, anchor: str = "middle", spacing: float = 0) -> None:
        extra = f' letter-spacing="{spacing}"' if spacing else ""
        self.parts.append(
            f'<text x="{x}" y="{y}" font-size="{size}" font-weight="{weight}" fill="{fill}" '
            f'text-anchor="{anchor}"{extra}>{escape(s)}</text>')

    def label(self, x: float, y: float, s: str) -> None:
        """A small grey section label in capitals."""
        self.text(x, y, s.upper(), size=17, weight=600, fill=GREY, anchor="start", spacing=0.6)

    def note(self, x: float, y: float, s: str, anchor: str = "start") -> None:
        self.text(x, y, s, size=19, fill=GREY, anchor=anchor)

    def shade(self, x: float, y: float, w: float, h: float, caption: str | None = None) -> None:
        self.parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="12" fill="{SHADE}"/>')
        if caption:
            self.note(x + w / 2, y + 34, caption, anchor="middle")

    def box(self, x: float, y: float, w: float, h: float, title: str,
            sub: str | list[str] | None = None, hi: bool = False) -> None:
        stroke, width = (BLUE, 3) if hi else (LINE, 2)
        fill = BLUE_FILL if hi else "#ffffff"
        self.parts.append(
            f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="12" fill="{fill}" '
            f'stroke="{stroke}" stroke-width="{width}"/>')
        lines = [] if sub is None else ([sub] if isinstance(sub, str) else list(sub))
        cx = x + w / 2
        total = 24 + 28 * len(lines)
        top = y + (h - total) / 2 + 20
        self.text(cx, top, title, size=22, weight=600)
        for i, line in enumerate(lines):
            self.text(cx, top + 30 + 28 * i, line, size=19, fill=GREY)

    def diamond(self, cx: float, cy: float, w: float, h: float, s: str, hi: bool = False) -> None:
        stroke, width = (BLUE, 3) if hi else (LINE, 2)
        fill = BLUE_FILL if hi else "#ffffff"
        pts = f"{cx - w / 2},{cy} {cx},{cy - h / 2} {cx + w / 2},{cy} {cx},{cy + h / 2}"
        self.parts.append(f'<polygon points="{pts}" fill="{fill}" stroke="{stroke}" stroke-width="{width}"/>')
        self.text(cx, cy + 8, s, size=22, weight=600)

    def arrow(self, pts: list[tuple[float, float]], dashed: bool = False, colour: str = LINE,
              head: bool = True) -> None:
        d = "M " + " L ".join(f"{x} {y}" for x, y in pts)
        dash = ' stroke-dasharray="9 7"' if dashed else ""
        width = 3 if dashed else 2
        self.parts.append(f'<path d="{d}" fill="none" stroke="{colour}" stroke-width="{width}"{dash}/>')
        if head:
            (x0, y0), (x1, y1) = pts[-2], pts[-1]
            dx, dy = x1 - x0, y1 - y0
            n = (dx * dx + dy * dy) ** 0.5 or 1
            ux, uy = dx / n, dy / n
            px, py = -uy, ux
            a, b = 11, 5.5
            tri = (f"{x1},{y1} {x1 - a * ux + b * px},{y1 - a * uy + b * py} "
                   f"{x1 - a * ux - b * px},{y1 - a * uy - b * py}")
            self.parts.append(f'<polygon points="{tri}" fill="{colour}"/>')

    def svg(self) -> str:
        body = "\n".join(self.parts)
        return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.w}" height="{self.h}" '
                f'viewBox="0 0 {self.w} {self.h}" font-family="{FONT}">\n'
                f'<rect width="{self.w}" height="{self.h}" fill="#ffffff"/>\n{body}\n</svg>\n')


def architecture() -> Diagram:
    d = Diagram(1344, 854, "The net is trained by the same feature code it plays with")
    d.label(44, 110, "Engine · CPU, C++20")
    d.label(778, 110, "Training loop · offline")
    left, width = 44, 634
    d.box(left, 136, width, 96, "5DUCI", "test-match runner · other engines' tools · GUIs")
    d.box(left, 270, width, 96, "Search", "PVS over turns · TT · null move · Lazy SMP")
    d.box(left, 404, 302, 124, "Turn generator", ["hypercuboid search,", "best-first, legal only"])
    d.box(376, 404, 302, 124, "NNUE eval", ["board embeddings,", "relations, roles, head"], hi=True)
    d.box(left, 568, width, 96, "Position layer", "immutable boards · board records · content cache")
    d.box(left, 702, width, 96, "Rules engine · from Temporal-5D, reviewed",
          "move generation · check · relation facts · tests")
    right, rwidth = 778, 522
    d.box(right, 136, rwidth, 96, "Game sources", "self-play · t0database starts")
    d.box(right, 270, rwidth, 96, "Records", "games: start · turns · scores · flags · result")
    d.box(right, 404, rwidth, 96, "Featuriser", "the engine's feature code, via its C API", hi=True)
    d.box(right, 538, rwidth, 96, "Trainer", "PyTorch · simulates the integer path")
    d.box(right, 672, rwidth, 96, "Net file", "quantised · provenance header · NETS.md")
    d.arrow([(361, 232), (361, 270)])
    d.arrow([(195, 366), (195, 404)])
    d.arrow([(527, 366), (527, 404)])
    d.arrow([(195, 528), (195, 568)])
    d.arrow([(527, 528), (527, 568)])
    d.arrow([(361, 664), (361, 702)])
    d.arrow([(678, 184), (778, 184)])
    d.note(728, 170, "games", anchor="middle")
    for y0, y1 in ((232, 270), (366, 404), (500, 538), (634, 672)):
        d.arrow([(1039, y0), (1039, y1)])
    d.arrow([(678, 452), (778, 452)], dashed=True, colour=BLUE, head=False)
    d.arrow([(778, 720), (728, 720), (728, 500), (678, 500)])
    return d


def network() -> Diagram:
    d = Diagram(1344, 1258, "Board state is cached; what crosses boards is rebuilt per evaluation")
    d.shade(58, 100, 416, 488, "once per board, at first use")
    d.note(890, 134, "per evaluation, head boards only", anchor="middle")
    d.box(72, 156, 388, 98, "Pieces", "2,048 features per board")
    d.box(72, 312, 388, 98, "Piece accumulator", "2,048 → 256 per view, 16-bit")
    d.box(72, 468, 388, 98, "Board embedding", "256 → 32 per view, cached", hi=True)
    d.box(488, 156, 388, 98, "Attack relations", "68,304 live rows · both colours")
    d.box(488, 312, 388, 98, "Relation accumulator", ["→ 128 per view, 8-bit rows;", "same-board part cached"])
    d.box(906, 156, 388, 98, "Placement", "18 features per head board")
    d.box(906, 312, 388, 98, "Placement layer", "18 → 16 per view")
    d.box(72, 624, 388, 98, "Past-board vector", "embedding only: 32")
    d.box(488, 624, 806, 98, "Head-board vector",
          "embedding 32 + relations 128 + placement 16 = 176, each after clipped ReLU")
    d.box(72, 780, 1222, 98, "Role pooling, per view",
          "4 head roles × sum and max of 176 · 2 past roles × sum of 32 · 1,472 in all")
    d.box(72, 936, 388, 126, "Material path", ["2,048 → 5, one per stack", "a weight per stack and role"])
    d.box(488, 936, 806, 126, "Head, one of five stacks",
          ["2 views × 1,472 = 2,944 → 32 → 32 → 1", "stack chosen by active timelines: 1, 2, 3–4, 5–8, 9+"])
    d.box(72, 1120, 1222, 84, "Score = head output + material path, from the chosen stack")
    d.arrow([(266, 254), (266, 312)])
    d.arrow([(266, 410), (266, 468)])
    d.arrow([(682, 254), (682, 312)])
    d.arrow([(1100, 254), (1100, 312)])
    d.arrow([(266, 566), (266, 624)])
    d.arrow([(460, 517), (530, 517), (530, 624)])
    d.arrow([(682, 410), (682, 624)])
    d.arrow([(1100, 410), (1100, 624)])
    d.arrow([(266, 722), (266, 780)])
    d.arrow([(890, 722), (890, 780)])
    d.arrow([(890, 878), (890, 936)])
    d.arrow([(72, 361), (36, 361), (36, 999), (72, 999)])
    d.arrow([(266, 1062), (266, 1120)])
    d.arrow([(890, 1062), (890, 1120)])
    return d


def roadmap() -> Diagram:
    d = Diagram(1344, 1386, "Nothing trains until the optimised core passes G1")
    x, w, cx = 170, 530, 435
    phases = [
        ("Phase 0 · Copy and review", "done: rules engine copied, findings written"),
        ("Phase 1 · Optimise and build", "rules engine, then the net, then the search"),
        ("Phase 2 · Gen 1", "bootstrap self-play, first training"),
        ("Phase 3 · Integrate", "gen-1 net in the search, ablations"),
        ("Phase 4 · Generations", "self-play with the latest net"),
        ("Phase 5 · Release", "AVX2, AVX-512, NEON and WASM builds"),
    ]
    gates = [
        ("G0", "merged 15 November 2026 · the gate ruled on in §3.5"),
        ("G1", "three parts: rules engine · net exact on every target · search sound"),
        ("G2", "data floor · parity · family check and screens · offline gate"),
        ("G3", "gen 1 beats bootstrap · imported heuristics confirmed or dropped"),
        ("G4", "each net promoted only by the promotion test"),
    ]
    y = 100
    for i, (title, sub) in enumerate(phases):
        d.box(x, y, w, 98, title, sub)
        if i < len(gates):
            gy = y + 98 + 64
            d.arrow([(cx, y + 98), (cx, gy - 28)])
            d.diamond(cx, gy, 114, 56, gates[i][0], hi=(gates[i][0] == "G1"))
            d.note(520, gy + 7, gates[i][1])
            d.arrow([(cx, gy + 28), (cx, y + 226)])
        y += 226
    # the generations loop: from G4 back to Phase 4
    g4y = 100 + 226 * 4 + 98 + 64
    p4y = 100 + 226 * 4 + 49
    d.arrow([(cx - 57, g4y), (98, g4y), (98, p4y), (x, p4y)])
    d.note(112, g4y - 18, "next generation")
    return d


def main() -> int:
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("docs/img")
    out.mkdir(parents=True, exist_ok=True)
    for name, fn in (("architecture", architecture), ("network", network), ("roadmap", roadmap)):
        d = fn()
        (out / f"{name}.svg").write_bytes(d.svg().encode("utf-8"))
        print(f"{name}.svg {d.w}x{d.h}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

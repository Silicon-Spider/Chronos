# Fractal

A CPU-only engine for 5D Chess With Multiverse Time Travel: alpha-beta search over whole turns, scored by an NNUE that reads the boards themselves. Written in C++20.

Status: Phase 0 is done. The rules engine is copied from Temporal-5D at a pinned commit, builds on Linux and Windows and passes its tests, and the baselines are measured. The engine itself (the net, the search and the protocol) is not built yet; Phase 1 is next.

- [Design spec](docs/SPEC.md)
- [Decision register](docs/DECISIONS.md)
- [Build prompt for the coding agent](docs/BUILD_PROMPT.md)
- [G0, the Phase 0 gate report](docs/gates/G0.md), and [the rulings and spec revision that followed it](docs/gates/G0-rulings.md)
- [Measurements](docs/MEASUREMENTS.md)
- [Provenance of the copied files](PROVENANCE.md)
- [The prompt that started Phase 0](docs/INITIAL_PROMPT.md), kept for the record

To build and test what exists:

```
cmake -S . -B build/engine -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/engine
python tools/run_tests.py --exe build/engine/t5d-tests
```

Licensed under GPL-3.0 (see `LICENSE`). The human games from t0database keep their MIT licence, and the oracle harness's ftxi driver keeps the BSD 2-Clause notice for the part adapted from ftxi.

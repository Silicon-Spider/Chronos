# Fractal: rules for every agent session

Fractal is a standalone, CPU-only engine for 5D Chess With Multiverse Time Travel, in C++20: alpha-beta over whole turns, scored by an NNUE that reads the boards. The design is `docs/SPEC.md`, the decisions are `docs/DECISIONS.md`, and the build plan is `docs/BUILD_PROMPT.md`.

- Nothing in the engine's build or in the running engine depends on Temporal-5D, Kairos or any other engine. The harness's pinned oracles are test fixtures under `oracles/third_party/`, run as separate programs in tests only; a clone without submodules still builds the engine and passes its tests.
- Temporal-5D's rules engine is copied in once (spec §3.1), with every file's provenance in `PROVENANCE.md` and in a header where the file can carry one; nothing from §3.2.
- No code from Kairos, Stockfish, Lc0, KataGo, dlshogi, YaneuraOu or the oracle engines in the engine, its tools or its trainer. Ideas only.
- Copied code passes its own tests and the gate's commit tier after each change, and the nightly tier before a change to it merges.
- No hand-crafted evaluation of any kind. The net is the only evaluator; unweighted rule-fact counts appear only in arm NX2.
- Nets train only on Fractal's own searches and results, never on another engine's evaluations.
- The search never calls per-move exact legality. A turn is legal because the turn generator yielded it, the depth-first detector returned it, or it was replayed whole and passed the submission test. A stopped or failed generator never proves mate, and no mate is reported unless every legal turn was searched.
- Rules logic lives only in the rules engine. Store games and positions, never features.
- Incremental updates equal from-scratch rebuilds; the integer eval is bit-identical across targets; a node-limited single-thread search is deterministic.
- Don't change a committed decision without a written proposal in a gate report. Don't edit `docs/SPEC.md` or `docs/DECISIONS.md`: they change only at Fergus's request, and each revision is recorded in `docs/gates/`.
- Record gate results in `docs/gates/`, measurements in `docs/MEASUREMENTS.md` and nets in `NETS.md`. Every gate report ends with the rulings it needs.
- Git: one branch per phase or gate part, a pull request into `main` at each gate, never force-push `main`, and keep data and nets out of git.
- Compute: the desktop by default and always for training. GitHub Actions runs CI and adds spare capacity for test matches and self-play shards, within GitHub's terms and the budget in spec §11; every Actions job also runs locally.
- Build: CMake and Ninja, C++20, g++ 13+ or clang; Windows through MSYS2 ucrt64. Tests need Python 3.12+ and git.

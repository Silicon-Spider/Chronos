# Chronos: rules for every agent session

Chronos is a standalone, CPU-only engine for 5D Chess With Multiverse Time Travel, in C++20: alpha-beta over whole turns, scored by an NNUE that reads the boards. The design is `docs/SPEC.md`, the decisions are `docs/DECISIONS.md`, and the build plan is `docs/BUILD_PROMPT.md`.

- No build- or run-time dependency on Temporal-5D or Kairos. Temporal-5D's rules engine is copied in once (spec §3.1) with a provenance header per file; nothing from §3.2.
- No code from Kairos, Stockfish, Lc0, KataGo, dlshogi, YaneuraOu or the oracle engines. Ideas only.
- Copied code passes its own tests unchanged before any change, and again after each change.
- No hand-crafted evaluation of any kind. The net is the only evaluator; unweighted rule-fact counts appear only in arm NX2.
- Nets train only on Chronos's own searches and results, never on another engine's evaluations.
- The search never calls per-move exact legality. Legal turns come from the turn generator, and a stopped generator never proves mate.
- Rules logic lives only in the rules engine. Store positions, never features.
- Incremental updates equal from-scratch rebuilds; the integer eval is bit-identical across targets; a fixed-node single-thread search is deterministic.
- Don't change a committed decision without a written proposal in a gate report. Don't edit `docs/SPEC.md` or `docs/DECISIONS.md`.
- Record gate results in `docs/gates/`, measurements in `docs/MEASUREMENTS.md` and nets in `NETS.md`.
- Git: one branch per phase, a pull request into `main` at each gate, never force-push `main`, and keep data and nets out of git.
- Build: CMake and Ninja, C++20, g++ 13+ or clang; Windows through MSYS2 ucrt64.

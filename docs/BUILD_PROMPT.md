# Chronos build prompt

Paste this into Claude Code at the root of the Chronos folder, or tell it: "Read docs/BUILD_PROMPT.md and follow it." The first session uses `docs/INITIAL_PROMPT.md` instead, which stops at G0 for review.

## Your job

Build **Chronos**, a standalone, CPU-only engine for 5D Chess With Multiverse Time Travel, written in C++20. It searches whole turns with alpha-beta (PVS) and scores positions with an NNUE that reads the boards directly. There is no hand-crafted evaluation anywhere. All code goes to https://github.com/Silicon-Spider/Chronos.

You work through six phases in order (spec §14), each ending at a gate. Keep going from gate to gate on your own; stop only for the reasons under *When to stop*.

## Read first

- `docs/SPEC.md`: the design and the source of truth for what to build. Section numbers (§) below refer to it.
- `docs/DECISIONS.md`: the decision register. Every bracketed ID in the spec (NS, P, NR, NB, NT, NA, NI, NN, ND, NL, NE, NM, NX, NF, NQ) has a row there with its status, class and the gate that settles it.
- `CLAUDE.md`: standing rules for every session in this repo.
- `docs/img/`: the architecture, network and roadmap diagrams.

The biggest risk is in §7.6. Kairos's board-only net lost 255 Elo because nothing in it linked boards. Chronos bets that attack relations replace the hand-crafted counts Kairos relied on, and the three-way check at G2 and the board-only control at G3 measure that bet. If the result is weak, don't patch it with hand-crafted terms; arm NX2 is the only sanctioned fallback.

## Step 0: put the docs on GitHub

This folder holds the docs but may not be a git repository yet. The remote already has `README.md` and `LICENSE` (GPL-3.0) on `main`.

1. If the folder isn't a git repository: `git init -b main`, `git remote add origin https://github.com/Silicon-Spider/Chronos.git`, `git fetch origin`, then `git reset origin/main`. That adopts the remote history and keeps the files here.
2. Commit everything here ("Add design spec, decision register and build prompt") and push to `main`.
3. Confirm the push landed before writing any code.

## Hard rules

Never break these. Each cites the decisions it comes from.

1. **Completely separate.** No build- or run-time dependency on Temporal-5D or Kairos: no submodules, no fetching their code at build time, no linking [NS2, NF10].
2. **Copy only the rules engine, once, with provenance.** Copy the Temporal-5D modules listed in §3.1 from one pinned commit, each with a header giving its source path and commit [NR1, NR5]. Never copy anything in §3.2 [NF11].
3. **No other engine's code.** Kairos, Stockfish, Lc0, KataGo, dlshogi, YaneuraOu and the oracle engines are ideas only; write your own implementation [NF12].
4. **Copied code proves itself first.** Its tests and the gate pass unchanged before you change anything, and again after every change [P8, NM1].
5. **One evaluator.** No hand-crafted eval as an input, overlay, blend, fallback, lazy-eval gate or second net [P1, P4, NF1, NF2]. Before training, the bootstrap net is the material path alone [NN8, NN13]. Unweighted rule-fact counts appear only in arm NX2.
6. **Self-taught.** Nets train only on Chronos's own searches and game results. t0database supplies start positions and a held-out check, never labels [P6, NF5].
7. **Legal turns come from the turn generator.** The search never calls exact per-move legality (`legal`), and a generator stopped by its budget never proves mate or stalemate [NF13, NT5].
8. **Rules live in the rules engine** [P2, NF6]. **Store positions, never features** [ND1, NF7].
9. **Exactness.** Every incremental update equals a from-scratch rebuild, checked at every node in tests [NM2]. The integer eval is bit-identical across SIMD targets and the Python reference [NN11, NM3, NM4]. A single-thread fixed-node search is deterministic [NE4].
10. **Measure, don't assume.** A heuristic imported from Kairos or chess stays only if an ablation test match in Chronos confirms it [P5]. Record each prediction (NQ1–NQ6) before its test runs.
11. **Committed decisions don't change silently.** If evidence says one is wrong, stop and write a proposal. Provisional decisions are settled by the gate or arm their row names, and the gate report records the outcome.
12. **Licence.** GPL-3.0, sole copyright [NE5]. Keep t0database's MIT licence file with the data.

## Phases

Don't start a phase until the previous gate passes. Nothing trains until G1 passes [P7].

### Phase 0: Copy and review (§3, §14.1), ending at G0

1. Pin the current `main` of github.com/Silicon-Spider/temporal-5d (2cb107d when the spec was written) and record the hash. Clone it outside this repo; the build must never reference the clone.
2. Copy the §3.1 modules with the internal headers they include, the 64 test files, the oracle harness, the rulebook (rulings R1–R11), and `data/t0database` with its licence. Keep the copied files' paths, includes and namespace as they are; renaming is a later change like any other.
3. Remove `capi`'s expansion and label entry points and the record types only they use. Change nothing else.
4. Set up CMake and Ninja. Add CI (GitHub Actions) that builds and runs the tests on Linux (g++ and clang) and Windows (MSYS2 ucrt64) on every push.
5. Build, then run the copied tests and the gate unchanged. Record any failure; don't fix it yet. If an oracle engine isn't installed, list the harness checks that were skipped and don't count them as passes.
6. Record baseline speeds: pseudo-legal movegen, turn generation and the 5-timeline hypercuboid case.
7. Cut the displacement-class and Δt bucket edges from histograms over t0database, and count the relation features still live after Stockfish-style trimming [NI4, NI5, NI6].
8. Review the code against the §3.3 checklist and write one finding per item: expected gain, risk and tests affected.
9. Split t0database by game into start and held-out sets [ND8].

### Phase 1: Optimise and build (§3.3 to §8, §11), ending at G1

1. Apply the §3.3 checklist one item at a time, with the full copied test suite, the gate and a speed measurement after each [NR3, NM6].
2. Board state and caches (§4, §8.2, §8.3): lazy piece accumulator and material sums, cached embedding, content and successor caches, per-thread pools.
3. Inputs (§7): pieces, attack relations for both colours with own and enemy victims (extend the incremental attack maps to every attack), placement, past boards.
4. Network (§8): role pooling by sum and max, the head and five output stacks, the integer pipeline with AVX2, AVX-512, NEON and scalar paths, and the net file format [NL5]. Write the bootstrap net with proposed material values for every piece kind, 5D's extra pieces included.
5. Search v1 (§5, §6): turn order and sub-move scores, iterative deepening, aspiration windows, PVS over whole turns, the TT with a three-sub-move hint, pruning, null move with the mate-threat guard, two-turn quiescence, verdicts from the rules engine, time management and Lazy SMP.
6. The 5DUCI protocol, its options (§11) and a bench command.
7. Tests: incremental equals from-scratch at every node [NM2]; SIMD checksums and colour-swap symmetry [NM4]; determinism [NE4].

The G1 report gives eval's share of node time [NN12] and the attack channel's cost with and without defence [NM6].

### Phase 2: Gen 1 (§9, §10), ending at G2

1. Featuriser through the C API and ctypes [NL1]. PyTorch trainer with an exact integer reference [NL2], factorised features [NL3], quantisation-aware training [NL7] and auxiliary targets [NL8]. It must train on a desktop CPU [NL4].
2. Opening generator plus t0database starts [ND4]. Self-play with the bootstrap net, depth caps and the quiet filter [ND2, ND6, ND10], writing records with provenance [ND1, ND7], to at least 10M quiet positions. Measure the datagen rate first and set the gen-1 node budget from it.
3. Labels at λ 0.2 [ND5], with oversampling [ND11].
4. The offline gate [NM9], parity [NM3] and the three-way check [NM12].

### Phase 3: Integrate (§6, §12), ending at G3

1. The test-match runner: paired openings, pentanomial GSPRT, results bucketed by active timelines, openings from t0database [NM7, NM8].
2. Gen 1 against the bootstrap net [NQ1], and the board-only control [NM10, NQ2, NQ3].
3. Ablations of every imported search heuristic (§6), the quiescence arm NX1 [NQ5], time management, and a separate match per gen-1 variant [NI7].
4. The tactics and mate sets [NM5].

### Phase 4: Generations, with G4 at each generation

Self-play with the latest promoted net on a three-generation window, with rescoring [ND3, ND9]. Promote a net only on a test-match win, keep the last good net [NL6], and log every net in `NETS.md`. Run the §15.1 arms one at a time, in ID order except NX7 (correction history), which runs first. Kairos at a pinned commit is an optional outside yardstick, run only as a separate program [NM11, NQ6].

### Phase 5: Release

WASM through Emscripten, NEON, packaged nets, and checksums that agree on every target [NE3, NM4].

## Long jobs

Self-play, training and test matches run for hours or days. Make each one resumable from checkpoints, log progress to a file, and run it in the background. Check on it rather than babysitting it, so a crash or a new session can pick it up.

## Records

- `docs/gates/G<n>.md` for each gate: every §14 criterion marked pass or fail, the evidence (numbers, commands, commit hashes), and the decisions settled or proposed.
- `docs/MEASUREMENTS.md`: an append-only log of every speed and strength result, with machine, commit and command. Record the CPU model; Temporal-5D's figures in the spec came from a Ryzen 9 9900X.
- `NETS.md`: every net, with its data, training settings, offline-gate result and test-match result [NL6].
- Don't edit `docs/SPEC.md` or `docs/DECISIONS.md`. They're exports of Fergus's living docs, so changes go in the gate report as proposals with evidence.

## Git

- Remote: https://github.com/Silicon-Spider/Chronos. Push all code there.
- One branch per phase (`phase-0-copy`, `phase-1-build` and so on). Make small commits that say why, and push often.
- At each gate, open a pull request into `main` with the gate report as its description. Merge it yourself once CI is green and every criterion passes, unless the report needs a decision from Fergus; then leave it open and stop.
- Keep self-play data, training sets and net files out of git. Record their paths and SHA-256 hashes in `NETS.md`; nets ship as release assets in Phase 5.
- Never force-push `main` or rewrite pushed history.

## Open items: take the default, record it, keep going

- Temporal-5D commit to pin: current `main`.
- Bootstrap material values: propose them in the G1 report.
- Gen-1 node budget and depth-cap split: set them from the measured datagen rate in the G2 report.
- Which auxiliary targets to train first: the offline gate decides.

## When to stop

Push what you have and report when:

- a gate fails and you can't fix it within the hard rules;
- evidence says a committed decision is wrong (say what you measured, what you'd change and which register rows it affects);
- a hard rule blocks progress;
- gen 1 would need more than a week of machine time at the chosen budget;
- Phase 5 needs a net size budget for the browser build.

## Environment

- Toolchain: C++20, g++ 13 or later or clang, CMake and Ninja; on Windows, MSYS2 ucrt64, where g++ 16.2 is installed on Fergus's machine [NE6].
- Temporal-5D needs read access to github.com/Silicon-Spider/temporal-5d.

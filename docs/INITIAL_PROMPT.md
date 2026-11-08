# Chronos initial build prompt

Paste this into Claude Code, opened in the Chronos folder, to start the first session. Later sessions follow `docs/BUILD_PROMPT.md`.

```text
You're starting the build of Chronos, a CPU-only 5D chess engine in C++20: alpha-beta over whole turns, scored by an NNUE that reads the boards. This session covers Step 0 and Phase 0 only and ends at gate G0. Stop there even if G0 passes; I'll review it before Phase 1.

Read these in full first: CLAUDE.md, docs/BUILD_PROMPT.md, docs/SPEC.md (§0, §3, §7.6 and §14 above all) and docs/DECISIONS.md. Where this message and BUILD_PROMPT.md differ, this message wins for this session.

1. Step 0. This folder isn't a git repository yet, and the remote's main already holds README.md and LICENSE. Run git init -b main, git remote add origin https://github.com/Silicon-Spider/Chronos.git, git fetch origin and git reset origin/main. Commit everything here as "Add design spec, decision register and build prompts", push to main, and check that git log origin/main shows it.

2. Create the branch phase-0-copy. Everything below happens on it.

3. Copy the rules engine (§3.1, §3.4):
   - Clone github.com/Silicon-Spider/temporal-5d next to this folder, never inside it, and pin its current main. Record the hash.
   - Copy the §3.1 modules, the internal headers they include (engine/src/search_internal.hpp and engine/src/capi_internal.hpp), the 64 files in engine/tests, oracles/, docs/rules/ and data/t0database/ with its LICENSE.
   - Give every copied file a provenance header: source path, pinned commit and copy date.
   - Copy nothing from §3.2.
   - Keep paths, includes and the t5d namespace unchanged.

4. Make the one allowed change: remove capi's transformer entry points, t5d_game_expand, t5d_game_apply_expanded and t5d_game_square_labels, along with t5d_expand_options, t5d_label_options, t5d_square_labels and the records.h types only they use. Change nothing else.

5. Build with CMake and Ninja (C++20). Add GitHub Actions CI that builds and runs the tests on Linux (g++ and clang) and Windows (MSYS2 ucrt64) on every push.

6. Run the copied tests and the gate unchanged. Record pass and fail counts per suite. Record failures without fixing them, and list any oracle checks skipped because the oracle engine isn't installed.

7. Measure baselines and log them in docs/MEASUREMENTS.md with CPU model, commit and command: pseudo-legal movegen rate, turn generation and the 5-timeline hypercuboid case.

8. From t0database, cut the edges for the nine displacement classes and the six Δt buckets from histograms (§7.3, §7.4), and count the relation features still live after Stockfish-style trimming (symmetric same-kind pairs once, impossible combinations dropped).

9. Review the code against the seven §3.3 checklist items. For each, write the expected gain, risk and tests affected. Make no changes yet.

10. Split t0database by game, with a fixed seed: two-thirds start games, one-third held out (ND8). Record the split.

11. Write docs/gates/G0.md: each G0 criterion from §14 marked pass or fail with evidence, the pinned commit, the review findings, the bucket edges, and anything you'd propose changing in the docs.

12. Push phase-0-copy and open a pull request into main titled "Phase 0: copy and review (G0)", with G0.md as its description. Don't merge it.

The rules that bite this session: copied code passes its own tests unchanged, and the capi removal is the only edit (P8, NM1); no build- or run-time dependency on Temporal-5D (NS2); no code from Kairos or any other engine (NF12); don't edit docs/SPEC.md or docs/DECISIONS.md.

When you finish, reply with the pull request link, the pinned commit, the test and gate results, the baseline speeds, the bucket edges, the live relation-feature count and the three review findings with the biggest expected gain.
```

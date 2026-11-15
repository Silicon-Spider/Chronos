# Fractal build prompt

Paste this into Claude Code at the root of the Fractal folder, or tell it: "Read docs/BUILD_PROMPT.md and follow it." Phase 0 is done (`docs/gates/G0.md`); the next session starts Phase 1.

## Your job

Build **Fractal**, a standalone, CPU-only engine for 5D Chess With Multiverse Time Travel, written in C++20. It searches whole turns with alpha-beta (PVS) and scores positions with an NNUE that reads the boards directly. There is no hand-crafted evaluation anywhere. All code goes to https://github.com/Silicon-Spider/Fractal.

You work through the phases in order (spec §14), each ending at a gate. Keep going from gate to gate on your own; stop only for the reasons under *When to stop*.

## Read first

- `docs/SPEC.md`: the design and the source of truth for what to build. Section numbers (§) below refer to it.
- `docs/DECISIONS.md`: the decision register. Every bracketed ID in the spec (NS, P, NR, NB, NT, NA, NI, NN, ND, NL, NE, NM, NX, NF, NQ) has a row there with its status, class and the gate that settles it.
- `docs/gates/G0.md` and `docs/gates/G0-rulings.md`: what Phase 0 found, and the rulings and evidence behind the spec's revision of 15 November 2026.
- `docs/MEASUREMENTS.md`: the baselines every later speed result is compared with.
- `CLAUDE.md`: standing rules for every session in this repo.
- `docs/img/`: the architecture, network and roadmap diagrams, drawn by `tools/docs/diagrams.py`.

The biggest risk is in §7.6. Kairos's pure net lost 255 Elo to its counts-fed net; the leading explanation is that nothing in it linked boards, and the record cannot isolate that cause. Fractal bets that attack relations replace the hand-crafted counts Kairos relied on. The family check at G2 and the board-only control at G3 measure that bet. If the result is weak, don't patch it with hand-crafted terms; arm NX2 is the only sanctioned fallback.

## Hard rules

Never break these. Each cites the decisions it comes from.

1. **Completely separate.** Nothing in the engine's build or in the running engine depends on Temporal-5D, Kairos or any other engine: no include, no link, no fetch at build time, no subprocess at run time [NS2, NF10]. The differential harness's pinned oracles are test fixtures: submodules under `oracles/third_party/`, run as separate programs in tests only (§3.5). A clone without submodules must build the engine and pass its own tests, and one CI job keeps proving it.
2. **The rules engine was copied once, with provenance.** Every copied file has a row in `PROVENANCE.md`, and a header where the file can carry one [NR1, NR5]. Never copy anything in §3.2 [NF11]. Later Temporal-5D fixes are merged by hand and recorded.
3. **No other engine's code.** Kairos, Stockfish, Lc0, KataGo, dlshogi, YaneuraOu and the oracle engines are ideas only; write your own implementation [NF12]. The harness's own test drivers keep the notices they came with.
4. **Copied code proves itself.** Its tests and the gate's commit tier pass after every change, and the nightly tier passes before a change to copied code merges [P8, NM1, NR6]. Change copied code one checklist item at a time (§3.3).
5. **One evaluator.** No hand-crafted eval as an input, overlay, blend, fallback, lazy-eval gate or second net [P1, P4, NF1, NF2]. Before training, the bootstrap net is the material path alone [NN8, NN13]. Unweighted rule-fact counts appear only in arm NX2. Piece values used to order captures come from the loaded net's material path, never a second table [NT4].
6. **Self-taught.** Nets train only on Fractal's own searches and game results. t0database supplies start positions and a sanity check, never labels [P6, NF5]. Held-out games never supply a start, for self-play or for a match [ND8].
7. **The search never calls per-move legality** (`legal`). A turn is legal because the turn generator yielded it, the depth-first detector returned it, or it was replayed whole through the state machine and passed the submission test. A generator that was stopped by its budget, or that threw, never proves mate or stalemate [NF13, NT5].
8. **No unproven mates.** A mate score is proven only when every legal turn was searched at each defending node; a node that skipped a turn stores no upper bound [NA9].
9. **Rules live in the rules engine** [P2, NF6]. **Store games and positions, never features** [ND1, NF7].
10. **Exactness.** Every incremental update equals a from-scratch rebuild, checked at every node in tests with the test nets [NM2, NM14]. The integer eval is bit-identical across SIMD targets and the Python reference, under the rules of §8.4 [NN11, NM3, NM4]. A single-thread search limited by nodes is deterministic: integer scores, node-counted budgets, no floating point in any search decision [NE4].
11. **Measure, don't assume.** A heuristic imported from Kairos or chess stays only if its ablation in Fractal confirms it, under the test classes of §12.3 [P5]. On an inconclusive result, keep the simpler engine. Record each prediction (NQ1–NQ9) before its test runs.
12. **Committed decisions don't change silently.** If evidence says one is wrong, stop and write a proposal in the gate report. Provisional decisions are settled by the gate, screen or arm their row names, and the gate report records the outcome.
13. **Licence.** GPL-3.0 [NE5]. Keep t0database's MIT licence file with the data and the BSD notice in the ftxi oracle driver.

## Phases

Don't start a phase until the previous gate passes. Nothing trains until G1 passes [P7].

### Phase 0: Copy and review, done

Delivered at G0. Spec §14.1 lists what it produced; `docs/gates/G0-rulings.md` answers the five rulings it left open.

### Phase 1: Optimise and build (§3 to §8, §11), ending at G1

Three parts, each with its own branch, gate report and pull request. §14.2 is the brief.

**Part a, the rules engine (G1a).**

1. The safety net first. Add the five pinned oracles as submodules under `oracles/third_party/` with `python oracles/harness.py sync` (never a recursive submodule update), build ftxi and the notation parser, and add a second CI job that runs `selftest`, `verify` and `gate --tier commit`. Keep the existing job free of submodules. Run `gate --tier nightly` on the desktop and record what it gates. Never run or try to fix the release tier (§3.5).
2. Recover `test_search` by a recorded trim, cover the copied-code cases of `test_turn_state` and `test_capi_game` with new tests, add the generator pins, allocation counts and the construction-against-`next()` bench split, and make CI assert the number of tests per suite (§3.6). Ask Fergus to protect `main` with the CI jobs as required checks.
3. Apply §3.3 in its order: item 7, then item 6 step by step, then item 1. After each: the copied tests, the commit tier, the pins and a bench entry in `docs/MEASUREMENTS.md`. Before each merge: the nightly tier.

**Part b, the net (G1b).**

4. The relation-fact source over the unchanged movegen API (§3.3 item 4), tested against an independent reading through movegen.
5. Board records and the content cache (§4, §8.2, §8.3); the inputs of §7 (pieces, relations by the reading of §7.3, placement, past boards); pooling, the head and five output stacks; the integer pipeline of §8.4 with scalar, AVX2, AVX-512 and NEON paths; the net file [NL5].
6. The bootstrap net with the values and role weights of §8.1, and the seeded test nets [NM14].
7. Tests: incremental equals from-scratch at every node [NM2]; checksums on every target and the colour swap [NM4]; the rules symmetries [NM18]; sanitiser builds [NM17]. Then measure the cost of an evaluation and of the attack channel, with attacks alone and with defences [NM6].

**Part c, the search (G1c).**

8. Search v1 (§5, §6): option scores and the rule for leaving a board, the candidate set, the budget rule with the detector fallback, the node flow, quiescence, proofs and bounds, time management and Lazy SMP with per-thread roots.
9. 5DUCI as ftxi's draft defines it, with Fractal's move reader and writer, the options, `bench` and `eval` (§11).
10. Tests: soundness against plain minimax [NM15]; mates in one, 1,000 clean self-play games and the bench signature [NM16]; determinism on every CI target [NE4]. Then measure the evaluation's share of search time [NN12].

### Phase 2: Gen 1 (§9, §10), ending at G2

1. The batch featuriser through the C API [NL1]. The PyTorch trainer with an exact integer reference [NL2], factorised features [NL3], quantisation-aware training [NL7], auxiliary targets [NL8] and the file mirror [NL9]. It must train on a desktop CPU [NL4]; record its positions per second.
2. The opening generator and t0database starts [ND4]. Self-play with the bootstrap net on the node clock, with the game cap [ND13] and depth caps [ND10], recording every searched position with its flags [ND1, ND7]. Measure the datagen rate first and set the node budgets from it. Stay on the desktop unless the rate needs the Actions shards of §11 [ND12].
3. The data report of §9.2: results, lengths, the cap rate [NQ9], positions per stack and per variant.
4. Labels and loss as §9.3 defines them [ND5], uniform weights [ND11], the quiet filter at load [ND6], a shuffled window [ND14].
5. Parity [NM3], the learning curve, the family check [NM12], the first four screens of §15.1, and the offline gate [NM9]. Apply what the family check and the screens show, and say so in the G2 report.

### Phase 3: Integrate (§6, §12), ending at G3

1. The test-match runner with the four test classes, the node clock and the wall clock, the opening book and the endings of §12.3 [NM7, NM8]. It runs locally and as an Actions workflow [NM13]. Check its LLR against published fishtest results, and play a 100-game smoke match before any long test.
2. Gen 1 against the bootstrap net [NQ1], and the board-only control [NM10, NQ2, NQ3].
3. Ablations of every imported search heuristic in §6.4 and of the candidate set [NQ8], the quiescence arm NX1 [NQ5], time management, and a separate match per gen-1 variant [NI7]. Then tune the surviving constants.
4. The tactics and mate sets [NM5].

### Phase 4: Generations, with G4 at each generation

Self-play with the latest promoted net on a growing window [ND3], with rescoring only if its test passes [ND9]. Test matches run on Actions, and training stays on the desktop [NM13, NL4]. Promote a net only by the promotion test, keep the last good net, measure each promoted net against the fixed anchors [NL6, §12.3], and log every net in `NETS.md`. Run the §15.1 arms in the order given there. Kairos at commit `194e426` is an optional outside yardstick, run only as a separate program from the harness's directory [NM11, NQ6].

### Phase 5: Release

WASM through Emscripten, NEON, packaged nets, and checksums that agree on every target [NE3, NM4].

## Long jobs

Self-play, training and test matches run for hours or days. Make each one resumable from checkpoints, log progress to a file, and run it in the background. Check on it rather than babysitting it, so a crash or a new session can pick it up.

## Compute

- Your desktop is the default for everything, and training always runs there [NL4].
- GitHub Actions runs CI and the gate's commit tier on every push, and adds spare capacity for test matches from Phase 3 and, only if the measured rate needs it, self-play shards from Phase 2 [NE7, NM13, ND12]. Fractal is public, so standard runners are free: 4 CPUs, 16 GB of RAM, 14 GB of disk and a 6-hour cap per job, with up to 20 jobs at once on GitHub's Free plan.
- Runners come from a mixed pool. Shards run a baseline build (x86-64-v3) on the node clock, play both engines on the same runner with their own slice of the opening book, and record the runner's CPU model [ND7]. A datagen shard uploads its games as an artefact, and the desktop downloads, checks and merges them.
- Keep to the written budget of §11: a match is at most 20 jobs of at most an hour, one match or datagen run at a time and queued rather than cancelled, never on a schedule, artefacts kept a week, status calls under 1,000 an hour.
- Every Actions job also runs locally with the same command; nothing may depend on Actions being available.

## Records

- `docs/gates/G<n>.md` for each gate or gate part: every §14 criterion marked pass or fail, the evidence (numbers, commands, commit hashes), the decisions settled or proposed, and a rulings section for anything only Fergus can decide.
- `docs/MEASUREMENTS.md`: an append-only log of every speed and strength result, with machine, commit and command. Record the CPU model.
- `NETS.md`: every net, with its data, training settings, offline-gate result and test-match result [NL6].
- `PROVENANCE.md`: every change to a copied file and every fix merged from Temporal-5D.
- Don't edit `docs/SPEC.md` or `docs/DECISIONS.md`. They change only at Fergus's request, and each revision is recorded in `docs/gates/`. Put what you would change in the gate report as a proposal with evidence.

## Git

- Remote: https://github.com/Silicon-Spider/Fractal. Push all code there.
- One branch per phase or gate part (`phase-1a-rules`, `phase-1b-net` and so on). Make small commits that say why, and push often.
- At each gate, open a pull request into `main` with the gate report as its description. Merge it yourself once CI is green and every criterion passes. If a criterion fails or the report has an open ruling, leave it open and stop.
- Keep self-play data, training sets and net files out of git. Record their paths and SHA-256 hashes in `NETS.md`; nets ship as release assets in Phase 5.
- Never force-push `main` or rewrite pushed history.

## Open items: take the default, record it, keep going

- Generator budget size: set it at G1 from positions in Fractal's own search trees.
- Gen-1 node budgets and the depth-cap split: set them from the measured datagen rate in the G2 report.
- Stack edges: re-cut them from gen-1 data at G2.
- Which auxiliary targets to train first: the offline gate decides.
- NX2's list of counts: write it before G3.

## When to stop

Push what you have and report when:

- a gate fails and you can't fix it within the hard rules;
- evidence says a committed decision is wrong (say what you measured, what you'd change and which register rows it affects);
- a hard rule blocks progress;
- the nightly tier fails after a change to copied code and the cause isn't your change;
- more than one bootstrap self-play game in five ends on the cap [NQ9];
- gen 1 would need more than a week of machine time at the chosen budget;
- Phase 5 needs a net size budget for the browser build;
- GitHub cancels Actions jobs or restricts the repo (move that work back to the desktop first).

## Environment

- Toolchain: C++20, g++ 13 or later or clang, CMake and Ninja; on Windows, MSYS2 ucrt64, where g++ 16.2 is installed on Fergus's machine. Tests need Python 3.12 or later and git, the native Windows builds rather than MSYS2's [NE6].
- The oracle builds need more: Node 18 or later for the commit tier, and GHC 9.10.3 for the nightly tier's cwmtt, which `python oracles/harness.py build --bootstrap` fetches.
- A read-only clone of Temporal-5D at the pin is useful for its manual and logs. The build must never refer to it.

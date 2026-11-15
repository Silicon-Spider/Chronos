# 5D NNUE engine — decision register

1 November 2026 · Fergus

107 decisions (66 committed, 41 provisional), plus 27 arms, 13 refusals and 9 predictions, for the [design spec](SPEC.md). Revised on 15 November 2026 after G0: IDs kept their meaning, new rows took new IDs, and `docs/gates/G0-rulings.md` gives the evidence for every change. (The rewrite of 7 November renumbered the IDs, so references older than that don't carry over.)

- **Status:** committed rows hold unless a gate or test match overturns them; provisional rows wait on the gate named.
- **Class:** committed-class decisions get expensive to change once code depends on them; retrofittable ones can change later.
- **Gate** names the gate (G0–G4), arm (NX) or measurement (NM) that settles a row; **Spec** names the section with the detail.

## Scope, principles and rules engine

Nineteen rows: 19 committed and 0 provisional.

| ID | Decision | Status | Class | Gate | Spec |
| --- | --- | --- | --- | --- | --- |
| NS1 | Standalone engine, Fractal: own repo (github.com/Silicon-Spider/Fractal), C++20, CPU only, alpha-beta with an NNUE | Committed | Committed-class | — | §0 |
| NS2 | Completely separate: nothing in the engine's build or in the running engine depends on Temporal-5D, Kairos or any other engine. Test-time oracles are fixtures, not dependencies: a clone without submodules builds the engine and passes its own tests | Committed | Committed-class | — | §0, §3.5 |
| NS3 | Borrow Temporal-5D code only where it would be identical in an NNUE engine, after review | Committed | Committed-class | — | §0, §3 |
| NS4 | Kairos: ideas and test results only, no code | Committed | Retrofittable | — | §0, §13.2 |
| NS5 | Gen-1 variants: Standard, Standard - Turn Zero, Standard - Defended Pawn, Standard - Half Reflected | Committed | Retrofittable | — | §0 |
| P1 | Facts, not opinions; unweighted counts only in arm NX2 | Committed | Committed-class | — | §2 |
| P2 | Rules live in the rules engine | Committed | Committed-class | — | §2 |
| P3 | Immutability: per-board work computed at most once per thread while cached, and shared | Committed | Committed-class | — | §2 |
| P4 | One evaluator; a hand-set bootstrap net stands in before training | Committed | Committed-class | — | §2 |
| P5 | Measure, don't assume: an imported heuristic stays only after an ablation test match | Committed | Committed-class | — | §2, §12.3 |
| P6 | Self-taught: nets train only on this engine's own searches and results | Committed | Committed-class | — | §2 |
| P7 | Untrained first: G1 passes before anything trains, with the bootstrap net for play and test nets for exactness and cost | Committed | Committed-class | — | §2 |
| P8 | Borrowed code passes its own tests unchanged before any change, then after each | Committed | Committed-class | — | §2 |
| NR1 | Copy Temporal-5D's rules engine at a pinned commit, with its tests, harness, rulebook and t0database: done at `c8a9c49` | Committed | Committed-class | — | §3.1 |
| NR2 | Don't copy transformer, MCTS or verification-layer code | Committed | Retrofittable | — | §3.2 |
| NR3 | Review checklist before any change, applied one item at a time in the order G0's review set | Committed | Retrofittable | G1 | §3.3 |
| NR4 | Rulings R1–R11 bind; the engine keeps its own rulebook, and Fractal's decisions replace the Temporal-5D conventions it cites | Committed | Committed-class | — | §3.1 |
| NR5 | Every copied file has its provenance recorded in PROVENANCE.md, and in a header where the file can carry one; later fixes merged by hand and recorded | Committed | Retrofittable | — | §3.4 |
| NR6 | The gate: five pinned oracles as test-time submodules; the commit tier in CI, the nightly tier before a change to copied code merges; the release tier unused | Committed | Retrofittable | G1 | §3.5 |

## Position, turns and search

Nineteen rows: 9 committed and 10 provisional. Every imported search heuristic stays provisional until its G3 ablation (P5).

| ID | Decision | Status | Class | Gate | Spec |
| --- | --- | --- | --- | --- | --- |
| NB1 | Board: Temporal-5D's immutable board, with its net state in a per-thread record outside it | Committed | Committed-class | — | §4 |
| NB2 | Per-thread roots, then pooled storage behind the same handle types | Provisional | Retrofittable | G1 | §3.3, §4 |
| NB3 | Persistent multiverse, copy-on-write per timeline; positions are values, with no unmake | Committed | Retrofittable | — | §4 |
| NB4 | Hash: one term per board and per timeline, summed by 64-bit addition; the search keys on the game state's key | Committed | Retrofittable | — | §4 |
| NT1 | Search unit: the complete turn, one option on every playable board, counted by distinct successor | Provisional | Committed-class | NX9 | §5.1 |
| NT2 | Hypercuboid best-first turn generator, copied; legal turns only | Committed | Committed-class | — | §5.2 |
| NT3 | Turn order: integer option scores in bands, with a rule for leaving a board; the hint by replay or by score | Provisional | Retrofittable | G3 | §5.3 |
| NT4 | Sub-move scores: captures by ordering values from the net's material path, then killers and history with gravity and maluses | Provisional | Retrofittable | G3 | §5.3 |
| NT5 | Budget rule: a stopped or failed generator proves nothing; the depth-first detector supplies a turn, else a flagged static eval; the root always plays a legal turn | Committed | Committed-class | — | §5.4 |
| NT6 | Candidate set: every legal turn at the root, mandatory boards only below it, widening when no candidate is legal or all are mated | Provisional | Retrofittable | G3 | §5.5 |
| NA1 | Iterative deepening, aspiration windows, PVS over whole turns | Committed | Retrofittable | — | §6 |
| NA2 | TT keyed by the game state's key: score, bound, depth, static eval, proven bit and up to three sub-moves of the best turn; a whole stored turn is replayed before any generator is built | Provisional | Retrofittable | G3 | §6 |
| NA3 | Reverse futility, internal iterative reduction, late-move caps at depth 1–4, late-move reductions and board-rank pruning | Provisional | Retrofittable | G3 | §6.1 |
| NA4 | Null move as the rules engine's passed position; no reductions or board-rank skips where passing gets mated | Provisional | Retrofittable | G3 | §6.1 |
| NA5 | Quiescence: two turns of capture turns with an exchange test, and evasions in check | Provisional | Retrofittable | NX1 | §6.2 |
| NA6 | Verdicts come from the rules engine: an exhausted generator, then check status; no repetition rule (R5) | Committed | Retrofittable | — | §6.3 |
| NA7 | Time management: a soft and a hard limit per move from the clock | Provisional | Retrofittable | G3 | §6 |
| NA8 | Lazy SMP over one shared TT; each thread owns its root, pools, caches and history | Committed | Retrofittable | — | §6 |
| NA9 | Proofs and bounds: a mate is proven only when every legal turn was searched at each defending node; a node that skipped a turn stores no upper bound and widens while it looks mated | Committed | Committed-class | NM15 | §6.3 |

## Inputs and network

Twenty-one rows: 6 committed and 15 provisional. Inputs and widths stay provisional until the net shows it learns from them. Records store games, so any of them can change later without regenerating data (ND1).

| ID | Decision | Status | Class | Gate | Spec |
| --- | --- | --- | --- | --- | --- |
| NI1 | Boards: head boards in four roles with a fixed precedence, plus past boards of active timelines, near (1–2 half-turns back) and far (3–4) | Provisional | Retrofittable | G2 | §7.1 |
| NI2 | Pieces: 2,048 features per board and view, unmoved flags included; gen 1 uses the six kinds its variants hold | Provisional | Retrofittable | G2 | §7.2 |
| NI3 | Attack relations by G0's reading (semantics B): attacks wherever the victim stands, defences only onto head boards; royals stay as victims | Provisional | Retrofittable | G3, NQ2, NM12 | §7.3 |
| NI4 | Relation index: attacker kind × colour × victim kind × colour × location × class, colours by view, value a count; 68,304 live rows, 18,072 for gen 1 | Provisional | Retrofittable | G2, NX12 | §7.3 |
| NI5 | Nine displacement classes in whole turns, edges cut from t0database at G0 | Provisional | Retrofittable | G2 | §7.3 |
| NI6 | Placement: 18 features per head board, recomputed at each evaluation; Δt in whole turns | Provisional | Retrofittable | G2 | §7.4 |
| NI7 | No ruleset flag: the gen-1 variants share one ruleset, board size and piece set; each gets its own test match | Provisional | Retrofittable | G3 | §7.4 |
| NI8 | Excluded: hand-crafted terms, eval outputs, search-derived values, global counts | Committed | Committed-class | — | §7.5 |
| NN1 | Piece accumulator 2,048 → 256 per view, once per board | Provisional | Retrofittable | NX11 | §8 |
| NN2 | Board embedding 256 → 32 per view, cached with the board | Provisional | Retrofittable | NX11 | §8 |
| NN3 | Relation accumulator → 128 per view from 8-bit rows: the same-board part once per board, the rest scanned at each evaluation; clipped ReLU before pooling | Provisional | Retrofittable | G1, NX11 | §8 |
| NN4 | Placement layer 18 → 16 per view | Provisional | Retrofittable | G2 | §8 |
| NN5 | Six roles: head roles pooled by sum and max (176 per board), past roles by sum (32); an empty role gives zero | Provisional | Retrofittable | NX19 | §8 |
| NN6 | Head: 2 × 1,472 = 2,944 → 32 → 32 → 1 | Provisional | Retrofittable | NX15, NX24 | §8 |
| NN7 | Five output stacks by active-timeline count, with a shared part in training and a fallback for starved stacks; edges re-cut at G2 | Provisional | Retrofittable | G2 | §8 |
| NN8 | Bootstrap net: the material path alone, hand-set, in the same file format | Committed | Committed-class | — | §8.1 |
| NN9 | Incremental contract: board records built at first use from the lineage parent; every update equals a from-scratch rebuild | Committed | Committed-class | NM2 | §8.2 |
| NN10 | Content cache keyed by content hash, board size and net, confirmed on a hit; it is the store of the board records | Committed | Retrofittable | G1 | §8.3 |
| NN11 | Integer pipeline with stated types, bounds and rounding, bit-identical across targets and the reference | Committed | Committed-class | NM4 | §8.4 |
| NN12 | Eval at most 50% of search time, on a stated workload | Committed | Retrofittable | G1 | §8.5 |
| NN13 | Material path: five more piece-table columns, one per stack, summed by role straight into the score; score scale K fixed in the header | Provisional | Retrofittable | G2 | §8 |

## Data and training

Twenty-three rows: 9 committed and 14 provisional.

| ID | Decision | Status | Class | Gate | Spec |
| --- | --- | --- | --- | --- | --- |
| ND1 | Records store games (start plus turns) with a score, flags and result per searched position, never features | Committed | Committed-class | — | §9.1 |
| ND2 | Gen 1: bootstrap self-play, at least 10M quiet-filter positions from at least 250,000 games, more if the learning curve is still falling | Provisional | Retrofittable | G2 | §9 |
| ND3 | Gen 2 on: self-play with the latest net, training on a growing window | Provisional | Retrofittable | G4 | §9 |
| ND4 | Starts: own opening generator for every variant, plus t0database's start games, which are Turn Zero; the timeline mix is steered and reported | Provisional | Retrofittable | G2 | §9.2 |
| ND5 | Labels in win-probability space; λ is the weight on the score, 0.2 in gen 1 and chosen by test after | Provisional | Retrofittable | G2 | §9.3 |
| ND6 | Quiet positions only, filtered at load | Provisional | Retrofittable | NX5 | §9.4 |
| ND7 | Provenance on every game, including the machine or runner and its CPU model | Committed | Committed-class | — | §9 |
| ND8 | Validation split by game: a twentieth of each generation for the offline gate; a held-out third of t0database as a sanity check | Committed | Retrofittable | — | §9.4 |
| ND9 | Rescore stored positions with each newly promoted net | Provisional | Retrofittable | G4 | §9 |
| ND10 | Depth caps: every position recorded; a quarter of turns at the full budget, the rest at a sixth | Provisional | Retrofittable | G2 | §9 |
| ND11 | Sample weights uniform; surprise weighting only after its screen | Provisional | Retrofittable | NX27 | §9 |
| ND12 | Self-play on the desktop; shards on GitHub Actions only if the measured rate needs them | Provisional | Retrofittable | G2 | §9, §11 |
| ND13 | A game ends on the rules engine's verdict or at a cap of 200 turns or 400 boards; a cap is not a verdict | Provisional | Retrofittable | G2 | §9.2 |
| ND14 | The window is shuffled across games each epoch; of duplicate positions the newest label is kept | Committed | Retrofittable | — | §9 |
| NL1 | Featuriser: the engine's feature code through its C API, a whole batch per call | Committed | Committed-class | NM3 | §10 |
| NL2 | PyTorch trainer with an exact integer reference | Committed | Retrofittable | NM3 | §10 |
| NL3 | Factorised features in training, folded in at export | Provisional | Retrofittable | G2 | §10 |
| NL4 | Training runs on a desktop CPU, never on GitHub Actions | Committed | Retrofittable | — | §10 |
| NL5 | Net file header: format, hashes, scales, score scale, stack map, provenance, checksum | Committed | Committed-class | — | §10 |
| NL6 | NETS.md; promote only by the promotion test; keep the last good net | Committed | Retrofittable | — | §10, §12.3 |
| NL7 | Quantisation-aware training: the trainer simulates the engine's integer rounding | Provisional | Retrofittable | G2 | §10 |
| NL8 | Training-only auxiliary targets from the game's future, dropped at export | Provisional | Retrofittable | G2 | §10 |
| NL9 | The file mirror of each position as training data | Provisional | Retrofittable | G2 | §10 |

## Interface and measurement

Twenty-five rows: 23 committed and 2 provisional.

| ID | Decision | Status | Class | Gate | Spec |
| --- | --- | --- | --- | --- | --- |
| NE1 | 5DUCI as ftxi's published draft: long-algebraic moves with `submit`, positions as initial boards plus moves | Committed | Committed-class | — | §11 |
| NE2 | Options: Threads, Hash, EvalFile, MoveOverhead, Nodestime | Committed | Retrofittable | — | §11 |
| NE3 | Targets: AVX2, AVX-512 and NEON; WASM through Emscripten | Committed | Retrofittable | Phase 5 | §11 |
| NE4 | Determinism: a node-limited single-thread search reproduces exactly | Committed | Committed-class | — | §11 |
| NE5 | GPL-3.0, copyright Fergus's, with the MIT data and the BSD-noticed test driver keeping their notices | Committed | Committed-class | — | §11 |
| NE6 | Toolchain: C++20, g++ 13+ or clang, CMake and Ninja, MSYS2 ucrt64; Python 3.12+ and git for tests | Committed | Retrofittable | — | §11 |
| NE7 | Compute: the desktop by default; GitHub Actions for CI and the gate, and as spare capacity for test matches and self-play shards within a written budget; every job also runs locally | Committed | Retrofittable | — | §11 |
| NM1 | Copied tests and the gate's commit tier pass after each change; nightly before copied code merges | Committed | Committed-class | G1 | §12.1 |
| NM2 | Incremental equals from-scratch at every node, with the test nets | Committed | Committed-class | G1 | §12.1 |
| NM3 | Featuriser and integer reference in parity with the engine | Committed | Committed-class | G2 | §12.1 |
| NM4 | Eval checksums agree on every target; a colour swap leaves the eval unchanged | Committed | Retrofittable | G1 | §12.1 |
| NM5 | Own tactics and mate sets, every mate proven by the detector | Committed | Retrofittable | G3 | §12.1 |
| NM6 | Search time by stage; attack-channel cost first, with attacks alone and with defences | Committed | Retrofittable | G1 | §12.1 |
| NM7 | Own test-match runner: paired openings, pentanomial GSPRT in normalised Elo, four test classes | Committed | Retrofittable | G3 | §12.3 |
| NM8 | Results bucketed by the opening's active timelines | Committed | Retrofittable | G3 | §12.1 |
| NM9 | Offline gate as a filter before any match | Committed | Retrofittable | G2 | §12.1 |
| NM10 | Board-only control net | Committed | Retrofittable | G3 | §12.1 |
| NM11 | Kairos at commit 194e426 as an optional outside yardstick | Provisional | Retrofittable | — | §12.1 |
| NM12 | Offline family check: board-only, plus attacks, plus head defences, plus history defences | Committed | Retrofittable | G2 | §12.1 |
| NM13 | Test matches on GitHub Actions: shards on disjoint openings, both engines on the same runner, results pooled and checked | Provisional | Retrofittable | G3 | §12.3 |
| NM14 | Seeded random test nets that reach every clipping bound | Committed | Retrofittable | G1 | §12.1 |
| NM15 | Search soundness against plain minimax on tiny multiverses | Committed | Retrofittable | G1 | §12.1 |
| NM16 | Search function: mates in one, clean self-play games, a bench signature equal on every target | Committed | Retrofittable | G1 | §12.1 |
| NM17 | Sanitiser builds in CI | Committed | Retrofittable | G1 | §12.1 |
| NM18 | Rules symmetries: colour swap and file mirror | Committed | Retrofittable | G1 | §12.1 |

## Arms

Twenty-seven arms, all prototype; NX16 is retired. Input, network and data arms are screened offline at G2 before any match; search arms run as matches after G3, bucketed by active timelines (P5, NM8). Spec §15.1 gives the order.

| ID | Arm | Tests against | Spec |
| --- | --- | --- | --- |
| NX1 | Quiescence depth of 0, 2 or 4 turns; the exchange test on or off | NA5, NQ5 | §15.1 |
| NX2 | Unweighted rule-fact counts as extra inputs; the fallback if relations fail NQ2 | NI8 | §7.5 |
| NX3 | Reach: moves onto empty squares across boards | NI3 | §7.3 |
| NX4 | Attacks only: drop defence relations | NI3 | §7.3 |
| NX5 | All positions instead of quiet ones, or quiescence leaves | ND6 | §9.4 |
| NX6 | Futility pruning | NA3 | §6.4 |
| NX7 | Correction history | NA1 | §6.4 |
| NX8 | Move-prior head for turn order | NT3, NT4 | §15.1 |
| NX9 | One sub-move per ply instead of whole turns | NT1 | §5.1 |
| NX10 | Attention over head boards: a learned ΔL/ΔT/role bias, then a position-dependent mix of bias tables | NN5 | §15.1 |
| NX11 | Widths: relations 64 or 256, pieces 512, embedding 64 | NN1–NN3 | §15.1 |
| NX12 | Full victim squares across boards | NI4 | §7.3 |
| NX13 | The attacker's square in the relation index | NI4 | §7.3 |
| NX14 | King buckets for piece features | NI2 | §15.1 |
| NX15 | Squared clipped ReLU or pairwise products instead of clipped ReLU | NN2, NN3, NN6 | §15.1 |
| NX16 | WDL head (retired) | NN6 | §15.1 |
| NX17 | Signed ΔL | NI5 | §7.3 |
| NX18 | Singular extensions | NA1 | §6.4 |
| NX19 | Pooling: sum alone, or mean with sum and max | NN5 | §8 |
| NX20 | Attacker counts per square, or piece inputs split by attack counts | NI3 | §7.3 |
| NX21 | Proof-number mate solver (df-pn) on a helper thread at the root | NA8 | §15.1 |
| NX22 | Checks first in turn order | NT3 | §6.4 |
| NX23 | Royal neighbourhood: attacks on the empty squares beside a royal | NI3 | §7.3 |
| NX24 | A mixing layer on each head board before pooling | NN5, NN6 | §15.1 |
| NX25 | Branch priority as two inputs to the head | NI8 | §15.1 |
| NX26 | Cross-timeline attack rows split by whether the capture would branch | NI4 | §15.1 |
| NX27 | Sample weights by search surprise | ND11 | §9.5 |

## Refusals

Thirteen refusals. Each holds unless the principle or decision it rests on changes.

| ID | Refused | Rests on | Spec |
| --- | --- | --- | --- |
| NF1 | Hand-crafted eval as an input, overlay, fallback or blend | P1, P4 | §0, §15.2 |
| NF2 | Lazy-eval gates or a second net | P4 | §15.2 |
| NF3 | An offset-indexed kernel over L/T | P2; it can't see whether a slider's path is clear | §7.3 |
| NF4 | Raw-offset indexing | 5D offsets are unbounded | §7.3 |
| NF5 | Training on another engine's evaluations | P6 | §2 |
| NF6 | Rules logic outside the rules engine | P2 | §2 |
| NF7 | Storing features instead of positions | ND1 | §9.1 |
| NF8 | MCTS or a policy-driven main search | NS1 | §15.2 |
| NF9 | GPU inference | NS1 | §15.2 |
| NF10 | Any dependency of the engine's build, or of the running engine, on Temporal-5D, Kairos or an oracle | NS2 | §0, §3.5 |
| NF11 | Copying Temporal-5D code built for the transformer or MCTS | NS3 | §3.2 |
| NF12 | Copying or porting code from Kairos, Stockfish, Lc0, KataGo, dlshogi, YaneuraOu or the oracles into the engine, its tools or its trainer; the harness's test drivers keep their notices | NS4 | §3.5, §13.3 |
| NF13 | Exact per-move legality in the search loop | NT2; it has taken up to 200 s for one position at 600 boards | §5.2 |

## Predictions

Nine predictions, written down before their tests run.

| ID | Prediction | Tested by | Result |
| --- | --- | --- | --- |
| NQ1 | Gen 1 beats the bootstrap net by more than 100 Elo | Measurement match at G3 | Pending |
| NQ2 | The full net beats the board-only control by more than 100 Elo; if it doesn't, NX2 is next | NM10 at G3 | Pending |
| NQ3 | That margin grows with active timelines | Bucketed results (NM8) | Pending |
| NQ4 | Eval takes under 50% of search time | Search-time split (NM6) at G1 | Pending |
| NQ5 | Quiescence at two turns beats zero and four | NX1 | Pending |
| NQ6 | By gen 8 the engine beats Kairos at commit 194e426 at 10+0.1, one thread each | NM11 | Pending |
| NQ7 | Adding defences onto history boards does not lower held-out loss | NM12 at G2 | Pending |
| NQ8 | The mandatory-boards candidate set beats the full set by more than 50 Elo | NT6's ablation at G3 | Pending |
| NQ9 | Fewer than one bootstrap self-play game in five ends on the cap | G2's data report | Pending |

## Open items

- **Phase 1** is the next step (spec §14.2), starting with the oracle checkouts and the nightly tier, which are not yet in place (NR6).
- **Branch protection on `main`** is not yet switched on (spec §3.6).
- **The ftxi oracle pin** predates ftxi's 14 October 2026 change to the check rule that R1 uses; refreshing a pin changes copied harness files (NR5).
- **The oracle corpus:** even the nightly tier gates only Half Reflected among the gen-1 variants, and Defended Pawn has no record; grow the corpus from t0database (NR6).
- **Mate certificates:** the copied certificate module cannot build without the prover, so NM5 uses the detector; decide in Phase 3 whether to write a Fractal-owned checker.
- **Generator budget size,** set at G1 from positions in Fractal's own search trees (NT5).
- **Bootstrap values for the six kinds outside gen 1** (NN8).
- **Gen-1 node budgets,** once the datagen rate is measured (ND2, ND10).
- **Stack edges,** re-cut from gen-1 data at G2 (NN7).
- **NX2's list of counts,** written before G3.
- **Auxiliary targets:** which to train first (NL8).
- **Browser build:** WASM through Emscripten, threads, and the net budget (NE3).
- **Fergus's living docs:** this revision was made in the repository, so they need the same changes before the next export.

## Reversals log

| Date | Was | Now | Why |
| --- | --- | --- | --- |
| 2026-11-15 | The engine was called Chronos | Fractal (NS1) | Renamed by Fergus; the repository, tools and build option follow |
| 2026-11-15 | No oracle checkouts; G0's "gate passes unchanged" unmet | Five pinned oracles as test-time submodules; commit tier in CI, nightly before copied code merges; release tier unused (NS2, NF10, NR6, NM1) | G0 ruling 1: CI compared the engine with an oracle at no position, and the register already kept Kairos as a harness oracle |
| 2026-11-15 | NF12 refused every oracle's code; "sole copyright" | The harness's test drivers keep their notices; NE5 names the MIT data and the BSD-noticed driver (NF12, NE5) | G0 ruling 2 |
| 2026-11-15 | A provenance header on every copied file | A row in PROVENANCE.md for every file, a header where it can carry one (NR5) | 39 of the 221 files have pinned bytes or no comment syntax |
| 2026-11-15 | Board state beside the board; BoardBuilder records changed squares | Per-thread records outside the copied board, built from the lineage parent by comparing square codes (NB1, NN9, NN10, P3) | G0 ruling 5; no copied code changes |
| 2026-11-15 | Royal-attack maps extended to every attack; relations updated from them | The maps stay unused; relations are scanned, and the same-board part is cached with the board (NN3, §3.3 item 5) | The maps' update rule fails under the relation reading, and 70% of heads are replaced each turn |
| 2026-11-15 | Movegen emits the net's relation indices | A Fractal-owned relation-fact source over the unchanged API (§3.3 item 4) | Net indices in copied rules code would break P2 |
| 2026-11-15 | Relations onto own and enemy victims everywhere | Attacks everywhere, defences only onto head boards (NI3) | Half of all relations were defences onto history boards, where nothing can be captured. This also settles G0 ruling 4 |
| 2026-11-15 | The reading for the side not to move unstated | G0's semantics B (NI3) | G0 ruling 3 |
| 2026-11-15 | ΔT in half-turns | Whole turns (NI5) | Pieces move in whole turns |
| 2026-11-15 | 110,592 relation rows, 16-bit | 68,304 live rows, 18,072 for gen 1, 8-bit, colours by view (NI4, NN3) | G0's probe, the head-defence rule and Stockfish's practice |
| 2026-11-15 | Placement: 20 features, Δt buckets to be cut | 18 features; Δt in whole turns (NI6, NN4) | "Who moves next" duplicated "playable"; G0's histogram |
| 2026-11-15 | No ruleset flag because pieces and placement identify the variant | No flag because the gen-1 variants share ruleset, size and piece set (NI7) | The old reason was false for board size and for Turn Zero |
| 2026-11-15 | Activations before pooling and the scale of pooled sums unstated | Clipped ReLU on every part; sums clipped to 8,191 into a 16-bit layer; stated integer rules (NN5, NN11) | §8.4 could not be implemented as written |
| 2026-11-15 | Five independent output stacks | A shared part in training, a fallback for starved stacks, edges re-cut at G2 (NN7) | Human positions average 1.4 active timelines under 50 boards |
| 2026-11-15 | Bootstrap values open; equal role weights | Pawn 100, knight 400, bishop 450, rook 350, queen 1,200; role weights one over the stack's timeline count, inactive a quarter (NN8) | Published 5D value sets; a plain sum rewards branching when ahead |
| 2026-11-15 | Eval at most 50% of node time, undefined | The same bound on a stated workload (NN12) | Not checkable as written |
| 2026-11-15 | Turn order: TT hint, summed scores, quiet turns by their moves | Integer option scores in bands with a rule for leaving a board (NT3) | The generator has one order; Kairos's "quiet by moves" defines a quiet turn and measured −1.6 ± 7.2; best-first failed where every move outscored leaving |
| 2026-11-15 | Every legal turn searched at every node | Mandatory boards only below the root, with widening (NT6) | 12–70% of turn starts have 1,000 or more legal turns; Kairos +117.3 ± 6.2 |
| 2026-11-15 | Pruning: uncapped late-move pruning, LMR, board-rank pruning | Reverse futility, internal iterative reduction, caps at depth 1–4, reductions, board-rank pruning as Kairos defines it (NA3) | Kairos's record re-read: four of the five imports were not wins there, and larger winners had been passed over |
| 2026-11-15 | Quiescence: captures with SEE ≥ 0 | Capture turns with an exchange test from relations; evasions in check (NA5) | SEE has no definition across boards |
| 2026-11-15 | A generator that yields nothing takes a static eval | The depth-first detector first; a failed generator proves nothing; the root always plays a legal turn (NT5) | Temporal-5D: 3,672 detector nodes against 2,000,000; its issue 535 |
| 2026-11-15 | "Never a false mate" by the budget rule alone | Proven mates and honest bounds (NA9) | Pruned nodes can look mated; Kairos's fix gained 17.4 ± 8.2 |
| 2026-11-15 | Arms in ID order, correction history first | Screens before matches, ordered by expected gain (§15.1) | Records store games; Kairos lost with correction history twice |
| 2026-11-15 | Records as 5DFEN or start plus moves; only quiet positions recorded | Games as start plus turns; every searched position recorded with flags; filters at load (ND1, ND6) | 5DFEN loses the mover, en passant and timeline creators; arm NX5 could not have run |
| 2026-11-15 | A three-generation window | A growing window, shuffled, duplicates keeping the newest label (ND3, ND14) | Viridithas's record and KataGo's window |
| 2026-11-15 | Labels λ × score + (1 − λ) × result | The same mix in win-probability space with a fitted scale, a stated loss and K fixed in the header (ND5, NN13) | Not implementable as written |
| 2026-11-15 | Shallow self-play moves unrecorded | Every position recorded with its budget class (ND10) | The result is the scarce label; KataGo's reason does not apply |
| 2026-11-15 | Oversampling by search surprise | Uniform weights; surprise weighting is a screen (ND11, NX27) | KataGo's method is for its policy target; Kairos's weighting did not gain |
| 2026-11-15 | No rule for how a game ends | A verdict or a cap, and a cap is not a verdict (ND13) | The only draw is stalemate and a game need not end |
| 2026-11-15 | 5DUCI defined here over 5DFEN and 5DPGN | The existing 5DUCI draft by ftxi (NE1) | It exists and Kairos speaks it; the copied code has no 5DPGN writer |
| 2026-11-15 | A colour swap negates the eval | A colour swap, now defined, leaves the mover's eval unchanged (NM4) | The net reads the mover's view first |
| 2026-11-15 | Promotion "on a test-match win" | Four test classes with bounds, clocks, caps and a rule for inconclusive results; promotion on [−5, +5] with anchors (NL6, NM7) | Undefined before; Kairos merged 37 inconclusive tests |
| 2026-11-15 | Offline gate: beat the current net on held-out loss | A filter: tolerance, parity and mates solved (NM9) | Offline measures disagreed with play three times in Kairos |
| 2026-11-15 | Three-way offline check | Four nets by relation family (NM12) | Tests the head-defence rule |
| 2026-11-15 | One gate for Phase 1, plumbing criteria only | Three parts, with test nets and functional search criteria (NM14–NM18) | G1's exactness checks pass trivially with the bootstrap net, and nothing tested the search |
| 2026-11-15 | Held-out human games scored against their results | A sanity check on rows the rules engine does not contradict; a twentieth of self-play games held out for the offline gate (ND8) | 227 usable held-out games; 22 rows in the file contradict their final position |
| 2026-11-15 | NS5 provisional | Committed | Confirmed at G0, with the caveats in spec §0 |
| 2026-11-15 | NX16, a WDL head | Retired | Draws are 0.25% of Kairos's games |
| 2026-11-08 | Everything except CI on the desktop | Test matches on GitHub Actions from Phase 3 and optional self-play shards from Phase 2; training stays on the desktop (NE7, NM13, ND12, NL4) | Fractal is public, so standard runners are free; Kairos runs its matches this way |
| 2026-11-07 | Defence relations as arm NX4 | In v1 (NI3); NX4 tests dropping them | Stockfish's threat inputs count both; Kairos's most useful counts need defence |
| 2026-11-07 | Sum pooling only | Sum and max for head roles (NN5) | The worst board decides; Kairos's max-pooling test ran with its worst-board counts present |
| 2026-11-07 | Board state built when a board is published | Built at first use (NN9) | Many generated boards are never evaluated; Stockfish defers its updates the same way |
| 2026-11-07 | Bootstrap net as hand-set weights through the whole net | The material path alone (NN8, NN13) | A direct material path lets gen 1 learn material from the first batch |
| 2026-11-07 | Rust | C++20 (NS1) | Temporal-5D's engine is C++20 with a C API and ctypes, not Rust with PyO3 as first stated; porting its tested rules code would lose that testing |
| 2026-11-07 | Rules shared with Temporal-5D as a crate | Rules engine copied in after review (NS3, NR1) | A completely separate project; code is borrowed only where it would be identical in an NNUE engine |
| 2026-11-07 | Per-move legality tiers not copied | Copied only because search includes them, never called in the search loop (NR1, NF13) | P8: copied code first passes its own tests unchanged |
| 2026-11-07 | Roles and lineages tied to Temporal-5D | Dropped | No relation to Temporal-5D beyond the copied code |
| 2026-11-07 | Kairos as teacher, match runner and test sets | Self-taught gen 1, own runner, own test sets (P6, NM5, NM7) | No relation to Kairos beyond ideas and results |
| 2026-11-07 | Quiet-only training refused | Quiet positions only (ND6) | Kairos: training on all positions lost 43 ± 11 |
| 2026-11-07 | Reach inputs in v1 | Arm NX3 | Kairos: reach terms lost 11 ± 7 |
| 2026-11-07 | Head boards only, no unmoved flags | Past boards near and far, cached embeddings, unmoved flags (NI1, NI2, NN2) | Kairos: past boards won 9.7 ± 7, and the cache makes them nearly free |
| 2026-11-07 | Null move and board-rank pruning left for later | In v1 (NA3, NA4), re-tested at G3 | Kairos test-match wins |
| 2026-11-01 | A GPL-3.0 fork of Kairos | A standalone engine (NS1) | An entirely new engine with an NNUE design, not a fork |
| 2026-11-01 | Hand-crafted terms overlaid on the net | Raw board inputs only (P1, NF1) | The net looks at the board itself |

Rows dated before 15 November give the reasons as they were understood then. Where the 15 November reading of Kairos's record differs (the 43 ± 11, the 11 ± 7 and the "test-match wins" above), spec §13.2 and §6.4 have the corrected account.

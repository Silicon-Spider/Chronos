# 5D NNUE engine — decision register

1 November 2026 · Fergus

93 decisions (56 committed, 37 provisional), plus 21 arms, 13 refusals and 6 predictions, for the [design spec](SPEC.md). Rewritten on 7 November 2026 for a fully separate C++20 project; IDs were renumbered, so older references don't carry over.

- **Status:** committed rows hold unless a gate or test match overturns them; provisional rows wait on the gate named.
- **Class:** committed-class decisions get expensive to change once code depends on them; retrofittable ones can change later.
- **Gate** names the gate (G0–G4), arm (NX) or measurement (NM) that settles a row; **Spec** names the section with the detail.

## Scope, principles and rules engine

Eighteen rows: 17 committed and 1 provisional.

| ID | Decision | Status | Class | Gate | Spec |
| --- | --- | --- | --- | --- | --- |
| NS1 | Standalone engine, Chronos: own repo (github.com/Silicon-Spider/Chronos), C++20, CPU only, alpha-beta with an NNUE | Committed | Committed-class | — | §0 |
| NS2 | Completely separate: no build- or run-time dependency on Temporal-5D or Kairos | Committed | Committed-class | — | §0 |
| NS3 | Borrow Temporal-5D code only where it would be identical in an NNUE engine, after review | Committed | Committed-class | — | §0, §3 |
| NS4 | Kairos: ideas and test results only, no code | Committed | Retrofittable | — | §0, §13.2 |
| NS5 | Gen-1 variants: Standard, Turn Zero, Defended Pawn, Half Reflected | Provisional | Retrofittable | G0 | §0 |
| P1 | Facts, not opinions; unweighted counts only in arm NX2 | Committed | Committed-class | — | §2 |
| P2 | Rules live in the rules engine | Committed | Committed-class | — | §2 |
| P3 | Immutability: per-board work computed once and shared | Committed | Committed-class | — | §2 |
| P4 | One evaluator; a hand-set bootstrap net stands in before training | Committed | Committed-class | — | §2 |
| P5 | Measure, don't assume: an imported heuristic stays only after an ablation test match | Committed | Committed-class | — | §2 |
| P6 | Self-taught: nets train only on this engine's own searches and results | Committed | Committed-class | — | §2 |
| P7 | Untrained first: G1 passes with the bootstrap net before anything trains | Committed | Committed-class | — | §2 |
| P8 | Borrowed code passes its own tests unchanged before any change, then after each | Committed | Committed-class | — | §2 |
| NR1 | Copy Temporal-5D's rules engine at a pinned commit, with its tests, harness, rulebook and t0database | Committed | Committed-class | G0 | §3.1 |
| NR2 | Don't copy transformer, MCTS or verification-layer code | Committed | Retrofittable | — | §3.2 |
| NR3 | Review checklist before any change, applied one item at a time | Committed | Retrofittable | G1 | §3.3 |
| NR4 | Rulings R1–R11 bind; the engine keeps its own rulebook | Committed | Committed-class | — | §3.1 |
| NR5 | Provenance header on every copied file; later fixes merged by hand | Committed | Retrofittable | — | §3.4 |

## Position, turns and search

Seventeen rows: 8 committed and 9 provisional. Every imported search heuristic stays provisional until its G3 ablation (P5).

| ID | Decision | Status | Class | Gate | Spec |
| --- | --- | --- | --- | --- | --- |
| NB1 | Board: Temporal-5D's immutable board, with its accumulator and cached embedding beside it | Committed | Committed-class | — | §4 |
| NB2 | Per-thread pooled storage instead of reference counts | Provisional | Retrofittable | G1 | §3.3, §4 |
| NB3 | Persistent multiverse, copy-on-write per timeline | Committed | Retrofittable | — | §4 |
| NB4 | Hash: each content hash mixed with its place, summed by 64-bit addition | Committed | Retrofittable | — | §4 |
| NT1 | Search unit: the complete turn | Provisional | Committed-class | NX9 | §5.1 |
| NT2 | Hypercuboid best-first turn generator, copied; legal turns only | Committed | Committed-class | — | §5.2 |
| NT3 | Turn order: TT hint, summed sub-move scores, quiet turns by their moves | Provisional | Retrofittable | G3 | §5.3 |
| NT4 | Sub-move scores: captures, then history with gravity and maluses | Provisional | Retrofittable | G3 | §5.3 |
| NT5 | Budget rule: a stopped generator proves nothing; flagged static eval, never a false mate | Committed | Committed-class | — | §5.4 |
| NA1 | Iterative deepening, aspiration windows, PVS over whole turns | Committed | Retrofittable | — | §6 |
| NA2 | TT keyed by the 5D hash, with a three-sub-move hint | Provisional | Retrofittable | G3 | §6 |
| NA3 | Uncapped late-move pruning, LMR and board-rank pruning | Provisional | Retrofittable | G3 | §6 |
| NA4 | Null move; no reductions or board-rank skips where passing gets mated | Provisional | Retrofittable | G3 | §6 |
| NA5 | Quiescence: two turns, captures with SEE ≥ 0 | Provisional | Retrofittable | NX1 | §6 |
| NA6 | Verdicts come from the rules engine; no repetition rule (R5) | Committed | Retrofittable | — | §6 |
| NA7 | Time management: a budget per move from clock and increment | Provisional | Retrofittable | G3 | §6 |
| NA8 | Lazy SMP over one shared TT | Committed | Retrofittable | — | §6 |

## Inputs and network

Twenty-one rows: 6 committed and 15 provisional. Inputs and widths stay provisional until the net shows it learns from them. Records store positions, so any of them can change later without regenerating data (ND1).

| ID | Decision | Status | Class | Gate | Spec |
| --- | --- | --- | --- | --- | --- |
| NI1 | Boards: head boards in four roles, plus past boards near (1–2 half-turns back) and far (3–4) | Provisional | Retrofittable | G2 | §7.1 |
| NI2 | Pieces: 2,048 features per board and view, unmoved flags included | Provisional | Retrofittable | G2 | §7.2 |
| NI3 | Attack relations: both colours, own and enemy victims, owned by the attacker's board | Provisional | Retrofittable | G3, NQ2, NM12 | §7.3 |
| NI4 | Relation index: attacker kind × colour × victim kind × colour × location × class; 110,592 features before trimming | Provisional | Retrofittable | G0, NX12 | §7.3 |
| NI5 | Nine displacement classes, edges cut from t0database | Provisional | Retrofittable | G0 | §7.3 |
| NI6 | Placement: 20 features per head board, recomputed per node | Provisional | Retrofittable | G2 | §7.4 |
| NI7 | No ruleset flag; each variant gets its own test match | Provisional | Retrofittable | G3 | §7.4 |
| NI8 | Excluded: hand-crafted terms, eval outputs, search-derived values, global counts | Committed | Committed-class | — | §7.5 |
| NN1 | Piece accumulator 2,048 → 256 per view, once per board | Provisional | Retrofittable | NX11 | §8 |
| NN2 | Board embedding 256 → 32 per view, cached with the board | Provisional | Retrofittable | NX11 | §8 |
| NN3 | Relation accumulator 110,592 → 128 per view, per node from the attack maps | Provisional | Retrofittable | G1, NX11 | §8 |
| NN4 | Placement layer 20 → 16 per view | Provisional | Retrofittable | G2 | §8 |
| NN5 | Six roles: head roles pooled by sum and max (176 per board), past roles by sum (32) | Provisional | Retrofittable | NX19 | §8 |
| NN6 | Head: 2 × 1,472 = 2,944 → 32 → 32 → 1 | Provisional | Retrofittable | NX15, NX16 | §8 |
| NN7 | Five output stacks by active-timeline count | Provisional | Retrofittable | G3 | §8 |
| NN8 | Bootstrap net: the material path alone, hand-set, in the same file format | Committed | Committed-class | — | §8.1 |
| NN9 | Incremental contract: board state built at first use from the nearest ancestor; every update equals a from-scratch rebuild | Committed | Committed-class | NM2 | §8.2 |
| NN10 | Content cache across nodes; successor cache within a node | Committed | Retrofittable | G1 | §8.3 |
| NN11 | Integer pipeline, bit-identical across targets and the reference | Committed | Committed-class | NM4 | §8.4 |
| NN12 | Eval at most 50% of node time | Committed | Retrofittable | G1 | §8.5 |
| NN13 | Material path: five more piece-table columns, one per stack, summed by role straight into the score | Provisional | Retrofittable | G2 | §8 |

## Data and training

Nineteen rows: 8 committed and 11 provisional.

| ID | Decision | Status | Class | Gate | Spec |
| --- | --- | --- | --- | --- | --- |
| ND1 | Records store positions, scores and results, never features | Committed | Committed-class | — | §9 |
| ND2 | Gen 1: bootstrap self-play, at least 10M quiet positions | Provisional | Retrofittable | G2 | §9 |
| ND3 | Gen 2 on: self-play with the latest net, three-generation window | Provisional | Retrofittable | G4 | §9 |
| ND4 | Starts: own opening generator plus t0database | Provisional | Retrofittable | G0 | §9 |
| ND5 | Labels: λ 0.2 in gen 1, rising toward 0.7 | Provisional | Retrofittable | G2 | §9 |
| ND6 | Quiet positions only | Provisional | Retrofittable | NX5 | §9 |
| ND7 | Provenance on every record | Committed | Committed-class | — | §9 |
| ND8 | Validation split by game; a held-out third of t0database | Committed | Retrofittable | — | §9 |
| ND9 | Rescore stored positions with each newly promoted net | Provisional | Retrofittable | G4 | §9 |
| ND10 | Depth caps: most self-play moves shallow and unrecorded, a fraction deep and recorded | Provisional | Retrofittable | G2 | §9 |
| ND11 | Oversample positions where deep search disagrees most with the static eval | Provisional | Retrofittable | G2 | §9 |
| NL1 | Featuriser: the engine's feature code through its C API and ctypes | Committed | Committed-class | NM3 | §10 |
| NL2 | PyTorch trainer with an exact integer reference | Committed | Retrofittable | NM3 | §10 |
| NL3 | Factorised features in training, folded in at export | Provisional | Retrofittable | G2 | §10 |
| NL4 | Training runs on a desktop CPU | Committed | Retrofittable | — | §10 |
| NL5 | Net file header: format, hashes, scales, provenance, checksum | Committed | Committed-class | — | §10 |
| NL6 | NETS.md; promote only on a test-match win; keep the last good net | Committed | Retrofittable | — | §10 |
| NL7 | Quantisation-aware training: the trainer simulates the engine's integer rounding | Provisional | Retrofittable | G2 | §10 |
| NL8 | Training-only auxiliary targets from the game's future, dropped at export | Provisional | Retrofittable | G2 | §10 |

## Interface and measurement

Eighteen rows: 17 committed and 1 provisional.

| ID | Decision | Status | Class | Gate | Spec |
| --- | --- | --- | --- | --- | --- |
| NE1 | 5DUCI: UCI's commands, positions as 5DFEN or a start plus 5DPGN moves | Committed | Committed-class | — | §11 |
| NE2 | Options: Threads, Hash, EvalFile, Move Overhead, deterministic mode | Committed | Retrofittable | — | §11 |
| NE3 | Targets: AVX2, AVX-512 and NEON; WASM through Emscripten | Committed | Retrofittable | Phase 5 | §11 |
| NE4 | Determinism: a fixed-node single-thread search reproduces exactly | Committed | Committed-class | — | §11 |
| NE5 | GPL-3.0, sole copyright | Committed | Committed-class | — | §11 |
| NE6 | Toolchain: C++20, g++ 13+ or clang, CMake and Ninja, MSYS2 ucrt64 | Committed | Retrofittable | — | §11 |
| NM1 | Copied tests and gate pass unchanged, then after each change | Committed | Committed-class | G0, G1 | §12.1 |
| NM2 | Incremental equals from-scratch at every node | Committed | Committed-class | G1 | §12.1 |
| NM3 | Featuriser and integer reference in parity with the engine | Committed | Committed-class | G2 | §12.1 |
| NM4 | SIMD checksums agree; a colour swap negates the eval | Committed | Retrofittable | G1 | §12.1 |
| NM5 | Own tactics and mate sets | Committed | Retrofittable | G3 | §12.1 |
| NM6 | Node time by stage; attack-channel cost first, with and without defence | Committed | Retrofittable | G1 | §12.1 |
| NM7 | Own test-match runner: paired openings, pentanomial GSPRT | Committed | Retrofittable | G3 | §12.1 |
| NM8 | Results bucketed by active timelines; t0database openings | Committed | Retrofittable | G3 | §12.1 |
| NM9 | Offline gate before any match | Committed | Retrofittable | G2 | §12.1 |
| NM10 | Board-only control net | Committed | Retrofittable | G3 | §12.1 |
| NM11 | Kairos at a pinned commit as an optional outside yardstick | Provisional | Retrofittable | — | §12.1 |
| NM12 | Three-way offline check: board-only, plus attacks, plus defence | Committed | Retrofittable | G2 | §12.1 |

## Arms

Twenty-one arms, all prototype. Each runs after G3, one at a time, as a test match against v1 bucketed by active timelines (P5, NM8): in ID order, except NX7, which runs first.

| ID | Arm | Tests against | Spec |
| --- | --- | --- | --- |
| NX1 | Quiescence depth of 0, 2 or 4 turns | NA5, NQ5 | §15.1 |
| NX2 | Unweighted rule-fact counts as extra inputs; the fallback if relations fail NQ2 | NI8 | §7.5 |
| NX3 | Reach: moves onto empty squares across boards | NI3 | §7.3 |
| NX4 | Enemy victims only: drop defence relations | NI3 | §7.3 |
| NX5 | All positions instead of quiet ones | ND6 | §9 |
| NX6 | Futility pruning | NA3 | §6 |
| NX7 | Correction history; runs first | NA1 | §6 |
| NX8 | Move-prior head for turn order | NT3, NT4 | §15.1 |
| NX9 | One sub-move per ply instead of whole turns | NT1 | §5.1 |
| NX10 | Attention over head boards: a learned ΔL/ΔT/role bias, then a smolgen-style bias from the position | NN5 | §15.1 |
| NX11 | Widths: relations 64 or 256, pieces 512, embedding 64 | NN1–NN3 | §15.1 |
| NX12 | Full victim squares across boards | NI4 | §7.3 |
| NX13 | The attacker's square in the relation index | NI4 | §7.3 |
| NX14 | King buckets for piece features | NI2 | §15.1 |
| NX15 | SCReLU instead of clipped ReLU | NN2, NN6 | §15.1 |
| NX16 | WDL head | NN6 | §15.1 |
| NX17 | Signed ΔL | NI5 | §7.3 |
| NX18 | Singular extensions | NA1 | §6 |
| NX19 | Pooling: sum alone, or mean plus count instead of sum | NN5 | §8 |
| NX20 | Per-square attacker counts per colour, capped at 3 | NI3 | §7.3 |
| NX21 | Proof-number mate solver (df-pn) on a helper thread at the root | NA8 | §6 |

## Refusals

Thirteen refusals. Each holds unless the principle or decision it rests on changes.

| ID | Refused | Rests on | Spec |
| --- | --- | --- | --- |
| NF1 | Hand-crafted eval as an input, overlay, fallback or blend | P1, P4 | §0, §15.2 |
| NF2 | Lazy-eval gates or a second net | P4; Stockfish 19 retired its small net | §15.2 |
| NF3 | An offset-indexed kernel over L/T | P2; it can't see whether a slider's path is clear | §7.3 |
| NF4 | Raw-offset indexing | 5D offsets are unbounded | §7.3 |
| NF5 | Training on another engine's evaluations | P6 | §2 |
| NF6 | Rules logic outside the rules engine | P2 | §2 |
| NF7 | Storing features instead of positions | ND1 | §9 |
| NF8 | MCTS or a policy-driven main search | NS1 | §15.2 |
| NF9 | GPU inference | NS1 | §15.2 |
| NF10 | Any build- or run-time dependency on Temporal-5D or Kairos | NS2 | §0 |
| NF11 | Copying Temporal-5D code built for the transformer or MCTS | NS3 | §3.2 |
| NF12 | Copying code from Kairos, Stockfish, Lc0, KataGo, dlshogi, YaneuraOu or the oracles | NS4 | §13.3 |
| NF13 | Exact per-move legality in the search loop | NT2; worst case 200 s at 600 boards | §5.2 |

## Predictions

Six predictions, written down before their tests run.

| ID | Prediction | Tested by | Result |
| --- | --- | --- | --- |
| NQ1 | Gen 1 beats the bootstrap net by more than 100 Elo | Test match at G3 | Pending |
| NQ2 | The full net beats the board-only control by more than 100 Elo; if it doesn't, NX2 is next | NM10 at G3 | Pending |
| NQ3 | That margin grows with active timelines | Bucketed results (NM8) | Pending |
| NQ4 | Eval takes under 50% of node time | Node-time split (NM6) at G1 | Pending |
| NQ5 | Quiescence at two turns beats zero and four | NX1 | Pending |
| NQ6 | The engine beats the pinned Kairos commit at 10+0.1 by gen 8 | NM11 | Pending |

## Open items

- **Phase 0, copy and review:** the next step (spec §14.1).
- **Which Temporal-5D commit to pin;** 2cb107d is current.
- **Kairos as harness oracle and outside yardstick:** default keep, run only as an outside program (NM11, NS2).
- **Bootstrap material values,** especially for 5D's extra pieces (NN8, NN13).
- **Bucket edges** for displacement classes and Δt, cut from t0database at G0 (NI5, NI6).
- **Relation trimming:** how many of the 110,592 relation features stay live after trimming, counted at G0 (NI4).
- **Auxiliary targets:** which to train first (NL8).
- **Browser build:** WASM through Emscripten, and how much of a net that is about 30 MB before trimming it can carry (NE3).
- **Gen-1 node budget** and the depth-cap split, once the datagen rate is measured (ND2, ND10).

## Reversals log

| Date | Was | Now | Why |
| --- | --- | --- | --- |
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

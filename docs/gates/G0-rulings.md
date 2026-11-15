# G0 rulings and the spec revision of 15 November 2026

G0 (`G0.md`) was merged with one criterion unmet and five rulings open. On 15 November 2026 Fergus renamed the project from Chronos to Fractal and asked for `docs/SPEC.md` to be read over in full and improved, with freedom to change decisions. This report records what was ruled, what changed and on what evidence. It is the written record that the rule "don't change a committed decision without a written proposal in a gate report" asks for; the register's reversals log has one row per change.

## How the revision was made

- **Evidence.** Nine reading passes, each with file-and-line or URL evidence: four over the copied engine, the rulebook and Phase 0's outputs; five over the outside sources the spec cites (Stockfish, KataGo, Lc0 and the shogi engines, Kairos's full test record, the 5D ecosystem, and current training, testing and GitHub Actions practice). They flagged 247 spec statements as wrong, stale, vague or not buildable.
- **Checked again at the source for this report:** every figure in *Phase 0 data* below (`tools/phase0_figures.py`, MEASUREMENTS entry 2), the Temporal-5D measurements quoted in spec §5, the existence and content of the 5DUCI draft, and the copied headers the search calls (`turngen.hpp`, `search.hpp`, `state.hpp`, `check.hpp`, `movegen.hpp`, `attacks.hpp`).
- **Taken from the reading passes without a second check:** Kairos's test figures (read from its commit messages and test summaries), the Stockfish, KataGo, Lc0, shogi and Viridithas facts, GitHub's documentation, ftxi's audit of t0database, and the two symmetry claims (colour swap and file mirror). Each is cited to its primary source in the spec's §13 or below, and NM18 tests the symmetries before anything relies on them.
- **Not done.** A second, independent design review of the revised text was planned and did not run. The decisions below are one editor's reading of the evidence. Every design change is provisional and names the gate, screen or ablation that settles it.

## Rulings on G0's five questions

| # | Question | Ruling | Where |
| --- | --- | --- | --- |
| 1 | The gate needs oracle checkouts, which NS2 and NF12 seemed to rule out | The five pinned oracles are test-time fixtures: git submodules under `oracles/third_party/`, run as separate programs in tests only. The commit tier runs in CI and the nightly tier before any change to copied code merges. The release tier is not used. Kairos stays pinned and unbuilt, which makes the harness's copy check a standing test of NF12 against it. A CI job without submodules keeps proving the engine builds and passes its own tests alone | Spec §3.5; NS2, NF10, NF12, NR6, NM1 |
| 2 | The ftxi oracle driver carries code adapted from ftxi under BSD 2-Clause | It stays, under its notice, as a test-only program the engine never links. NE5 now names both third-party notices | Spec §3.5, §11; NF12, NE5 |
| 3 | The relation reading for the side not to move | G0's "semantics B, turn-aware" is confirmed and written into §7.3. It covers every R1 check under the default rules and is wider than check | Spec §7.3; NI3 |
| 4 | Whether a piece's defence of its own earlier self counts | It does not. The revision goes further: no defence onto a history board counts (see *The largest changes*) | Spec §7.3; NI3 |
| 5 | Board state outside `t5d::Board`, per thread, built from the lineage parent by a diff | Accepted, with the cache key (content hash, board size, net; confirmed on a hit) and the no-parent case stated | Spec §4, §8.2, §8.3; NB1, NN9, NN10, P3 |

Ruling 1 is not yet carried out: the submodules, the second CI job and branch protection on `main` are Phase 1's first step (spec §14.2), and they are listed as open items.

## G0's sixteen proposed doc changes

All sixteen are adopted. In brief: provenance as a row for every file and a header where possible (1); certificate copied but not buildable, with NM5 using the detector (2); the seven tools files and the 36 of 64 test files that build (3); Fractal-owned tests for the copied code no built test reaches (4); the gate's real state (5); movegen's relations for the colour to move only (6); ΔT in whole turns (7); the same-kind pair kept once per view (8); the reading wider than check (9); past-self defences excluded (10); Δt buckets (11, now cut in whole turns); checklist items 4 and 7 re-scoped (12); the board-state readings (13); the corpus facts and the MCTS origin of "27 turns per node" (14); a game as a cluster of duplicate rows, with contradicted rows left out of the held-out check (15); the pin `c8a9c49` (16).

## The largest changes

Each row gives the decision, the evidence and the test that can overturn it.

| Change | Evidence | Settled by |
| --- | --- | --- |
| **Relations: attacks everywhere, defences only onto head boards** (NI3). 68,304 live rows instead of 77,136; 81.5 relations per position instead of 162.2 | A piece lands only on boards where its own colour is to move (`movegen.hpp`, rule L4), so a piece on such a history board cannot be captured there and a "defence" of it protects nothing. Those defences were 49.7% of all relations in the start games | NM12's fourth net at G2 (NQ7) |
| **The same-board part of the relation sum is cached with the board** (NN3, NN10) | Same-board relations depend only on the board's contents; they are 60.0 of the 81.5 relations per position | NM2; NM6 |
| **The royal-attack maps are not extended** (§3.3 item 5) | Their update rule needs boards that never change and appear only in void; the reading's passed copies break both, and 69.8% of head boards are mandatory | — |
| **8-bit relation rows and stated integer rules** (NN11) | §8.4 gave no rule for a sum over many boards; Stockfish stores threat rows in 8 bits; AVX2's byte multiply-add saturates above 127 | NM4 with the test nets |
| **Option scores in bands, with a rule for leaving a board** (NT3) | The generator has one order, fixed when it is built (`turngen.hpp:97-124`). Where every move outscored leaving, best-first spent 2,000,000 nodes without a legal turn (Temporal-5D `docs/BUILD_LOG.md:4853-4857`) | G3 ablations |
| **Mandatory boards only below the root** (NT6) | 12.1% / 46.4% / 70.5% of human turn starts have at least 1,000 legal turns; humans play an optional board in 1.2–2.5% of turns; Kairos +117.3 ± 6.2 | NQ8 at G3 |
| **Proven mates and honest bounds** (NA9) | With late-move caps and rank pruning every searched turn can be mated while an unsearched one escapes; Kairos's fix gained 17.4 ± 8.2 | NM15 |
| **The search's imports from Kairos, restated** (§6.4) | Of the five the spec called "test-match winners", only board-rank pruning's first form passed its test; the others measured +0.2, +1.2, +4.8 and −1.6. Reverse futility, internal iterative reduction and killers, which did pass, were not in the spec | G3 ablations |
| **Quiescence as capture turns with an exchange test** (NA5) | SEE has no definition across boards. Kairos's own leaf rule adds a hand-computed capture gain to the eval, which NF1 refuses | NX1 |
| **The existing 5DUCI** (NE1) | ftxi's draft 0.3.3 exists, and Kairos and Kata5DChess speak it; the copied code has no 5DPGN writer and 5DFEN cannot carry a mid-game position | — |
| **Records are games; every searched position is recorded; filters apply at load** (ND1, ND6, ND10) | 5DFEN loses the mover, en passant and timeline creators. Recording only quiet positions would have made arm NX5 impossible | G2 |
| **How a game ends** (ND13) | The only draw is stalemate and every move adds a board. The longest of 692 human games has 99 turns and 186 boards | NQ9 at G2 |
| **Labels and loss defined** (ND5) | The spec mixed a score with a result and named no scale or loss | G2 |
| **A growing, shuffled window** (ND3, ND14) | Viridithas's record: new-data-only nets lost 60–107 Elo and unshuffled data ruined nets; KataGo's window grows | G4 |
| **Test classes, clocks and the promotion rule** (NM7, NL6) | "A test-match win" had no bounds. Kairos's standard test ends inconclusive 77% of the time, and 37 such changes were merged | G3, G4 |
| **G1 in three parts, with test nets and search criteria** (NM14–NM18) | The bootstrap net's non-material weights are zero, so "incremental equals from-scratch" and the checksum tests pass with wrong features; nothing in G1 tested the search | G1 |
| **Bootstrap net: values and role weights** (NN8) | No agreed 5D values exist; three of four published sets put bishop ≥ knight > rook. A plain material sum rewards branching when ahead | NQ9; NQ1 |

## What the Kairos record says, re-read

The spec's §7.6 and §13.2 rested on a reading of 7 November. The record was read again in full on 15 November (main `194e426`): 400 commits on main, 1,681 on its test branch, 497 test summaries over 4.13M games. Nothing had been tested since; the corrections come from the same record.

- The pure-net loss of 255.5 ± 18.2 is right, but it cannot be laid to one cause: the blend's removal alone cost 30.3 ± 9.7, and about 122 of the 172 counts the pure net lost are single-board facts.
- Its relation channel did play, once, inside a counts-fed net trained on all positions (−43.0 ± 10.9 against the hand-crafted eval). That same test is the one the spec cited for "quiet positions only", so the quiet-only prior is confounded.
- Better evidence for linking boards exists and was not cited: +103.9 ± 15.8 for counts of cross-board danger to royals.
- Its "reach" test fed a slider count times history length to a residual net. It says nothing about arm NX3.
- It lost with correction history twice, so NX7 no longer runs first.

## Phase 0 data, read again

From `tools/phase0_figures.py` over G0's own outputs (MEASUREMENTS entry 2):

- 162.2 relations per position: 15.9 attacks and 146.3 defences. 80.7 of the defences land on history boards.
- Same-board 37.0%, timeline-only 13.3%, time-only 30.4%, both 19.3%. Time-only relations always land on history.
- Turn starts with at least 1,000 legal turns: 12.1% under 50 boards, 46.4% at 50–99, 70.5% at 100–199.
- Δt in whole turns over 25,202 head boards: 611 behind the present, 17,611 at it, then 1,663 / 1,375 / 1,853 / 2,089.
- Of the 692 usable human games, 31% end on one timeline and 1% on nine or more.

## Small errors in G0's own record

G0.md is left as the report it was. Three details in it are not carried into the spec:

- "88.6% of turns" under 50 boards is Temporal-5D's figure for turns of the 692 games; in Fractal's own count 85.3% of distinct turn starts are under 50 boards.
- Five clusters of duplicate rows have conflicting results (26, 27 and 158 in the start set; 391 and 392 held out), where G0 names two pairs.
- MEASUREMENTS entry 1 cites commit `6364eb0` for a command whose script first appears in `2346376` (noted in entry 2).

## Still open

- Carrying out ruling 1: submodules, the second CI job, the nightly tier, branch protection.
- The ftxi pin predates its 14 October 2026 change to the check rule R1 uses.
- Even the nightly tier gates only Half Reflected among the gen-1 variants; Defended Pawn has no oracle record.
- Fergus's living docs do not yet have this revision.
- The working papers behind this report (the nine evidence briefs) are kept outside the repository and can be added if wanted.

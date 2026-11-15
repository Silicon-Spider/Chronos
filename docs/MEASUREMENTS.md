# Measurements

An append-only log of every speed and strength result, newest last. Each entry gives the machine, the commit, the build and the exact command, so anyone can re-run it (BUILD_PROMPT.md, *Records*).

Conventions:

- **Machine.** CPU model, cores and logical processors, RAM and OS, and whether the machine was otherwise quiet.
- **Build.** Compiler and version, build type and flags, as the tool prints them.
- **Commit.** The Fractal commit the binaries were built from.
- **Rates.** Medians over interleaved rounds, with the slowest and fastest round in brackets, unless an entry says otherwise. Percentiles are nearest rank.
- **Comparison.** Temporal-5D's figures quoted in the spec came from the same CPU model (Ryzen 9 9900X), so the Phase 0 entries compare against them directly.
- **Names.** The project was called Chronos until 15 November 2026, and this log now gives the current names throughout. At a commit from before the rename, the tools are `chronos-bench` and `chronos-relhist`, and the CMake option is `CHRONOS_WERROR`.

## 1. Phase 0 baselines: movegen, turn generation, the 5-timeline hypercuboid case (2026-11-15)

The copied rules engine, unchanged apart from the capi removal, measured before any §3.3 change (G0).

- **Machine:** AMD Ryzen 9 9900X 12-Core Processor (12 cores, 24 logical processors), 31.1 GiB (32 GB) RAM, Windows 11 Pro 10.0.26300.
  - The machine was quiet: 1.2% busy just before the run (PowerShell `Get-Counter '\Processor(_Total)\% Processor Time' -SampleInterval 2 -MaxSamples 3`, averaged). The bench itself doesn't sample load during the run.
  - One thread, pinned to logical processor 2, priority above normal.
- **Build:** GNU g++ 16.2.0 (MSYS2 ucrt64), Release, `-O2 -DNDEBUG`, statically linked.
- **Commit:** Fractal `6364eb0` on `phase-0-copy`; engine copied from Temporal-5D `c8a9c49`.
- **Commands:**

  ```
  cmake -S . -B build/engine -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe -DFRACTAL_WERROR=ON
  cmake --build build/engine
  python -I data/t0database/to_5dpgn.py build/t0db
  build/engine/fractal-bench --records build/t0db --json build/g0/bench.json
  python -I tools/bench/turngen_table.py build/g0/bench.json
  ```

- **Defaults:**
  - movegen: a warm-up round, then 15 interleaved rounds, each with one 200 ms sample per class and bucket (8 samples);
  - synthetic positions: 16 games per class, 6 one-move children per base position;
  - turn generation: 3 passes, at most 1,000 turns per generator, a 2,000,000-node budget.

  Total run time 137 s.

### Pseudo-legal movegen, one thread

Each sample calls `generate_moves` on a stream of positions, cycled to the sample's deadline.

- **Synthetic classes.** Temporal-5D M9's definition: seeded random play from Standard to N boards with T active timelines (seed = 1 + 1,000,003 N + 10,007 T + 37 g). The stream is 6 one-move children of each of the 16 games' end positions.
- **Records rows.** Every distinct turn start of the 716 t0database records that replay to the end in parse mode (no royal-safety test at submission), by board count.

| Positions | Positions/s, median [slowest, fastest round] | Thread CPU ns per position (pooled) | Moves | Boards | Active timelines | Temporal-5D M9 (same CPU, `2740e66`) |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| N100 T6, synthetic | **157,707** [155,442, 159,630] | 6,319 | 448.5 | 104.1 | 6.35 | 171,804 |
| N160 T7, synthetic | 128,031 [126,642, 128,769] | 7,822 | 635.2 | 164.8 | 7.34 | 139,053 |
| N240 T8, synthetic | 100,813 [99,590, 101,339] | 9,933 | 817.8 | 246.3 | 8.16 | 102,263 |
| N400 T10, synthetic | 76,990 [74,894, 77,667] | 12,995 | 1,196.9 | 406.9 | 10.23 | 80,213 |
| N600 T12, synthetic | **57,279** [56,562, 57,664] | 17,577 | 1,622.0 | 606.2 | 12.19 | 58,002 |
| t0database, under 50 boards (19,410 positions) | 611,419 [577,093, 644,353] | 1,624 | 71.5 | 25.1 | 1.40 | - |
| t0database, 50-99 boards (3,162) | 398,444 [380,178, 402,087] | 2,515 | 130.3 | 63.6 | 2.70 | - |
| t0database, 100-199 boards (189) | 286,479 [277,764, 288,504] | 3,518 | 218.9 | 123.7 | 4.24 | - |

The synthetic rates are 1-8% below M9's. The seeds, the class definition and the timing loop are the same. Four things differ:

- the children chosen from each position;
- the engine commit (M9 measured `2740e66`; this copy is `c8a9c49`, whose movegen and state code changed in between);
- what shares a round (M9 interleaved its one-thread samples with multi-thread and expansion samples);
- pinning and priority.

The records rows time the positions themselves rather than their children, so they don't compare with M9's records. No t0database position reaches 200 boards.

### Turn generation, one thread

- **What runs.** The copied hypercuboid `TurnGenerator` (unit `Successors`) at each base position, pulled until it is done or has given 1,000 turns.
- **Scorer.** It has no chess knowledge. Each option scores a deterministic hash of the move, in [0.001, 1), and leaving a board scores 0.
- **Times.** Medians over the 3 passes per position, then over positions.
- **Columns.** "Up to 27 turns" times min(27, available) turns, until the generator is done if there are fewer; Temporal-5D's MCTS pulled about 27 turns per search node. "µs per turn, average to done" is (time until done or 1,000 turns) / turns. "µs per further turn" excludes the first: (done − first) / (turns − 1).
- **Positions with no legal turn.** The generator ends exhausted with none, which proves mate or stalemate. Most such t0database positions end their game. They are shown apart, because they cost a proof instead of turns.

| Positions | With a legal turn | First turn (construction + first `next()`), µs, median [p90] | Up to 27 turns, µs | µs per turn, average to done | µs per further turn | Turns/s (pooled) | No legal turn: count, median µs |
| --- | ---: | --- | ---: | ---: | ---: | ---: | --- |
| t0database, under 50 boards | 19,023 | **16.6** [52.1] | 91.5 | 3.10 | 2.83 | 167,434 | 387, 41.6 |
| t0database, 50-99 boards | 2,965 | 40.0 [125.9] | 174.9 | 5.13 | 4.73 | 139,620 | 197, 56.7 |
| t0database, 100-199 boards | 176 | 73.5 [354.2] | 309.5 | 7.13 | 6.88 | 120,081 | 13, 45.2 |
| N100 T6, synthetic | 14 | 703.7 [948.7] | 854.0 | 14.68 | 14.04 | 60,364 | 2, 508.0 |
| N160 T7, synthetic | 16 | 887.5 [1,882.3] | 1,265.2 | 19.20 | 18.40 | 34,159 | 0 |
| N240 T8, synthetic | 15 | 1,919.9 [3,083.5] | 3,096.5 | 27.48 | 25.99 | 34,388 | 1, 1,020.6 |
| N400 T10, synthetic | 12 | 3,938.8 [11,710.4] | 3,941.8 | 34.17 | 28.97 | 29,003 | 4, 3,608.8 |
| N600 T12, synthetic | 9 | 15,501.6 [101,041.7] | 15,504.9 | 54.80 | 37.39 | 13,199 | 7, 7,375.0 |

Notes:

- **Up to 27 turns.** Among positions with a legal turn, the time ran until the generator was done for 1,386 of 19,023, 418 of 2,965 and 10 of 176 in the three t0database buckets, which have fewer than 27 legal turns.
- **Synthetic rows.** They rest on 9-16 positions each, and random play is pseudo-legal, so it often ends without a legal turn. At N600 T12 the first turn takes between 3.9 ms and 101 ms.
- No generator was stopped by its node budget.

### The 5-timeline hypercuboid case

- **Position.** The near-mate family from `test_detectors.cpp` (`detectors_the_hypercuboid_stays_small_near_mate`): timelines -n..+n on 5x5 boards, all mandatory, a mate on timeline 0, and on every other board white rooks a1 and b1 and a black knight whose moves can't help. n = 2 is the 5-timeline case.
- **Measure.** `detect_mate`, median time per call. The hypercuboid figures come from thousands of calls. The naive search ran 3, 3 and 1 calls for n = 1, 2 and 3, so its n = 3 figure is a single run.

| n (timelines) | Hypercuboid | Naive | Temporal-5D (engine README) |
| --- | --- | --- | --- |
| 1 (3) | 126 nodes, 50.7 µs | 2,407 nodes, 0.75 ms | 126 against 2,407 nodes |
| **2 (5)** | **184 nodes, 75.4 µs** | **817,875 nodes, 0.589 s** | 184 nodes in under 1 ms, against 817,875 nodes in 0.6 s |
| 3 (7) | 248 nodes, 102.0 µs | stopped by the memory ceiling after 1,007,435 nodes, 0.705 s: unknown | 248 against 1,007,435 nodes and a stop |

All proven verdicts are checkmate, as the test expects. A `TurnGenerator` on the n = 2 position ends exhausted with no turn after 796 nodes, 464 µs.

## 2. Phase 0 outputs read again: turn width, relation families, live rows, Δt buckets, game lengths (2026-11-15)

Nothing was run again for this entry. It reads further figures out of the files Phase 0 wrote, for the spec's revision of 15 November 2026 (`docs/gates/G0-rulings.md`).

- **Inputs:** entry 1's `build/g0/bench.json`, and the outputs of G0's relation run on the same machine and build:

  ```
  build/engine/fractal-relhist --records build/t0db --games build/g0/start-games.txt --records-out build/g0/records.tsv --out build/g0/start.json
  build/engine/fractal-relhist --probe build/g0/probe.json
  ```

  `start-games.txt` lists the 507 start games of `data/splits/t0database-nd8.tsv`. The table step of entry 1 (`tools/bench/turngen_table.py`) first appears in commit `2346376`, not `6364eb0`.
- **Command:**

  ```
  python -I tools/phase0_figures.py --bench build/g0/bench.json --hist build/g0/start.json --probe build/g0/probe.json --records build/g0/records.tsv --split data/splits/t0database-nd8.tsv
  ```

### Turn starts with at least 1,000 legal turns

Entry 1's turn-generation run stopped each generator at 1,000 turns. These are the t0database turn starts that reached that cap.

| Positions | Turn starts | With a legal turn | At the cap | Share of those with a legal turn |
| --- | ---: | ---: | ---: | ---: |
| Under 50 boards | 19,410 | 19,023 | 2,302 | 12.1% |
| 50-99 boards | 3,162 | 2,965 | 1,376 | 46.4% |
| 100-199 boards | 189 | 176 | 124 | 70.5% |

That run's scorer gave every move a higher score than leaving its board, so with several playable boards its first turns play all of them.

### Relations by family, kind and where the victim stands

The start games: 2,496,465 relations over 15,387 distinct turn starts, 162.2 per position. An attack reaches an enemy piece and a defence an own one. Figures are per position.

| Family | Attacks onto heads | Attacks onto history | Defences onto heads | Defences onto history | All |
| --- | ---: | ---: | ---: | ---: | ---: |
| Same board | 6.90 | 0.00 | 53.11 | 0.00 | 60.00 (37.0%) |
| Timeline only | 0.84 | 1.18 | 7.69 | 11.82 | 21.54 (13.3%) |
| Time only | 0.00 | 5.19 | 0.00 | 44.17 | 49.36 (30.4%) |
| Both | 0.24 | 1.59 | 4.81 | 24.70 | 31.34 (19.3%) |
| **All** | 7.98 | 7.96 | 65.61 | 80.70 | 162.25 |

- Defences onto history boards are 1,241,663 relations, 49.7% of all.
- Attacks, and defences onto head boards, are 1,254,802: 81.5 per position, 60.0 of them on the attacker's own board and 21.5 leaving it.
- 1,478,807 relations (59.2%) are read on a passed copy.
- A victim on a passed copy counts as on its timeline's head.

### Live relation rows with defences kept only onto head boards

The same count as G0's (77,136 live rows, 20,376 for the gen-1 kinds), less the time-only defence rows: a time-only relation always lands on a history board.

| Rows | All twelve kinds | The six gen-1 kinds |
| --- | ---: | ---: |
| On the attacker's own board | 28,752 | 8,472 |
| Across boards | 39,552 | 9,600 |
| **Live** | **68,304** | **18,072** |
| Time-only defence rows, live at G0 and dropped here | 8,832 | 2,304 |

### Δt to the present in whole turns

G0's histogram of 25,202 head boards, with a half-turn distance rounded up to whole turns.

| Bucket | Behind | 0 | 1 | 2 | 3-4 | 5+ |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Head boards | 611 | 17,611 | 1,663 | 1,375 | 1,853 | 2,089 |
| Share | 2.4% | 69.9% | 6.6% | 5.5% | 7.4% | 8.3% |

### How the 692 usable human games end

| Timelines at the end | 1 | 2 | 3-4 | 5-8 | 9+ |
| --- | ---: | ---: | ---: | ---: | ---: |
| Games | 214 (30.9%) | 141 (20.4%) | 239 (34.5%) | 91 (13.2%) | 7 (1.0%) |

- Turns: 27,097 in all; mean 39.2, median 37, longest 99.
- Boards at the end: median 44, 95th percentile 85, most 186.

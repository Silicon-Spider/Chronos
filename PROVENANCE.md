# Provenance

Chronos copies Temporal-5D's rules engine once, from one pinned commit, and never depends on Temporal-5D at build or run time (spec §3.1 and §3.4; decisions NR1, NR5 and NS2). This file records what was copied, from where, and what has changed since.

## Source

- Repository: https://github.com/Silicon-Spider/temporal-5d
- Pinned commit: `c8a9c490bfde650466609531f8083d69513c6036`, the head of `main` when the copy was made (committed 2026-11-08 21:05 +1100).
- The spec names 2cb107d (2026-11-02). It is an ancestor of the pin, nine commits back, and no copied file differs between the two: `git diff 2cb107d c8a9c49` over the 221 copied paths is empty.
- Copied: 2026-11-08, 221 files. Paths, includes and the `t5d` namespace are unchanged.

## How each file carries its provenance

- **Header** (182 files): a five-line comment at the top gives the source path, the commit, the copy date and the file's state. In Python files it comes after the shebang, where there is one. Below the header, an unchanged file is byte for byte the source.
- **Record only** (39 files): the file is byte for byte the source, and its row in the table below is its provenance. Something pins each of these files' bytes, and a header would break the pin: a hash or signature the harness checks, a strict format, or a format with no comments. The table says what pins each one.

The table gives each file's source SHA-256: the hash of the source bytes, without any header. `python tools/check_provenance.py` re-checks every copied file against it.

## Beyond the spec's list

The spec lists the modules, the 64 test files, the harness, the rulebook and t0database. Seven more files from Temporal-5D's `engine/tools/` were copied, because the copied tests and the gate can't build or run without them. Each includes only copied modules.

- `engine/tools/t5d_gate.cpp` is the `main()` of `t5d-gate`, the engine adapter that `oracles/gate.toml` runs and that `test_gate_exe.cpp` starts.
- `engine/tools/synthetic.{hpp,cpp}` and `engine/tools/tool_support.{hpp,cpp}` are needed by `test_synthetic.cpp`.
- `engine/tools/turn_width.{hpp,cpp}` are needed by `test_turnwidth.cpp`.

## Not copied

- Anything in spec §3.2: expand, labels, views, mcts, mcts_evaluators, selfplay, capi_search, scanner, guard, hunt, prover, ladder and assets.
- The harness's five oracle submodules (the `oracles/third_party/*` gitlinks) and Temporal-5D's `.gitmodules`. Chronos has no submodules and fetches no oracle code (NS2, NF10, NF12).
- Temporal-5D's other tools (`t5d-measure`, `t5d-bench`, `t5d-play` and the rest), `engine/testdata/`, `engine/build.py`, `engine/CMakeLists.txt` and `engine/cmake/`. Chronos has its own build (`CMakeLists.txt`).
- Temporal-5D's `PROVENANCE.md`. The copied harness generated the inventory block at the end of this file in Chronos (`python oracles/harness.py provenance --write`).

## Changes since the copy

None yet.

## Licences

- Temporal-5D: GPL-3.0, by the same author as Chronos (NE5).
- t0database: MIT. The licence, `data/t0database/LICENSE`, stays with the data.
- `oracles/drivers/ftxi/ftxi_driver.cpp` adapts code from ftxi's `tools/position_tools.cpp` under the BSD 2-Clause licence. The notice is inside the file. This conflicts with NF12; see `docs/gates/G0.md`.

## Copied files

| Path | Provenance | Source bytes | Source SHA-256 | Why copied / what pins it |
| --- | --- | ---: | --- | --- |
| `data/t0database/Data.csv` | record only | 608066 | `37fb7c1ce46efb27d42a8b9d0b230df13faff1b36bef24af8c15ba5c8a6c7a4e` | spec 3.1, t0database with its MIT licence; pinned by the SHA-256 in data/t0database/to_5dpgn.py; CSV has no comments |
| `data/t0database/LICENSE` | record only | 1069 | `55c8e40cca749238beecc3be21ec5886978326150a46665e1028dac387f0a07f` | spec 3.1, t0database with its MIT licence; pinned by the source's MIT notice, copied byte for byte (data/t0database/README.md) |
| `data/t0database/README.md` | header | 1941 | `2d795421df3364ad6fe1cdb8cd4448e01c5e4203506be83268c7cdc989882e01` | spec 3.1, t0database with its MIT licence |
| `data/t0database/to_5dpgn.py` | header | 2266 | `5453720bebb6e40e2a3aeb8691e7fbc45709bc357bbc91d97022685dff8d5977` | spec 3.1, t0database with its MIT licence |
| `docs/rules/README.md` | record only | 171668 | `ab01119ebdfa2612e38df10860a916e15b4227c9966f3c4018aac552049c2261` | spec 3.1, the rulebook (rulings R1-R11); pinned by the SHA-256 in oracles/gate.toml [rulebook] |
| `engine/include/t5d/attacks.hpp` | header | 6986 | `48fb21d252b65e0792e2e9f2193b86e265e331f6f70692f28689ba6252b5d90d` | spec 3.1, module attacks |
| `engine/include/t5d/board.hpp` | header | 7234 | `78a5b58e68d0bf23842c40f77e44169f66330e378011e0153e08016f4bdc6ccf` | spec 3.1, module board |
| `engine/include/t5d/capi.h` | header | 26571 | `08b74f32e8a5ff8df49392776f4a9ed7dedd4eef4b0791e268235e1eaa65916f` | spec 3.1, module capi (its header) |
| `engine/include/t5d/certificate.hpp` | header | 3064 | `123ebbd4b367f0da563115eeaf329f217a6ba953e0342c6b467330c14e5e327c` | spec 3.1, module certificate |
| `engine/include/t5d/check.hpp` | header | 9882 | `c9e9eefd027a4e8343e1bca999d7e57c80659029bb1c5b464b9cf9af6105de66` | spec 3.1, module check |
| `engine/include/t5d/error.hpp` | header | 867 | `63f6bd4b484d8fd309abb9db44b8a1339f41bcdc8a5fc1a00532ebacd2fc5f96` | spec 3.1, module error |
| `engine/include/t5d/fen.hpp` | header | 8024 | `49898489643b97918da82ff67ee5f807713b9bd63ccc2c9a5089d9945a4e059f` | spec 3.1, module fen |
| `engine/include/t5d/fingerprint.hpp` | header | 2091 | `df2b492e7bf211a6154af01723786ec98fa88b36ae6c863daa086d6c1b0916f5` | spec 3.1, module fingerprint |
| `engine/include/t5d/gate.hpp` | header | 8235 | `1a2ea23b9021e4ce62838676d42ba614eeb4d9bf53bfca94ef4c71b710977cea` | spec 3.1, module gate |
| `engine/include/t5d/geometry.hpp` | header | 6810 | `98c6c97db1ee087374123c8f488a9f8be62caafe6ca290b6bdaf2c69c6b82381` | spec 3.1, module geometry |
| `engine/include/t5d/legal.hpp` | header | 8234 | `cda8feb42828080652c78072b29ebac8b0e1b4db73c1b963d0c1b6d82c7386c8` | spec 3.1, module legal |
| `engine/include/t5d/md5.hpp` | header | 1285 | `fe671d7e672a9fbc6d2820a3342f43d2b35621936902e5fe2125e4f8bdcc96be` | spec 3.1, module md5 |
| `engine/include/t5d/move.hpp` | header | 4024 | `8c3ae8b76ff9120b518876e461908e28f009a3e65e637838daa1ed53aa9365d1` | spec 3.1, module move |
| `engine/include/t5d/movegen.hpp` | header | 11563 | `657ad09d4758f6b49a6b145aa6d81bd48a1154e5b4807e26f7c240465e885d2e` | spec 3.1, module movegen |
| `engine/include/t5d/multiverse.hpp` | header | 15989 | `ac1f32276ba5bffcef44afba30c41f4e2adf3418a77c089581b85f659993e083` | spec 3.1, module multiverse |
| `engine/include/t5d/pgn.hpp` | header | 22756 | `a08fa7a98a22243c4b5f3541d23991ec8b66dd23fece714ae77ad754c73e1cd5` | spec 3.1, module pgn |
| `engine/include/t5d/records.h` | header | 3724 | `b8dcaf06108973c45aa051e7c88dc19c8d0a93a9f16db6d05915df9d6c5d25e5` | spec 3.1, module capi (included by capi.h) |
| `engine/include/t5d/rules.hpp` | header | 18967 | `c0c71f64cd94252a979e0baaa2c5fa662398cd22c7ad213b6e7ef5fba2280f88` | spec 3.1, module rules |
| `engine/include/t5d/search.hpp` | header | 12137 | `4cb28790cfd847470bf500c707c7995b14fd7843df54666305d4deef0ec63db3` | spec 3.1, module search |
| `engine/include/t5d/state.hpp` | header | 17656 | `c8e1a114d82a1d3135e8cba1f477308c14d699cdee87c825afa804a35142a66f` | spec 3.1, module state |
| `engine/include/t5d/timeline.hpp` | header | 4002 | `66cbfbfcf9bbfe5a05ddd863126aae2df5227f0d57dbea9e6e499f5608100169` | spec 3.1, module timeline |
| `engine/include/t5d/turngen.hpp` | header | 8836 | `5a6ab89e02df5c62feaaf53baff17139366a5cb02a010877c425599455b9969d` | spec 3.1, module turngen |
| `engine/include/t5d/types.hpp` | header | 10094 | `28bde19d76e06b7044cbb6eccaa1e1618da4810f95805fef741557ad2b971b6e` | spec 3.1, module types |
| `engine/include/t5d/variants.hpp` | header | 3891 | `db3a50a129d51454b4b832c3636a340be5d1472a602858425cdd24b569a457c8` | spec 3.1, module variants |
| `engine/include/t5d/zobrist.hpp` | header | 13616 | `c1c921a6175ea826c086d0ca1cf4661de712139bc6da833e1efec6ce7625f4c8` | spec 3.1, module zobrist |
| `engine/src/attacks.cpp` | header | 19222 | `0cfba7bbbe523e7b381d9a597cd913fa195c35f68f450f9e9d5e3a6ef9044f26` | spec 3.1, module attacks |
| `engine/src/board.cpp` | header | 2791 | `21217230a112cdd733979dbdf59f635fc4852dfd55453158019676e4861dcbdd` | spec 3.1, module board |
| `engine/src/capi.cpp` | header | 34761 | `3a044b988783b46a6d079860d041b7b57e706aae4d00928edff102bce858086d` | spec 3.1, module capi |
| `engine/src/capi_internal.hpp` | header | 1603 | `28822b847d76fae3a23edf7983f30d1fc15b024d01246fab71e3deadf4298e42` | spec 3.1, internal header of capi |
| `engine/src/certificate.cpp` | header | 5604 | `573d48a21852589bcce9c07de02ebf9bd9f7b6ac7eff0e54bc18de099b998b53` | spec 3.1, module certificate |
| `engine/src/check.cpp` | header | 17014 | `c450c0b21980aa6b869aea6b01b3b4a2abaaf7d5919c7c988eaef6592eb81e17` | spec 3.1, module check |
| `engine/src/fen.cpp` | header | 22514 | `32631412ea322e6c7543d45500f07554d889243616c13bf6c8f67c46bf8e245f` | spec 3.1, module fen |
| `engine/src/fingerprint.cpp` | header | 1315 | `d68c5313b4a40a4b8dcadb8b16d557f62c9c8e019ce416be31d1274f238a040a` | spec 3.1, module fingerprint |
| `engine/src/gate.cpp` | header | 18036 | `dbbd7545e7e825b23f46fc32eb29b89ad87707d414a7302e86652f6224655710` | spec 3.1, module gate |
| `engine/src/hypercuboid.cpp` | header | 95786 | `c8b4c13896ccdcabb498dc0df2f85f78185a014c5d3e8220556ea1ed9f47619f` | spec 3.1, module hypercuboid |
| `engine/src/legal.cpp` | header | 38235 | `77e9af8dba2ac5166c9db119fafec3e0cfcc85171c7cf16aa35d5ee40b077b5a` | spec 3.1, module legal |
| `engine/src/md5.cpp` | header | 5013 | `dfca049838b69994deec938f400354191fddeb9623e35d43fbda49886eb943fa` | spec 3.1, module md5 |
| `engine/src/move.cpp` | header | 889 | `90a392ce231f91c4ac4871b824e6c20be5354ed21f93350d57a66c567c08d65c` | spec 3.1, module move |
| `engine/src/movegen.cpp` | header | 28988 | `3a61d7ad21d9b2f806f57c9c32844f3b3b9952c5092ce2b1dc30d56b14e8a4d1` | spec 3.1, module movegen |
| `engine/src/multiverse.cpp` | header | 10713 | `eba66fd981f06ad73621a3b33337b81a582eb5ebce3e47f0ea8c34202d3d048b` | spec 3.1, module multiverse |
| `engine/src/pgn.cpp` | header | 60973 | `cbbde5d66913664f476ab1239e933e47dc149887b63f413629cb3513f5db9de9` | spec 3.1, module pgn |
| `engine/src/rules.cpp` | header | 8634 | `fd10d98e799ad88a011c233ccc30a4ef63be598352ee4bc7d2b0f929ea6189fb` | spec 3.1, module rules |
| `engine/src/search.cpp` | header | 13883 | `bd558825ebb14057b1f342e340e45df3cc38c37251ac4f06bc3ea45ccea840d6` | spec 3.1, module search |
| `engine/src/search_internal.hpp` | header | 7314 | `f800f90d51e3cc88dfa2121d4924015d1b602f4bc86f54c8842fcc2f4c6e8cf8` | spec 3.1, internal header of search and hypercuboid |
| `engine/src/state.cpp` | header | 29039 | `24f1b8335abdac406b5650d6387466da6cada07e4472465d0da7c14d213ac768` | spec 3.1, module state |
| `engine/src/timeline.cpp` | header | 3421 | `fc1e878159d8bb0f062766242dfb4b62176ca34f07e0f4ee4b7813c60f043177` | spec 3.1, module timeline |
| `engine/src/types.cpp` | header | 780 | `85496326b0424947ad9830ab074897c70320dce8328613725aeeed0743b1f262` | spec 3.1, module types |
| `engine/src/variants.cpp` | header | 8878 | `70b26db7f8c6690f945e2cf2e384688b4bfdd59d18067231284dcdbe950785ca` | spec 3.1, module variants |
| `engine/src/zobrist.cpp` | header | 960 | `89acab9450c1da955ef22eefc26ad51fc1ad068b0d6f51bb1590f8d4d9bae003` | spec 3.1, module zobrist |
| `engine/tests/test_alloc.cpp` | header | 3940 | `aac935208819eb4305c1f3521c14b8cd0727274f1849b72989118d0893062afb` | spec 3.1, one of the 64 test files |
| `engine/tests/test_alloc.hpp` | header | 1283 | `ac209a52c95174b2ad65a95dd4464ecca01c29f59543fb1aba2c4065adcd137a` | spec 3.1, one of the 64 test files |
| `engine/tests/test_apply.cpp` | header | 15326 | `eb282c115f90851691fedf7ea71928b82e6ea49d16c177c45770afe459d787a5` | spec 3.1, one of the 64 test files |
| `engine/tests/test_assets.cpp` | header | 4205 | `0d3fc1bc5545528c1344b5ce47e166efb16e6cebf7525133517b3a55a928616b` | spec 3.1, one of the 64 test files |
| `engine/tests/test_attacks.cpp` | header | 11486 | `f337e0febc80545c0bcbbdbf8388162a7688deeded58fb3cc07f8c321ea72e06` | spec 3.1, one of the 64 test files |
| `engine/tests/test_audit.cpp` | header | 19841 | `e52d8b4015c48f40446dc85bf52c5d3965e18d1cf8e0a705aafa20bc93788fa3` | spec 3.1, one of the 64 test files |
| `engine/tests/test_board.cpp` | header | 10136 | `c4df0baab4f3d076a196d31c33492c1f4557ab5df8c00d9e07e2efb433819248` | spec 3.1, one of the 64 test files |
| `engine/tests/test_capi.cpp` | header | 13034 | `dfb34132a5684a046f700302f324ec8b92a9cb07849692a7a636b336609f22a6` | spec 3.1, one of the 64 test files |
| `engine/tests/test_capi_game.cpp` | header | 35286 | `c389c48fab0aa8e5883f2d9c27c6532e41f21de8ea5f2f6727a3003498b3db1e` | spec 3.1, one of the 64 test files |
| `engine/tests/test_capi_search.cpp` | header | 60673 | `299f357d8fe7fa32b638ca86b46961623d0cb3aa3dec89aeaa95ae366aec02df` | spec 3.1, one of the 64 test files |
| `engine/tests/test_certificate.cpp` | header | 24028 | `f84adfa6fe4502235838189cd544823e850080417bd4fc3252a99a52f5dde77b` | spec 3.1, one of the 64 test files |
| `engine/tests/test_check.cpp` | header | 23955 | `98d5e8cceb148fac44e784cba3b97eb0b13d261660e50e85557e15c80bac6618` | spec 3.1, one of the 64 test files |
| `engine/tests/test_detectors.cpp` | header | 33761 | `2a56218a7ebc01fc4fe147fe9e3ac37c76fa76543ab97a0ead600cdefeb9e71b` | spec 3.1, one of the 64 test files |
| `engine/tests/test_expand.cpp` | header | 43626 | `8f3727dfc6366198b0c8f56f76b9a2ac5b300e01220c83a55e41babbf95792ab` | spec 3.1, one of the 64 test files |
| `engine/tests/test_expand_reference.cpp` | header | 35287 | `b3dd36722f1cc92e65a88c3641c46752eae8210a02cde32323253d3cdaa541d9` | spec 3.1, one of the 64 test files |
| `engine/tests/test_fen.cpp` | header | 22235 | `858ecd2296186d01c0f643cdc404eefe2509341863e51ef6650ba1322a69ec1b` | spec 3.1, one of the 64 test files |
| `engine/tests/test_fingerprint.cpp` | header | 6868 | `2a499882ddeb26cdcd796c216e06deda7d3e116a7294239a49d69b812a8a30c1` | spec 3.1, one of the 64 test files |
| `engine/tests/test_framework.hpp` | header | 8753 | `ddbe83a80d6a81aaeba8874b33c836cc7ff759d35fd363cb481b560b25268dfe` | spec 3.1, one of the 64 test files |
| `engine/tests/test_game_tree.hpp` | header | 6046 | `2e3a0a7ae724ca68c902a132f45e4c7f89c8fe86632730205924ba1092c7096b` | spec 3.1, one of the 64 test files |
| `engine/tests/test_gate.cpp` | header | 26992 | `96f4fe815ddc24cd1e5fac29b478b645925fb12c76706d9abe1bb99ab8cda510` | spec 3.1, one of the 64 test files |
| `engine/tests/test_gate_exe.cpp` | header | 8793 | `cfa9d250a8ca790b9f0d8a1b75b5a7ef59f94b8661481b73b3afce450d6e217c` | spec 3.1, one of the 64 test files |
| `engine/tests/test_guard.cpp` | header | 19254 | `438bf3b4f473db5cafd9328002e657799f46122dd063bfc9c7c26cbff73b12b0` | spec 3.1, one of the 64 test files |
| `engine/tests/test_hunt.cpp` | header | 5916 | `b5e5059287c53048a9aa7af01eabb57525cc2d3ebe232854e9a2d25723650715` | spec 3.1, one of the 64 test files |
| `engine/tests/test_hunt_truth.cpp` | header | 12991 | `f21651595d986e316f00079ce255f246f133634ff09a4fdcdd22797465d7cce1` | spec 3.1, one of the 64 test files |
| `engine/tests/test_ladder.cpp` | header | 57289 | `54f19a6fb614cd46870d6b5d840a23b47c5db1bc2f0d60cd4b8be600b463baab` | spec 3.1, one of the 64 test files |
| `engine/tests/test_main.cpp` | header | 2965 | `73d72439beacd745ace1ecf5339112b36994714a731357a876e425a06ddd6fa1` | spec 3.1, one of the 64 test files |
| `engine/tests/test_mate_refs.hpp` | header | 14042 | `64561a3a736562ca4de80d8825a4c0545d5746f6286d1480eea301fe8ac957aa` | spec 3.1, one of the 64 test files |
| `engine/tests/test_mcts.cpp` | header | 202474 | `7c07b9d457f897aff6e563891be727ab8d18a9f4f472a954201b80c586fbe344` | spec 3.1, one of the 64 test files |
| `engine/tests/test_mcts_memory.cpp` | header | 9668 | `9b9326c03944d5aa11fa58c1a9eb5508f5bc1cc7372f94ea8049745eac36afc7` | spec 3.1, one of the 64 test files |
| `engine/tests/test_mcts_tree.cpp` | header | 47281 | `446c76e9d1da3828cbe51ae3c191eb06aebc826f2bba58e836b9e0a4c4f1e891` | spec 3.1, one of the 64 test files |
| `engine/tests/test_md5.cpp` | header | 3098 | `3a33316e356488bb8907f8e9336fb323f434eaa7376979e5cc7c41fe225e472c` | spec 3.1, one of the 64 test files |
| `engine/tests/test_movegen.cpp` | header | 9600 | `22d7caacd4d4702ba8292e6ef9ef51fa94323620f57c45e33c2b1787a0ecf080` | spec 3.1, one of the 64 test files |
| `engine/tests/test_movegen_timelines.cpp` | header | 20327 | `8e290a4d634d55c24c553d8177dcadd4b6bf296a58e9fb68383931b1732d9d2e` | spec 3.1, one of the 64 test files |
| `engine/tests/test_multiverse.cpp` | header | 18995 | `f2ce4635c8b57a344052a6e2e517323be898dd4b7a26b93bd1809575d3f471b0` | spec 3.1, one of the 64 test files |
| `engine/tests/test_pgn_examples.cpp` | header | 24992 | `f7c6d34cc939ac33e40f9ce9e386d02757a341db3db4ab435415ffd9e22091b2` | spec 3.1, one of the 64 test files |
| `engine/tests/test_pgn_read.cpp` | header | 20267 | `3247871f41526226e90c73658c4f3b5075a087dac8721b4bf92ac982e3192aea` | spec 3.1, one of the 64 test files |
| `engine/tests/test_pgn_replay.cpp` | header | 40968 | `417002525808b01166bff9e6084c565c225696c851659fcd987d2532fe8acc91` | spec 3.1, one of the 64 test files |
| `engine/tests/test_play.hpp` | header | 4186 | `2da766452668c7b20d00491b96ff01594939ea38d5bc799f6f95c07ddae13d78` | spec 3.1, one of the 64 test files |
| `engine/tests/test_play_exe.cpp` | header | 11612 | `00ba4a7738170e693bcacd2db95bf733cc1de563e2b3ab5ad6ec65d8040294d0` | spec 3.1, one of the 64 test files |
| `engine/tests/test_positions.hpp` | header | 4515 | `a1cd303b1896badc2d7f46f4ae98118ebb329aabe5ecdd9aab7dbf0c8054d1a2` | spec 3.1, one of the 64 test files |
| `engine/tests/test_prover.cpp` | header | 25322 | `342d3aaa2bb9dd383889589a5e7517f2056516e6bde287a18af1c687e226f376` | spec 3.1, one of the 64 test files |
| `engine/tests/test_random_play.cpp` | header | 15197 | `f57b09efea100fceaa6b766f6bf8fedca4bfa364916549174bf2b479722c2001` | spec 3.1, one of the 64 test files |
| `engine/tests/test_reference.hpp` | header | 3164 | `1a07c847372f97b0c3f27b1ce14a79ed992ba21efe99e24729b17138b0688464` | spec 3.1, one of the 64 test files |
| `engine/tests/test_rules.cpp` | header | 8696 | `d80bdffdba5b46b457eeb83d6973d1e9c409a2200f80a8885e339d1bebc199e6` | spec 3.1, one of the 64 test files |
| `engine/tests/test_rulesets.hpp` | header | 2686 | `dfa1fe69a78e201efa279791417f40d9b6f3bdb8ebc0711ee53be21f6359d8f3` | spec 3.1, one of the 64 test files |
| `engine/tests/test_scanner.cpp` | header | 46155 | `d4e0c41cbea82bc565cd44f82c7f0bea9bf607f16e682b806e7e4d0e3ad45a62` | spec 3.1, one of the 64 test files |
| `engine/tests/test_search.cpp` | header | 33455 | `b235f9ead4cf8c54d6675544285ba1f9597a4830fa70cf3eb9d26bfed08ae40c` | spec 3.1, one of the 64 test files |
| `engine/tests/test_selfplay.cpp` | header | 15182 | `a50105dc915b428a0bfe83de65b37fc961e6c2b2ae7489725dfb898ad3df0499` | spec 3.1, one of the 64 test files |
| `engine/tests/test_shared_stores.cpp` | header | 6616 | `441a85726fb3f177c307fa567fc038899317f13cdb62c228d1485d43f18f039d` | spec 3.1, one of the 64 test files |
| `engine/tests/test_sixbpp.cpp` | header | 19850 | `6b454695417928c3572344adcb192327782023def3fe9f18d32f3ce92d1b613a` | spec 3.1, one of the 64 test files |
| `engine/tests/test_special_moves.cpp` | header | 21624 | `02ff9b937e1dce37c80b1012889a63cf2bb059b5a4326835871f38b7712742f1` | spec 3.1, one of the 64 test files |
| `engine/tests/test_state.cpp` | header | 27245 | `0414982deafab9753c4b647a4dee4ae322f3b7230d275069a3e65af3973b0876` | spec 3.1, one of the 64 test files |
| `engine/tests/test_support.hpp` | header | 1899 | `f9307700f120e9333233c7d25759645921a86034a94a8a1f1b226554e3a68ba3` | spec 3.1, one of the 64 test files |
| `engine/tests/test_synthetic.cpp` | header | 12606 | `70ac97198291e00e85c663832f9c6ee018b6c29244175a500bd498cc4583382e` | spec 3.1, one of the 64 test files |
| `engine/tests/test_timeline.cpp` | header | 4536 | `1df2e1946817488f7ca895db4f4ef65acf533aba4ce106adb2b48cefe0228e19` | spec 3.1, one of the 64 test files |
| `engine/tests/test_tiny.hpp` | header | 5041 | `fa7ca73f046d31ecbaa061edabaf104e1f2b712afee348a154328539f8531f57` | spec 3.1, one of the 64 test files |
| `engine/tests/test_turn_state.cpp` | header | 19248 | `b7c1feaefb3a2c4b73e6f1e649a6e9e8e4758aab018b3f16525cc329165645b0` | spec 3.1, one of the 64 test files |
| `engine/tests/test_turngen.cpp` | header | 33239 | `3744174cc380167eedfe81e5ae5de6ed7b82a38dc5f6beb449e94faf411a825e` | spec 3.1, one of the 64 test files |
| `engine/tests/test_turnwidth.cpp` | header | 46456 | `b5023ae0e0be4ac9fec727305ae223bd3cfbddd7f9a02b9d5cd3d3b3c440ff08` | spec 3.1, one of the 64 test files |
| `engine/tests/test_types.cpp` | header | 6853 | `713cded465c4a605c4208eec8b14f8716508db239974298249756bad368a9689` | spec 3.1, one of the 64 test files |
| `engine/tests/test_v3_support.hpp` | header | 4141 | `c6107e404942e9c43d1954b7c57266568e7e676a864ea0db5e91d9ebfada7b22` | spec 3.1, one of the 64 test files |
| `engine/tests/test_variants.cpp` | header | 12011 | `45dba1d8c35f8bb7081c350d94b977c7189916c6923911ba8e650950318fb9ea` | spec 3.1, one of the 64 test files |
| `engine/tests/test_views.cpp` | header | 33646 | `acb705420572779ce4afcd12bc14cf416419f515cf8aa77c7a956659d7eb00e7` | spec 3.1, one of the 64 test files |
| `engine/tests/test_zobrist.cpp` | header | 18040 | `20579872a6768fbb050c56ccefb14a15167bde5daeac12fc77e9b734452512df` | spec 3.1, one of the 64 test files |
| `engine/tools/synthetic.cpp` | header | 7653 | `f7ab04a5b085d01b1d35cdfe46748b7004fd8d41bf3c2144b45a36578520af79` | needed to build the copied test_synthetic.cpp |
| `engine/tools/synthetic.hpp` | header | 6244 | `f91664a0054d5253177fdf33cb710154a92504f6f735fdb204616596aa1a8397` | needed to build the copied test_synthetic.cpp |
| `engine/tools/t5d_gate.cpp` | header | 7334 | `ef08195c94ba26c31511dae105c0d39897f027aa7571ce0c2daa3fc2fb39513e` | main() of the gate adapter t5d-gate, run by the gate and by test_gate_exe.cpp |
| `engine/tools/tool_support.cpp` | header | 5834 | `09e579da3d45bb1d5db4516b8c4881e9e4163293a3a2ec0fae17976d05c20aba` | needed to build the copied test_synthetic.cpp |
| `engine/tools/tool_support.hpp` | header | 3713 | `3b504a7405412ae810cbe66ddb7de96ce24b975d79141071a1f0d8c825a3c6ed` | needed to build the copied test_synthetic.cpp |
| `engine/tools/turn_width.cpp` | header | 9258 | `ded4dd2b8bb9be04a3d81d51ce7e82b4af8b91c313d5d0b824382a15cab5b977` | needed to build the copied test_turnwidth.cpp |
| `engine/tools/turn_width.hpp` | header | 15856 | `f5527069c85f6e4bb89a143af85b6e4dc3cfb94f2fd259ea77b7ad73bdcec720` | needed to build the copied test_turnwidth.cpp |
| `oracles/DISAGREEMENTS.md` | record only | 24839 | `66d2ee6cec0e75a05ac00d8271f772fd94f27daea3f6ebdaa6d0a9cac214d571` | spec 3.1, the differential harness; pinned by the harness re-renders it from the golden files and compares (gate.py) |
| `oracles/README.md` | header | 86693 | `90dc3d45b82639be69252fdfbc61582354d1d0999ad3e1b4fa246e56330354ba` | spec 3.1, the differential harness |
| `oracles/corpus/manifest.toml` | header | 18321 | `9c1545e818a6c8d61a200768aa5f4227055f476c09b7feea725e46f8d6bb9642` | spec 3.1, the differential harness |
| `oracles/corpus/records/checkmate.5dpgn` | record only | 68 | `9a7d42471708746d7fec1db62e1cc26265ce8190a738a13ac82a86855d45db17` | spec 3.1, the differential harness; pinned by its SHA-256 and size in oracles/corpus/manifest.toml |
| `oracles/corpus/records/ignores-check.5dpgn` | record only | 55 | `746b727931cbc7ba9e96c9796672ff38b5e67ff400e5ceaae62cc86e4072f0f3` | spec 3.1, the differential harness; pinned by its SHA-256 and size in oracles/corpus/manifest.toml |
| `oracles/corpus/records/stalemate-with-a-past.5dpgn` | record only | 95 | `0fe871eb677304b7094bd69ce243a8d1d46175d9cf0a47251c34a0c05dcbc41a` | spec 3.1, the differential harness; pinned by its SHA-256 and size in oracles/corpus/manifest.toml |
| `oracles/corpus/records/stalemate.5dpgn` | record only | 67 | `ae8bec1ab964d81d27d3e5092ac6b93b681c54cdba23dd72c72cc4867751096d` | spec 3.1, the differential harness; pinned by its SHA-256 and size in oracles/corpus/manifest.toml |
| `oracles/drivers/chessin5d/driver.js` | record only | 33177 | `8a2991563f582ec7aa722a1e63f6276a2316e761c14acc39c2c6b93109806550` | spec 3.1, the differential harness; pinned by the chessin5d golden signature's driver_sha256 |
| `oracles/drivers/cwmtt/Driver.hs` | record only | 20253 | `cfa3ca067595f572d8c2547c8cf3f4e95307531955459e40e26b5b6b56d40fa3` | spec 3.1, the differential harness; pinned by the cwmtt golden signatures' driver_sha256 and the build key |
| `oracles/drivers/ftxi/CMakeLists.txt` | record only | 2066 | `dba84a0ff85aaa692f49e78353218117ba6041d4a94e1abc56f5699b2c0f9a28` | spec 3.1, the differential harness; pinned by the ftxi golden signatures' driver_sha256 and the build key |
| `oracles/drivers/ftxi/ftxi_driver.cpp` | record only | 25669 | `33ab3708a1c210cf5243e29a6c1a6f5f18d0284d5d35e5a2458718059b73114b` | spec 3.1, the differential harness; pinned by the ftxi golden signatures' driver_sha256 and the build key |
| `oracles/drivers/kairos/Cargo.lock` | record only | 241 | `215b0b3d2c55409ef18361bad9fcde91912ca2476e398f085c8f8927f41e7233` | spec 3.1, the differential harness; pinned by the pending Kairos source's signature and build key (generated by Cargo) |
| `oracles/drivers/kairos/Cargo.toml` | record only | 1081 | `70d1b72c63012db99e486022bb4fce623c630b856fef91e9942241be8bfdac74` | spec 3.1, the differential harness; pinned by the pending Kairos source's signature and build key |
| `oracles/drivers/kairos/src/main.rs` | record only | 16018 | `bbd712de30e2f929ded8353f004befe932b07a27a434176ff6bad8db2da1482a` | spec 3.1, the differential harness; pinned by the pending Kairos source's signature and build key |
| `oracles/drivers/notation/driver.js` | record only | 27841 | `4b8961751869c786821191f3d395246c1e5c562580ec13970500344a3fe2fdba` | spec 3.1, the differential harness; pinned by the notation golden signature's driver_sha256 |
| `oracles/engine-baseline.json` | record only | 5938 | `ad689cf3be80b7cddf9b0e1b5b264edbd6cae875823db55120eebaaf9b3f06e0` | spec 3.1, the differential harness; pinned by the SHA-256 in oracles/gate.toml [baseline]; JSON has no comments |
| `oracles/gate.toml` | header | 15233 | `f042b9ccb6f99be953f7ecb9c7ecb24272b3d05f813579425a96569d4477ec18` | spec 3.1, the differential harness |
| `oracles/golden/MANIFEST` | record only | 1071 | `b0a605fba5973b7c54dc566c772d62aa4cdae3137cbd931d7f48e04c818929ec` | spec 3.1, the differential harness; pinned by its strict format, checked by the harness (integrity.py) |
| `oracles/golden/calibration.json` | record only | 470 | `d1ce63ddaa80d389f0f55a1c92493eb3859f342717c8586632782e700f64c671` | spec 3.1, the differential harness; pinned by its SHA-256 in oracles/golden/MANIFEST; JSON has no comments |
| `oracles/golden/chessin5d.cost.json` | record only | 16988 | `f8b27cc866d3cc0bada13c21b644252323d147164a29d75da7e80580b8c87cbb` | spec 3.1, the differential harness; pinned by its SHA-256 in oracles/golden/MANIFEST; JSON has no comments |
| `oracles/golden/chessin5d.json` | record only | 121200 | `c7cf83f3d0fc85db961313e97e3160faa21e3eadbc66724508d8df6a96d1070b` | spec 3.1, the differential harness; pinned by its SHA-256 in oracles/golden/MANIFEST; JSON has no comments |
| `oracles/golden/cwmtt-naive.cost.json` | record only | 8001 | `40024577a7096dc7c95a44b2d955af0984040bad4ddb84ae67643e98e7bcb2dd` | spec 3.1, the differential harness; pinned by its SHA-256 in oracles/golden/MANIFEST; JSON has no comments |
| `oracles/golden/cwmtt-naive.json` | record only | 92704 | `ac4d713fdd9eb07d2fc3243666a860fd65395d5217110199243e63c39db13ba9` | spec 3.1, the differential harness; pinned by its SHA-256 in oracles/golden/MANIFEST; JSON has no comments |
| `oracles/golden/cwmtt.cost.json` | record only | 9029 | `2a70252fe91bba6af4b25bb04574acdf5646beedcf0ef85791193b8075cfbf4c` | spec 3.1, the differential harness; pinned by its SHA-256 in oracles/golden/MANIFEST; JSON has no comments |
| `oracles/golden/cwmtt.json` | record only | 113776 | `fe0467484a687b669ffb5203e51c3149d12210b4a6b0d127308ab0cc32483ea3` | spec 3.1, the differential harness; pinned by its SHA-256 in oracles/golden/MANIFEST; JSON has no comments |
| `oracles/golden/ftxi-naive.cost.json` | record only | 9540 | `2a06d12209af79791cb86dc2a9db2420b3fbf99ee4a17dd28e9f3bc58cf67a96` | spec 3.1, the differential harness; pinned by its SHA-256 in oracles/golden/MANIFEST; JSON has no comments |
| `oracles/golden/ftxi-naive.json` | record only | 115089 | `8aceb67809e0c4c1731c37255104108709591bbef630923f7ada5c180b9379d9` | spec 3.1, the differential harness; pinned by its SHA-256 in oracles/golden/MANIFEST; JSON has no comments |
| `oracles/golden/ftxi.cost.json` | record only | 10453 | `fb47984f8f7573b28630b196c4d0a1997d5340442f5b7ad44f6713f2c0a1c3e5` | spec 3.1, the differential harness; pinned by its SHA-256 in oracles/golden/MANIFEST; JSON has no comments |
| `oracles/golden/ftxi.json` | record only | 130378 | `a93908055d25cad2acd33e2e3371b526e465ecaae968fecf4946912fb842af56` | spec 3.1, the differential harness; pinned by its SHA-256 in oracles/golden/MANIFEST; JSON has no comments |
| `oracles/golden/notation.cost.json` | record only | 8523 | `9a90ae9c7232f1462375831340a0eb5fb170970161bef19f2f1eb8202e7ccdf9` | spec 3.1, the differential harness; pinned by its SHA-256 in oracles/golden/MANIFEST; JSON has no comments |
| `oracles/golden/notation.json` | record only | 74369 | `34b78023e7ad85cf20d28d50c16668dea10b97a8dda6e654d6c2716db26f0259` | spec 3.1, the differential harness; pinned by its SHA-256 in oracles/golden/MANIFEST; JSON has no comments |
| `oracles/harness.py` | header | 3574 | `8c49cc96a679a7a53e4d639b411b9d56daa06f85ce93d1679cb879f3869a94fc` | spec 3.1, the differential harness |
| `oracles/known-divergences.toml` | header | 316563 | `6be4a27424c049749d2c026412a620a17bd01461b9a54f4005589ad5c4dc4e48` | spec 3.1, the differential harness |
| `oracles/oracle_harness/__init__.py` | header | 387 | `a671608b1f6eaf8709b5b2f8892bae3ed7eb358c6312313e0b08fc60d1e447ed` | spec 3.1, the differential harness |
| `oracles/oracle_harness/__main__.py` | header | 52 | `13a1a5b340cdcfc1902b62be90e508c7c71886000d5bf087e7854aadf09fb35e` | spec 3.1, the differential harness |
| `oracles/oracle_harness/adapters/__init__.py` | header | 3153 | `bbf06f6958ed0babc1ef5cfa5df78a23666b49b12ae8d881f1df651db60fec2a` | spec 3.1, the differential harness |
| `oracles/oracle_harness/adapters/base.py` | record only | 20645 | `3487bd57b7675ed03654f9da8226fb37dcd3d803f15491bc3498a14ac338103b` | spec 3.1, the differential harness; pinned by every golden signature's adapter_sha256 |
| `oracles/oracle_harness/adapters/chessin5d.py` | record only | 8008 | `41de9ddcb1b83c34be2625ec18641dbc3b3d1d9293fd6c3af23f3db388713470` | spec 3.1, the differential harness; pinned by the chessin5d golden signature's adapter_sha256 |
| `oracles/oracle_harness/adapters/cwmtt.py` | record only | 28747 | `58ce1e478f639e86ebdaa062a56d208d9e86871e3454a475e2a31f874da521aa` | spec 3.1, the differential harness; pinned by the cwmtt golden signatures' adapter_sha256 |
| `oracles/oracle_harness/adapters/engine.py` | header | 6589 | `82965e32b64f52bf9be92897d0357446ed31218efad4240f05ba7594d038893c` | spec 3.1, the differential harness |
| `oracles/oracle_harness/adapters/ftxi.py` | record only | 5821 | `5771c7c32ec0a5a2c4748b4e3e0edd90ddf5e14ca202078625b5396404aaab39` | spec 3.1, the differential harness; pinned by the ftxi golden signatures' adapter_sha256 |
| `oracles/oracle_harness/adapters/kairos.py` | record only | 4887 | `44059e7ab6681ae35cfbdd3a9243f7ed3d88805984a644a52044d7c6251d663a` | spec 3.1, the differential harness; pinned by the pending Kairos source's signature |
| `oracles/oracle_harness/adapters/notation.py` | record only | 5826 | `5b69d1b938038a92e1ebb9df8289b744ab6853c0929cb01e269aa2ef27c919cc` | spec 3.1, the differential harness; pinned by the notation golden signature's adapter_sha256 |
| `oracles/oracle_harness/baseline.py` | header | 21846 | `5d2009cc2a1bccc3e834bd76be0d5e20f9adf61bb3e84faf9a85ce5850575644` | spec 3.1, the differential harness |
| `oracles/oracle_harness/boundary.py` | header | 38370 | `5a920ac7842a486fb428b48fb06efc46353a641a4d3f8a42aad08aae63ee8345` | spec 3.1, the differential harness |
| `oracles/oracle_harness/build.py` | header | 15314 | `bc627969b0f054d98feb91945067d8cd8953019620d3da299eaf1b3573d70c66` | spec 3.1, the differential harness |
| `oracles/oracle_harness/cli.py` | header | 18357 | `8cdea284964ff28eb3ff014413733786d81e7fdc0e101d78b5588f783890c440` | spec 3.1, the differential harness |
| `oracles/oracle_harness/config.py` | header | 16580 | `1b3374c2cc2cfb97502908b44f25a41f84c782e1d4f25048730362867c2e9a0f` | spec 3.1, the differential harness |
| `oracles/oracle_harness/corpus.py` | header | 18356 | `1a10b3eaaa80e0e8df1d3cbbfd2b20ae7a5a32da92822a8fb54ab67fa2e2b64a` | spec 3.1, the differential harness |
| `oracles/oracle_harness/divergences.py` | header | 19265 | `ab81aa6d3686b1612e60de750d9fab60d6c58d6dda25d71a9cf9acf063e7bd69` | spec 3.1, the differential harness |
| `oracles/oracle_harness/export_form.py` | header | 11170 | `532222f8fae271fb4419a3abc2c585d3b6847b72eb5cabd1de7b1818e3e1a1fa` | spec 3.1, the differential harness |
| `oracles/oracle_harness/fidelity.py` | header | 12119 | `84af1625d00308dd4dfb5748740f64a5e3df8f5e89e55d7b295ab7df364bcd47` | spec 3.1, the differential harness |
| `oracles/oracle_harness/fingerprint.py` | record only | 18233 | `f55fa1b38ed5852dd0583d91f7270a2b17082987377c99b9b3ab2578e2b77c6c` | spec 3.1, the differential harness; pinned by every golden signature's adapter_sha256 |
| `oracles/oracle_harness/gate.py` | header | 81572 | `24eec8a9501900793771800e3bba8707c5fcd47b143f5027f8a6a34285b80aba` | spec 3.1, the differential harness |
| `oracles/oracle_harness/gitutil.py` | header | 5179 | `1e79f393e936fecb99ef778bed22781fc668448be4d0882d0650a825999c8ff6` | spec 3.1, the differential harness |
| `oracles/oracle_harness/golden.py` | header | 43682 | `8c188b5801d54165980f0afb70614ae14e529a2ce2dbb6d26fe95b9fea112419` | spec 3.1, the differential harness |
| `oracles/oracle_harness/headers.py` | record only | 2360 | `6566d73d13350c30e41e87e084cfeee4be02d200a1935e9075ac4fe95a73ee4b` | spec 3.1, the differential harness; pinned by four golden signatures' adapter_sha256 |
| `oracles/oracle_harness/integrity.py` | header | 10845 | `c8aa7b3be350e75592173681d58d3abcc17b096e20a6d27388630e8329c64bcb` | spec 3.1, the differential harness |
| `oracles/oracle_harness/lockfile.py` | header | 11256 | `01b352f075e9f9443548867476b1377aeb7f7ffe2bc43370b6025cfe5a1196d0` | spec 3.1, the differential harness |
| `oracles/oracle_harness/observations.py` | header | 31471 | `04127ff5aa42662fe7d93b17c810c48307ffa98a066158687218532e739fff3f` | spec 3.1, the differential harness |
| `oracles/oracle_harness/paths.py` | header | 1537 | `ab6dba7c28e24355f42064600a8bc4377f041b9944fd1e78ac66f56c9b31e5a9` | spec 3.1, the differential harness |
| `oracles/oracle_harness/provenance.py` | header | 4701 | `e932ce5d958c57470a2b5c8738e6693822c5d51f15b71163fcffdf28a5e08744` | spec 3.1, the differential harness |
| `oracles/oracle_harness/rulebook.py` | header | 3284 | `81f3cab430f809a5daf97abd45b80175b60f69d81ecf57ea0f5e7aacc7d7649f` | spec 3.1, the differential harness |
| `oracles/oracle_harness/runner.py` | header | 4380 | `93e425c38b7337ab4a740350f8357441b7e6ceef090fff60cc66b2c889b57ef1` | spec 3.1, the differential harness |
| `oracles/oracle_harness/snapshot.py` | header | 6455 | `0223e5434a1e65237b7ee939e32cb275ed1c5e194604f2aa51a6964edeca8746` | spec 3.1, the differential harness |
| `oracles/oracle_harness/stability.py` | header | 39885 | `2954c08c6d56540ee7b8eedf593af686a0c3ebe49d89d6fe660daa6554e20aa7` | spec 3.1, the differential harness |
| `oracles/oracle_harness/toolchain.py` | header | 18337 | `7b2b56f1afed6d3b31ceac76c027cc694572f0abdcd7c0a831fe5bc47cf6a2a0` | spec 3.1, the differential harness |
| `oracles/oracle_harness/util.py` | header | 11230 | `160fa41007f6c7e56b7bd99c19e6f6bb4b850ba02677cb1370248bf487d343ca` | spec 3.1, the differential harness |
| `oracles/oracles.lock.toml` | header | 19009 | `f5d76d3758221fafceb86ddbe0edc97b609b4282845088f6f23cb64ec383cae0` | spec 3.1, the differential harness |
| `oracles/pyproject.toml` | header | 703 | `aedd0e7b3f839f6fca979d9cd24150e38675498ab8b0816fb8223eb0aa03d82f` | spec 3.1, the differential harness |
| `oracles/tests/support.py` | header | 8274 | `48cb2969b221ecc05ae99ade7dc5bb3e04a92b41c5f0a55a3492505b625c37b6` | spec 3.1, the differential harness |
| `oracles/tests/test_baseline.py` | header | 16157 | `0b7086ce6bbc0039cdde575bcc7dfdea2128554a0779b8eb39cecda608dc971d` | spec 3.1, the differential harness |
| `oracles/tests/test_boundary.py` | header | 38552 | `337b0ce7748a3c44454334b57f754ab4982f8afe666366563e5d923eaaf5854d` | spec 3.1, the differential harness |
| `oracles/tests/test_build.py` | header | 4358 | `46d3251e93d6cc667c1363bb33b225ff625f7ccfcc0970da6c7bd76339da2060` | spec 3.1, the differential harness |
| `oracles/tests/test_config.py` | header | 10215 | `bdbbc3d7b1a3efe37d15bcb4d61f52e3f6427144009348f53358a5fb1f409ed3` | spec 3.1, the differential harness |
| `oracles/tests/test_corpus.py` | header | 12582 | `ef4aa6e4b0e76a011cd15bdd3814eb3096c9377408de88a57af83e0c6f1648fd` | spec 3.1, the differential harness |
| `oracles/tests/test_cwmtt_merge.py` | header | 18355 | `bbd918c3d4780d20af0de0a01d6ee9556f38d2d45a3644296a7f738db06280cb` | spec 3.1, the differential harness |
| `oracles/tests/test_divergences.py` | header | 30707 | `226b0b732d75e91b8edf21c5fc5d20f74a372919b4b5058aac3087c7d3b5f18e` | spec 3.1, the differential harness |
| `oracles/tests/test_export_form.py` | header | 23016 | `23c2aef8b1875302920b361458e9013a8687b15e9b9d97a74849002354c0378e` | spec 3.1, the differential harness |
| `oracles/tests/test_fidelity.py` | header | 11643 | `b6f5b9554fdd902f4c8b2ea4a88e0e6fd7785f84fa3e85c757d1e03909ae51ce` | spec 3.1, the differential harness |
| `oracles/tests/test_fingerprint.py` | header | 15904 | `41ac211f87f58f3717021cf7f98af0f053e43b7a1631448b212e168367980f9a` | spec 3.1, the differential harness |
| `oracles/tests/test_gate.py` | header | 137063 | `9b290a2b6520607e818189ec6c18492313a220c1d91ce6767127d056d3b77a2a` | spec 3.1, the differential harness |
| `oracles/tests/test_gate_live.py` | header | 22185 | `b857f1416ad401952ad542c5099c15e014ce52e6e202b3c1b52b399c44b51835` | spec 3.1, the differential harness |
| `oracles/tests/test_golden.py` | header | 28604 | `24773050577fe106e40dfeaa581654b8895d4b0df69b1521c40f47a91755b4eb` | spec 3.1, the differential harness |
| `oracles/tests/test_integrity.py` | header | 12958 | `cfccd78158e5e2e20f7efd3de1591511e8b329867c513ee72eab2c081dc354a5` | spec 3.1, the differential harness |
| `oracles/tests/test_kairos.py` | header | 10458 | `190dd35bbc6c6c08a58e177596235092dc6d1c6b5ed379bbfff77835ea53f0b5` | spec 3.1, the differential harness |
| `oracles/tests/test_lockfile.py` | header | 6149 | `d79b8ea9dd325f3ef099cb87f91e83083c772853cb1efe1245f0f56ff97f6c8c` | spec 3.1, the differential harness |
| `oracles/tests/test_notation.py` | header | 17561 | `8d7349efdb7df425aace876e7702e4d6c9655053ded76b4cded0b267398567f4` | spec 3.1, the differential harness |
| `oracles/tests/test_observations.py` | header | 36128 | `e44c677d3f09f2c9eef84250fe97e2ed48ab5277b03bc2f4636222b2dce5b69a` | spec 3.1, the differential harness |
| `oracles/tests/test_protocol.py` | header | 52577 | `ec9103d0362ad05544e29f3f9d1f4c93af0a2093767459b611676e3212c15b3a` | spec 3.1, the differential harness |
| `oracles/tests/test_provenance.py` | header | 2217 | `15990964ca812ad82313b88a7e303e055e8795f5387e001f6fbd3a32da66e49f` | spec 3.1, the differential harness |
| `oracles/tests/test_repository.py` | header | 18618 | `3479b7a5af4cd092cf8f08d087b01bf3c090132c177df4b976692dce97650315` | spec 3.1, the differential harness |
| `oracles/tests/test_rulebook.py` | header | 4317 | `ed09587c92a2f65a360acb36531f7be794049608cadf421a3207926e6d3bfd54` | spec 3.1, the differential harness |
| `oracles/tests/test_stability.py` | header | 34304 | `b83d67c4e9e94956f369da3a4ca634a63a9f1c5e5b466e7da88d8bf0dac5fc71` | spec 3.1, the differential harness |
| `oracles/tests/test_toolchain.py` | header | 10316 | `6b18acb9f58be284572c1e9a360ae58d2693549b868bfe67c86536b59c2e700c` | spec 3.1, the differential harness |
| `oracles/tests/test_util.py` | header | 4638 | `4c806d9363b18cff590dcd94ca9fe4d265f122af564f9b7b5daf8d11630a3d7c` | spec 3.1, the differential harness |

## Oracle harness inventory

The copied harness checks that this block matches its lock file and corpus manifest. It is generated; don't edit it.

<!-- BEGIN GENERATED INVENTORY - python oracles/harness.py provenance --write -->

### Pinned repositories

| Id | Upstream | Pinned commit | Licence | Role | CI tier |
|---|---|---|---|---|---|
| `ftxi` | [ftxi/5dchess_engine](https://github.com/ftxi/5dchess_engine) | `2ea9f1b82880f08faa9400d1ac20b20102604187` (v0.4.10, 2026-09-15) | BSD-2-Clause | oracle | commit |
| `cwmtt` | [penteract/cwmtt](https://github.com/penteract/cwmtt) | `9632e9969dd455c6e3d6c27c57c1936c251d7e97` (master, 2026-05-18) | GPL-3.0 (LICENSE file); cwmtt.cabal declares BSD3 - treated as GPL-3.0 until upstream resolves the conflict | oracle | nightly |
| `chessin5d` | [5d-chess/5d-chess-js](https://gitlab.com/5d-chess/5d-chess-js.git) | `ee403c80e8c132fc4d168f5b99ddd9b8fe5cc17b` (master, 2022-06-15) | AGPL-3.0-or-later | oracle | nightly |
| `kairos` | [FirePlank/5d-chess-engine](https://github.com/FirePlank/5d-chess-engine) | `8bfea4a6e8d1cec2cc27cafa387b1bd78c0c94e0` (main, 2026-10-23) | GPL-3.0-only (LICENSE file: GitHub's GPL-3.0 template text; Cargo.toml: GPL-3.0-only); NOTICE: parts derived from ftxi/5dchess_engine under BSD-2-Clause | oracle | nightly |
| `notation` | [adri326/5dchess-notation](https://github.com/adri326/5dchess-notation) | `99f6163a9c95a6a9740b20fb5f3c7f6503a166a0` (master (v1.4.3-13), 2021-07-25) | X11 (MIT-style, with a no-advertising clause) | parse-oracle | commit |

#### `ftxi` - ftxi/5dchess_engine

- **What it is:** C++23 bitboard rules engine: move generation, check detection, hypercuboid and naive action searches
- **How it is used:** Built from an export of the pinned commit. Queried through oracles/drivers/ftxi (links its core library; part of that driver is adapted from its tools/position_tools.cpp and carries its licence notice) and cross-checked against its stock 5dtools CLI.
- **Where:** `oracles/third_party/ftxi` (submodule `oracle-ftxi`)
- **Licence file:** `LICENSE`, SHA-256 `c36ba7d9587abbf62264a9e096f35b9c9c57805e6a4ae169cfe6e03de5b79e5f`
- Pinned at the latest release tag rather than main: the eight commits after v0.4.10 rework the hypercuboid hot path and have not been released.
- Its own submodules (extern/pybind11, extern/client) are not needed and are never initialised; extern/client is declared with an SSH URL, so never update submodules recursively.
- Its test/pgn/README.txt lists twelve of its sixteen test records as taken from cwmtt, under the GPL-3.0. A thirteenth (100timelines) is cwmtt's 100_timelines again under another name, and is left out of the corpus; the other three are its own. All are read in place through the corpus manifest, never copied.
- At the pin a coordinate packs into 32 bits: 12 for the timeline, 12 for the turn, 4 each for rank and file (src/misc/vec4.h), so timelines and turns each span -2048..2047. The register's 'its 8-bit coordinate packing' (H24) describes an earlier tree.

#### `cwmtt` - penteract/cwmtt

- **What it is:** Haskell rules implementation and the original hypercuboid mate search, with a naive search beside it
- **How it is used:** Built from an export of the pinned commit with the pinned GHC. Its counts, and how far it replays a record, come from its stock CLI (`cwmtt count`), unmodified. Beside it runs oracles/drivers/cwmtt, a state driver of the harness's own compiled against the same modules with the same flags (it calls them and copies nothing), which replays the record and reports, at every position, the boards cwmtt holds and the side to move - the CLI has no command for either; the adapter pairs the two only where they read and replayed the record alike.
- **Where:** `oracles/third_party/cwmtt` (submodule `oracle-cwmtt`)
- **Licence file:** `LICENSE`, SHA-256 `3972dc9744f6499f0f9b2dbf76696f2ae7ad8af9b23dde66d6af86c9dfb36986`
- No z3 in the program at this commit. Upstream removed the z3-based search in 65e1bf9 (2023-03-26) because it was slower than the plain hypercuboid search; the design spec's 'Haskell and z3' describes the older tree. Its README still mentions z3 and its stack.yaml still lists the z3 package, but cwmtt.cabal depends only on base and containers, so GHC alone builds it.
- Its CLI reports whether a legal action exists, not why none does: it cannot tell checkmate from stalemate, and it has no check verdict.
- A Board header it does not know - or that its parser does not reach - is silently replayed on the Standard layout, 5DFEN boards are skipped and a Promotions header is never read; the adapter refuses such records instead of trusting the answer.
- Its parser stops without failing at the first thing it does not read (an annotation glyph, castling, a variation) and replays what it had. When that leaves the last action half read, the adapter reports the record as unsupported from there instead of passing the fragment's fate off as a verdict.

#### `chessin5d` - 5d-chess/5d-chess-js

- **What it is:** JavaScript rules library behind Chess in 5D (chessin5d.net): the most independent of the three implementations
- **How it is used:** Exported at the pinned commit; its three runtime npm dependencies are installed from its own package-lock.json. Queried through oracles/drivers/chessin5d, in a separate Node process per record.
- **Where:** `oracles/third_party/chessin5d` (submodule `oracle-chessin5d`)
- **Licence file:** `LICENSE`, SHA-256 `6f551a3f45c2c2235ec9b9e2d93c5896babab36caed027781b3e90df90876097`
- Chess in 5D's client (gitlab.com/5d-chess/chess-in-5d, AGPL-3.0) takes its rules from this library (dependency 5d-chess-js ^1.2.1). Only the rules library is pinned; the client adds no rules.
- Runtime npm dependencies, all MIT: module-alias 2.2.2, blueimp-md5 2.18.0, present 1.0.0.
- Defects at the pin, and what the driver does about each without touching rules logic: inCheckmate and inStalemate answer true when their search runs out of time (the driver calls the search itself and reads its timed-out flag); that search can exhaust memory on wide positions (it runs in a worker with its own heap limit); a Board header the library does not know, or that its header reader does not match, is silently replayed on the Standard layout, and 5DFEN boards are ignored unless Board is custom (the driver asks the library which variant it set up and refuses the record when that is not the one declared); a PGN result token at the end of the move text is read as moves (it is dropped); chess.actions() throws in its 'object' and 'json' formats (a ReferenceError) and in '5dpgn' (a TypeError), and in every format passes its promotion pieces where boardFuncs.moves() takes its 'spatial moves only' switch, so it would list no move through time or across timelines (not used: the driver's observe --actions diagnostic calls actionFuncs.actions() itself, with that switch off).
- Its action enumeration yields ordered move sequences under its own options, so it contributes check and mate verdicts and move legality, not action counts.

#### `kairos` - FirePlank/5d-chess-engine

- **What it is:** Kairos: a Rust 5D chess engine - rules up to 8x8 with the variant pieces, a hypercuboid legal-action search with an exhaustive one beside it, 5dpgn/5DFEN input, an alpha-beta player over 5DUCI (decision H52)
- **How it is used:** Built from an export of the pinned commit with the pinned Rust toolchain. Queried through oracles/drivers/kairos, a Rust program of the harness's own linked against the pinned crate as a library (it calls its 5dpgn reader, its replay, its check test, its mate test and its two action searches, and copies none of them), in one replay of the record. Its programs ask the same calls another way: the kairos binary plays over 5DUCI (position, go, print - no verdict or count); its developer tool ktools answers per position (perft: the hypercuboid count; naive: both searches' counts; rules: the boards, the check and the mate type) from position files of its own (G lines), which its pgn2g command writes from a 5dpgn file; and its JSON command interface (web::api) is a library API that only the WebAssembly build of its site calls. Those are the stock tools a driver check would hold the driver to (not built yet).
- **Where:** `oracles/third_party/kairos` (submodule `oracle-kairos`)
- **Licence file:** `LICENSE`, SHA-256 `589ed823e9a84c56feb95ac58e7cf384626b9cbf4fda2a907bc36e103de1bad2`
- Pinned at the head of main: the repository has no tags and no releases. It was created on 2026-10-15 and had 354 commits by the pin (2026-10-23), most days several; a pin bump will move answers, and is reviewed as any other.
- Not independent of ftxi in all it does. Its NOTICE says parts of its rules and legal-action code are derived from ftxi/5dchess_engine (BSD-2-Clause), its src/core and src/misc mirror ftxi's module for module (check_position, hypercuboid, geometry, integer_set, graph, ordering, vec4 - coordinates packed 12/12/4/4 bits, as ftxi's), and its 5dpgn reader says it is 'ported from the C++ engine's pgnparser'. Where it departs from ftxi the departures are its own: its check test advances only the active timelines whose last board is in the present (State::has_phantom_check through CheckPosition::for_present), where ftxi advances every timeline the mover may play on, and its hypercuboid search leaves out actions that waste a move (below). How far it counts as an implementation independent of ftxi for the two-oracle bar (H47) is the user's to rule before its tier entries are enabled (docs/BUILD_LOG.md, issue 526).
- Its hypercuboid search does not generate an action that both branches and plays a 'wasted' move - a non-branching move whose boards all stay out of the present, other than boards a branch lands on - because dropping that move leaves an equally legal action (hypercuboid.rs, wasted_problem). Its perft therefore counts those actions out: its own test gives 31406 at depth 3 from the Standard start, 'the 1780 actions that waste a move (33186 with them)' left out. That is not the gate's count, so the hypercuboid source answers no count. Its exhaustive enumerator (core/naive.rs, naive_actions) lists every action - its non-branching moves once, in timeline order, and its branching moves in every order - the gate's definition; source kairos:naive counts with it.
- Its 5dpgn reader is all or nothing: one action of the main line or of any variation that it cannot match, or cannot submit, fails the whole record, with no position read. Such a record is reported unsupported from position 0 (reason parse_failed), never refused at action 0. The reader does not test royal safety; the driver replays the main line through its own referee (web::tree::play_action, which its game loader runs), and an action that leaves a royal capturable is refused there (cannot_submit, as the other oracles report it). That is the only refusal it can give: a move it cannot play, or an action that does not move the present, fails the record in the reader (parse_failed), so its legality answer covers royal safety alone.
- A line that ends in variations is replayed with its last variation as the main line (as ftxi's reader does); the adapter refuses such a record, with ftxi's guard. It reads the Board header (else Variant), Size, Timeline and Promotions; it refuses a Board it does not know rather than set up Standard, and 44 named variants, whose set-ups it takes from adri326/5dchess-notation's table. Boards are 8x8 at most.
- Its .cargo/config.toml builds with -C target-cpu=native. The harness builds the driver outside the exported tree, where that file does not apply, so the build is the same on every x86-64 machine.

#### `notation` - adri326/5dchess-notation

- **What it is:** The 5dpgn / 5dfen notation specification, with sample game records and a parser written beside it
- **How it is used:** The pinned text of the notation the corpus is written in, and a source of corpus records (test/*.5dpgn). Its bundled parser (parsers/pgn.js and parsers/game.js, which need no package) is exported at the pinned commit and run as a parse-only oracle through oracles/drivers/notation, in a separate Node process per record: at every position, the boards it holds and the side it reads as moving next, and how many actions it read - never a rules verdict.
- **Where:** `oracles/third_party/notation` (submodule `corpus-notation`)
- **Licence file:** `LICENSE`, SHA-256 `8d4c0ecb12b11ec195afed72693869a24cea48e8b160d2cc4b8f03d08f42a0ae`
- Not in the design spec's table of rules oracles: pinned so the corpus has real game records that do not come from an oracle's own test suite, and so that the engine's parser is written from the specification (decision H28). Its bundled parser is a parse-only oracle (H26), and as off limits to the engine as everything else under third_party.
- The parser applies each move as written. It has no check, legality or end-of-game logic, and finds the moving piece with a movement test of its own only where the move text leaves the origin square out. So its acceptance of an action is no verdict - it accepts an action that leaves a king in check - and a move it cannot apply is a soft rejection.
- Defects at the pin, and what the harness does about each without touching the parser: it reports an error by printing it to stdout and calling process.exit(1) (the driver captures what it prints and turns the exit into an exception); a Board name it does not know gives one silently empty board, and a named layout is set up on boards of the Size header's size, or 8x8, never its own (both refused, by the adapter and again by the driver); it reads 5DFEN board strings only under Board "custom" (refused under any other); its tokeniser loops forever on a comment that is never closed (refused before it runs); it stores a 5DFEN "-0" board as a signed zero, which it writes back as "0" (left alone: a set-up that mixes it with "+0" is refused under a reason of its own, signed_zero, so that the recorded outcome names the defect; any other shows in the fingerprints); it drops the unmoved marker "*" (its state events say markers false); it splits a long-algebraic pawn move such as e2e4 into two moves (it then finds no pawn for the first, a soft rejection).
- X11 licence: the export the driver loads is the whole pinned tree, LICENSE included, in the harness cache, never committed or distributed; the project never uses the author's name to promote itself.

### Toolchains

| Tool | Required | Why |
|---|---|---|
| ghc | exactly 9.10.3; checksum-pinned archive for windows-x86_64 | The compiler of Stackage lts-24.40, the resolver cwmtt's stack.yaml names at the pin. cwmtt needs only GHC's boot packages, so no package snapshot is fetched. |
| cmake | 3.20 or newer; `--bootstrap` installs cmake==4.4.3 | Configures ftxi's own CMake project and the harness driver built against it. |
| ninja | any; `--bootstrap` installs ninja==1.13.2 | CMake generator: required on Windows, used elsewhere when present. |
| cxx | g++ 13 or newer | ftxi/5dchess_engine is C++23. Its own CI also builds with Apple Clang and MSVC 14.40; the harness looks only for g++ (set CXX to point at one). |
| rust | exactly 1.99.0; checksum-pinned archive for linux-x86_64, windows-x86_64 | Builds the Kairos oracle's crate (edition 2024: Rust 1.85 or newer) and the harness's driver linked against it; the stable release of 2026-10-01, the newest at the pin. Kairos has no crate dependencies, so nothing is fetched from crates.io (the build runs --locked --offline). On Windows the GNU target, whose self-contained MinGW linker ships with the toolchain (rust-mingw) - no Visual Studio; on Linux rustc links with the system's cc. |
| node | 18 or newer | Runs the Chess in 5D library, whose three pinned runtime dependencies npm installs from its own lockfile, and the notation repository's parser, which needs none. |

### Corpus records

41 records, listed one by one with blob ids and hashes in `oracles/corpus/manifest.toml`.

| Source | Records | Licence they are used under |
|---|---|---|
| `cwmtt` (read in place at the pinned commit, not copied) | 21 | GPL-3.0, as part of penteract/cwmtt |
| `ftxi` (read in place at the pinned commit, not copied) | 5 | BSD-2-Clause, as part of ftxi/5dchess_engine; GPL-3.0: adapted from cwmtt (ftxi test/pgn/README.txt) |
| `notation` (read in place at the pinned commit, not copied) | 11 | X11, as part of adri326/5dchess-notation |
| this repository | 4 | written for this project; no third-party content |

<!-- END GENERATED INVENTORY -->

// Fractal provenance: copied from Temporal-5D (https://github.com/Silicon-Spider/temporal-5d)
//   source: engine/src/fingerprint.cpp
//   commit: c8a9c490bfde650466609531f8083d69513c6036
//   copied: 2026-11-08 (Fractal spec 3.1 and 3.4; decisions NR1, NR5)
//   state:  unchanged: below this header the file is byte for byte the source
// The canonical state string and its fingerprint (see t5d/fingerprint.hpp).

#include "t5d/fingerprint.hpp"

#include "t5d/fen.hpp"
#include "t5d/md5.hpp"

namespace t5d {

std::string canonical_state_string(const Multiverse& multiverse) {
  std::string out;
  out.reserve(multiverse.board_count() * 84);
  const bool even = multiverse.even();
  // for_each_board visits ascending timeline, then ascending half-turn: the
  // canonical order (white's board before black's within a turn).
  multiverse.for_each_board([&](int p, int h, const Board& board) {
    append_fen_board_string(out, board, p, h, even);
  });
  return out;
}

Fingerprint fingerprint(const Multiverse& multiverse) {
  const std::string canonical = canonical_state_string(multiverse);
  Md5 full;
  full.update(canonical);
  Md5 placement;  // the same string with every '*' removed
  std::size_t start = 0;
  for (std::size_t i = 0; i <= canonical.size(); ++i) {
    if (i == canonical.size() || canonical[i] == '*') {
      placement.update(canonical.data() + start, i - start);
      start = i + 1;
    }
  }
  Fingerprint out;
  out.fingerprint = Md5::hex(full.finish());
  out.placement = Md5::hex(placement.finish());
  out.boards = multiverse.board_count();
  out.pieces = multiverse.piece_count();
  return out;
}

}  // namespace t5d

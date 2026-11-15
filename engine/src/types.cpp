// Fractal provenance: copied from Temporal-5D (https://github.com/Silicon-Spider/temporal-5d)
//   source: engine/src/types.cpp
//   commit: c8a9c490bfde650466609531f8083d69513c6036
//   copied: 2026-11-08 (Fractal spec 3.1 and 3.4; decisions NR1, NR5)
//   state:  unchanged: below this header the file is byte for byte the source
// Squares and board sizes as text.

#include "t5d/types.hpp"

#include <string>

namespace t5d {

std::string square_name(int square) {
  std::string name;
  name += static_cast<char>('a' + file_of(square));
  name += static_cast<char>('1' + rank_of(square));
  return name;
}

std::string to_string(const BoardSize& size) {
  return std::to_string(size.width) + "x" + std::to_string(size.height);
}

std::optional<BoardSize> parse_board_size(std::string_view text) noexcept {
  // "WxH" with 1 <= W, H <= 8: always three characters.
  if (text.size() != 3 || text[1] != 'x') return std::nullopt;
  const char w = text[0];
  const char h = text[2];
  if (w < '1' || w > '8' || h < '1' || h > '8') return std::nullopt;
  return BoardSize{w - '0', h - '0'};
}

}  // namespace t5d

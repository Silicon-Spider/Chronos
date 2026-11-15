// Fractal provenance: copied from Temporal-5D (https://github.com/Silicon-Spider/temporal-5d)
//   source: engine/src/capi_internal.hpp
//   commit: c8a9c490bfde650466609531f8083d69513c6036
//   copied: 2026-11-08 (Fractal spec 3.1 and 3.4; decisions NR1, NR5)
//   state:  changed: capi's transformer entry points removed (spec 3.4 step 2); see PROVENANCE.md
// What the C API's translation units share (src/capi.cpp, src/capi_search.cpp):
// a game handle's state and its count of moves in the action, making a game
// handle, and the description of an expansion in the C layout. Internal: not
// part of the library's interface.

#ifndef T5D_SRC_CAPI_INTERNAL_HPP
#define T5D_SRC_CAPI_INTERNAL_HPP

#include <stdexcept>

#include "t5d/capi.h"
#include "t5d/state.hpp"

namespace t5d::capi_internal {

/// A feature the engine does not implement (T5D_ERR_UNSUPPORTED).
class Unsupported : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// The state a game handle holds.
const GameState& game_state(const t5d_game& game) noexcept;
/// The moves made in the handle's action in progress (register A6; 0 at a turn start).
int game_moves_in_action(const t5d_game& game) noexcept;
/// A new game handle on `state` with `moves_in_action` moves made in its action
/// (legal submissions, no record). Throws std::bad_alloc, or std::invalid_argument
/// for a negative count.
t5d_game* new_game(const GameState& state, int moves_in_action);

}  // namespace t5d::capi_internal

#endif  // T5D_SRC_CAPI_INTERNAL_HPP

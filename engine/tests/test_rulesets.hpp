// Fractal provenance: copied from Temporal-5D (https://github.com/Silicon-Spider/temporal-5d)
//   source: engine/tests/test_rulesets.hpp
//   commit: c8a9c490bfde650466609531f8083d69513c6036
//   copied: 2026-11-08 (Fractal spec 3.1 and 3.4; decisions NR1, NR5)
//   state:  unchanged: below this header the file is byte for byte the source
// The engine's rulesets for the tests: the defaults, and every alternative of
// t5d/rules.hpp, one parameter at a time.

#ifndef T5D_TEST_RULESETS_HPP
#define T5D_TEST_RULESETS_HPP

#include <vector>

#include "t5d/rules.hpp"

namespace t5d_test {

/// Every alternative of the engine's ruleset, one parameter each (not the
/// defaults).
inline std::vector<t5d::RuleOptions> alternative_rulesets() {
  using namespace t5d;
  std::vector<RuleOptions> out;
  const auto with = [&](auto edit) {
    RuleOptions r;
    edit(r);
    out.push_back(r);
  };
  with([](RuleOptions& r) { r.activation = ActivationRule::Consecutive; });
  with([](RuleOptions& r) { r.initial_credit = InitialTimelineCredit::MinusZeroToBlack; });
  with([](RuleOptions& r) { r.play_activates = true; });
  with([](RuleOptions& r) { r.moves_after_shift = !r.moves_after_shift; });  // Q4's other reading
  with([](RuleOptions& r) { r.mandatory = MandatoryBoards::AllActive; });
  with([](RuleOptions& r) { r.reactivation = Reactivation::Deferred; });
  with([](RuleOptions& r) { r.turn_zero_reachable = false; });
  with([](RuleOptions& r) { r.sliders_cross_void = true; });
  with([](RuleOptions& r) { r.past_self_blocks = false; });
  with([](RuleOptions& r) { r.earlier_playable_jump_branches = true; });
  with([](RuleOptions& r) { r.inactive_history_is_destination = false; });
  with([](RuleOptions& r) { r.brawn_rank_timeline_capture = false; });
  with([](RuleOptions& r) { r.double_step_right = DoubleStepRight::HomeRank; });
  with([](RuleOptions& r) { r.timeline_double_step = false; });
  with([](RuleOptions& r) { r.double_step_over_void = true; });
  with([](RuleOptions& r) { r.branch_copies_keep_unmoved = false; });
  with([](RuleOptions& r) { r.jumps_keep_unmoved = true; });
  with([](RuleOptions& r) { r.en_passant_brawns = false; });
  with([](RuleOptions& r) { r.brawns_promote = false; });
  with([](RuleOptions& r) { r.brawn_cross_board_promotion = false; });
  with([](RuleOptions& r) { r.castling_empty_between = false; });
  with([](RuleOptions& r) { r.castling_destination_test = true; });
  with([](RuleOptions& r) { r.castling_rook_path_test = true; });
  with([](RuleOptions& r) { r.castling_any_rank = false; });
  with([](RuleOptions& r) { r.castling_rook_two_away = false; });
  with([](RuleOptions& r) { r.castling_on_optional_boards = false; });
  return out;
}

/// The defaults first, then every alternative.
inline std::vector<t5d::RuleOptions> every_ruleset() {
  std::vector<t5d::RuleOptions> out{t5d::RuleOptions{}};
  for (const t5d::RuleOptions& r : alternative_rulesets()) out.push_back(r);
  return out;
}

}  // namespace t5d_test

#endif  // T5D_TEST_RULESETS_HPP

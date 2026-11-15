// Chronos provenance: copied from Temporal-5D (https://github.com/Silicon-Spider/temporal-5d)
//   source: engine/include/t5d/capi.h
//   commit: c8a9c490bfde650466609531f8083d69513c6036
//   copied: 2026-11-08 (Chronos spec 3.1 and 3.4; decisions NR1, NR5)
//   state:  changed: capi's transformer entry points removed (spec 3.4 step 2); see PROVENANCE.md
/* t5d/capi.h - the rules engine's C API (the t5d shared library).
 *
 * For callers outside C++ - Python's ctypes first. Conventions:
 *
 *  - Handles are opaque: a t5d_state is created by a t5d_state_new_* or
 *    t5d_state_copy function and released with t5d_state_destroy.
 *  - Every function that can fail returns a t5d_status; T5D_OK is 0. No C++
 *    exception ever crosses this boundary.
 *  - Strings are UTF-8 (in practice ASCII), NUL-terminated. A string the
 *    library returns through a char** is allocated by the library and released
 *    by the caller with t5d_free. Where a function takes `char** error`, the
 *    argument may be NULL; otherwise it receives NULL on success and, on
 *    failure, a message to free with t5d_free (or NULL if even that could not
 *    be allocated).
 *  - A state is never modified by any function here, so one state may be read
 *    from several threads at once. A game (t5d_game, below: a state under the
 *    rules with its expansion buffers) is changed by the functions that move,
 *    submit, replay or expand it, and is used from one thread at a time.
 *  - The arrays an expansion hands out (t5d_expansion) belong to the game
 *    handle and are valid until that handle is next changed; copy what must
 *    outlive that.
 *
 * Timelines are integer positions p (adjacent timelines differ by 1; in a game
 * with an even number of starting timelines "-0" is -1 and "+0" is 0) and times
 * are half-turn indices h = 2*T + (0 white to move, 1 black), T as displayed.
 * The engine's README describes both.
 */
#ifndef T5D_CAPI_H
#define T5D_CAPI_H

#include <stddef.h>
#include <stdint.h>

#include "t5d/records.h"

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(T5D_BUILDING_DLL)
#    define T5D_API __declspec(dllexport)
#  elif defined(T5D_USING_DLL)
#    define T5D_API __declspec(dllimport)
#  else
#    define T5D_API
#  endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#  define T5D_API __attribute__((visibility("default")))
#else
#  define T5D_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ---- status codes ------------------------------------------------------- */

typedef int32_t t5d_status;

#define T5D_OK 0
#define T5D_ERR_ARGUMENT 1 /* a NULL pointer, an invalid size, an index out of range, a
                              player to move that cannot be inferred */
#define T5D_ERR_NOTATION 2 /* malformed 5DFEN, or boards that form no multiverse; a 5dpgn
                              record that cannot be read */
#define T5D_ERR_MEMORY 3   /* out of memory */
#define T5D_ERR_INTERNAL 4 /* anything else: a bug */
#define T5D_ERR_UNSUPPORTED 5 /* a game or a feature the engine does not implement: a variant
                                 it does not set up, a relation it does not build */
#define T5D_ERR_ILLEGAL 6     /* a move or a submission the rules refuse - a submission
                                 that leaves a royal piece capturable included (C4, R1) */

/* ---- parity: does the game start with an even number of timelines? ------- */

#define T5D_PARITY_INFER (-1) /* decide from the labels: "-0"/"+0" even, "0" odd */
#define T5D_PARITY_ODD 0
#define T5D_PARITY_EVEN 1

/* ---- the player to move ---------------------------------------------------- */

#define T5D_TO_MOVE_INFER (-1) /* only where the boards settle it; see t5d_state_new_from_fen */
#define T5D_TO_MOVE_WHITE 0
#define T5D_TO_MOVE_BLACK 1

/* ---- types --------------------------------------------------------------- */

typedef struct t5d_state t5d_state;

/* The canonical fingerprint (decision H30): see the engine README. */
typedef struct t5d_fingerprint {
  char fingerprint[33]; /* lower-case hex MD5 of the canonical state string */
  char placement[33];   /* the same with every '*' removed */
  uint64_t boards;      /* number of boards, superseded and head */
  uint64_t pieces;      /* number of pieces on all boards */
} t5d_fingerprint;

typedef struct t5d_info {
  uint64_t boards;
  uint64_t pieces;
  int32_t width;
  int32_t height;
  int32_t even;    /* 1: the game started with an even number of timelines */
  int32_t to_move; /* 0 white, 1 black: as given, or inferred (t5d_state_new_from_fen) */
  int32_t min_timeline;
  int32_t max_timeline;
  int32_t created_by_white; /* timelines created by each player */
  int32_t created_by_black;
  uint32_t ruleset;
} t5d_info;

/* ---- library -------------------------------------------------------------- */

/* The engine version, e.g. "0.1.0". A static string: do not free it. */
T5D_API const char* t5d_version(void);

/* Releases memory the library returned (strings). NULL is ignored. */
T5D_API void t5d_free(void* pointer);

/* The struct layouts this library was built with, for a binding that mirrors them
 * (python/t5d/engine/capi.py checks every one at load, so a binding and a library of
 * different stages refuse each other instead of writing past a buffer): with `field`
 * NULL, sizeof(`type`); otherwise the offset of `field` in `type`. `type` is one of
 * "t5d_fingerprint", "t5d_info", "t5d_move", "t5d_expand_options",
 * "t5d_game_summary", "t5d_expansion"; `field` is a member's name, as declared.
 * T5D_ERR_ARGUMENT for a NULL `type` or `out`, an unknown type or an unknown field.
 * (Stage E7.) */
T5D_API t5d_status t5d_layout(const char* type, const char* field, size_t* out);

/* ---- creating and releasing states ----------------------------------------- */

/* "Standard": the standard board on timeline 0 at T1, white to move. */
T5D_API t5d_status t5d_state_new_standard(t5d_state** out);

/* "Standard - Two Timelines": the standard board on -0 and +0 at T1. */
T5D_API t5d_status t5d_state_new_standard_two_timelines(t5d_state** out);

/* A state from `count` 5DFEN board strings ("[pieces:timeline:turn:colour]").
 * width, height: the board size (the Size header); 0 and 0 to take it from
 *   the first board string.
 * parity: T5D_PARITY_ODD, T5D_PARITY_EVEN or T5D_PARITY_INFER.
 * to_move: the player whose turn it is - T5D_TO_MOVE_WHITE or
 *   T5D_TO_MOVE_BLACK, taken as given - or T5D_TO_MOVE_INFER. 5DFEN does not
 *   record it, and inferring it in general needs the timeline-activity rule,
 *   which a later stage implements; until then INFER succeeds only where the
 *   boards settle it whatever that rule says (every head that could be the
 *   present has the same colour to move - every starting position does) and
 *   fails otherwise with T5D_ERR_ARGUMENT and a message naming two heads that
 *   could be the present. It feeds the Zobrist key and t5d_info.to_move; the
 *   fingerprint does not depend on it.
 * ruleset: the ruleset (variant rules) ID folded into the Zobrist key; 0 for
 *   the standard rules.
 * On failure *out is set to NULL. */
T5D_API t5d_status t5d_state_new_from_fen(const char* const* boards, size_t count,
                                          int32_t width, int32_t height, int32_t parity,
                                          int32_t to_move, uint32_t ruleset, t5d_state** out,
                                          char** error);

/* An independent copy (it shares the immutable boards). */
T5D_API t5d_status t5d_state_copy(const t5d_state* state, t5d_state** out);

/* Releases a state. NULL is ignored. */
T5D_API void t5d_state_destroy(t5d_state* state);

/* ---- reading a state ------------------------------------------------------- */

T5D_API t5d_status t5d_state_info(const t5d_state* state, t5d_info* out);

T5D_API t5d_status t5d_state_fingerprint(const t5d_state* state, t5d_fingerprint* out);

/* The canonical state string the fingerprint hashes. Free with t5d_free. */
T5D_API t5d_status t5d_state_canonical(const t5d_state* state, char** out);

/* The 5D Zobrist key (decision H18). */
T5D_API t5d_status t5d_state_zobrist(const t5d_state* state, uint64_t* out);

/* The number of boards; board indices run from 0 to count - 1 in the canonical
 * order (ascending timeline, then half-turn). */
T5D_API t5d_status t5d_state_board_count(const t5d_state* state, size_t* out);

/* The board at `index`: its timeline position, its half-turn and its canonical
 * 5DFEN board string (free with t5d_free). Any output pointer may be NULL. */
T5D_API t5d_status t5d_state_board(const t5d_state* state, size_t index, int32_t* timeline,
                                   int32_t* half_turn, char** fen);

/* =============================================================================
 * Games (stage E3): a game state under the rules - the player to move, the
 * present, the action in progress - with its expansion (t5d/expand.hpp, the
 * engine README's "Expansion") and the number of moves made in the action in
 * progress (register A6: no state records it - t5d_game_summary.moves_made,
 * t5d_expansion.moves_made). Every game is made at a turn start (count 0).
 * Moves are pseudo-legal; submissions are
 * LEGAL: a submission that leaves a royal piece of the mover's capturable is
 * refused (C4, R1; stage E4), and a game read from a record replays it so.
 *
 * A t5d_game handle is not thread-safe (it holds the expansion's buffers);
 * different handles may be used from different threads at once.
 *
 * Rulesets are passed as RuleOptions' canonical 64-bit encoding
 * (t5d/rules.hpp, RuleOptions::encode): 0 = the standard rules - with moves
 * after the present has shifted, until submission (Q4, ruled R8); bit 3 =
 * Q4's Guide-literal reading, no move once the present has shifted (a
 * diagnostic); the engine README lists every option. Each bit marks a choice
 * that differs from the default.
 * ============================================================================= */

typedef struct t5d_game t5d_game;

#define T5D_RULES_STANDARD UINT64_C(0)
/* Q4's Guide-literal reading: no move once the present has shifted. Bit 3 was
 * named T5D_RULES_MOVES_AFTER_SHIFT, and meant the opposite, until R8 made
 * moves after the shift the default (stage E7). */
#define T5D_RULES_NO_MOVES_AFTER_SHIFT (UINT64_C(1) << 3)
/* Moves after the shift: the default since R8, so no bit - kept so that a
 * caller that asked for moves after the shift still gets them. */
#define T5D_RULES_MOVES_AFTER_SHIFT UINT64_C(0)

typedef struct t5d_game_summary {
  uint64_t boards;
  int32_t width;
  int32_t height;
  int32_t even;             /* 1: the game started with an even number of timelines */
  int32_t to_move;          /* the player to move: 0 white, 1 black */
  int32_t present;          /* the present (T4): a half-turn */
  int32_t timelines;
  int32_t active_timelines; /* T10 */
  int32_t obligations_met;  /* 1: the present has shifted to the opponent (T5) */
  int32_t may_move;         /* 1: a move may be made now (by default always, R8; under Q4's
                               Guide-literal reading only before the shift) */
  int32_t may_submit;       /* 1: the action may be submitted: the present shifted (T5), every royal safe (C4) */
  uint32_t ruleset;         /* the ruleset ID (0: standard) */
  uint64_t rules;           /* the ruleset's canonical encoding */
  /* appended in engine ABI round 1 (register A6): */
  int32_t moves_made;       /* the moves made in the action in progress (the handle's
                               count - no state records it): 0 at a turn start, one more
                               per t5d_game_apply or _apply_expanded, 0 again after
                               t5d_game_submit and t5d_game_replay_next */
} t5d_game_summary;

/* A game from a 5dpgn record (`length` bytes of UTF-8 text; the record's
 * headers name the variant or give 5DFEN boards), set up at its first position
 * and ready to replay. `rules`: the ruleset encoding (the record's Promotions
 * header and variant family are added to it). Errors: T5D_ERR_NOTATION (the
 * set-up cannot be read), T5D_ERR_UNSUPPORTED (a game the engine does not
 * play), T5D_ERR_ARGUMENT (an invalid ruleset). On failure *out is NULL. */
T5D_API t5d_status t5d_game_new_from_pgn(const char* text, size_t length, uint64_t rules,
                                         t5d_game** out, char** error);

/* A game from 5DFEN board strings, as t5d_state_new_from_fen (the player to
 * move INFER: the colour to move on the present, Q9). */
T5D_API t5d_status t5d_game_new_from_fen(const char* const* boards, size_t count, int32_t width,
                                         int32_t height, int32_t parity, int32_t to_move,
                                         uint64_t rules, t5d_game** out, char** error);

/* A named variant's start ("Standard", "Standard - Two Timelines", ...). */
T5D_API t5d_status t5d_game_new_variant(const char* name, uint64_t rules, t5d_game** out,
                                        char** error);

/* An independent copy (sharing the immutable boards; not the expansion), with
 * the count of moves made in the action. */
T5D_API t5d_status t5d_game_copy(const t5d_game* game, t5d_game** out);

/* Releases a game. NULL is ignored. */
T5D_API void t5d_game_destroy(t5d_game* game);

T5D_API t5d_status t5d_game_info(const t5d_game* game, t5d_game_summary* out);

/* For a game made from a record: the actions the record holds, how many have
 * been replayed, and whether its move text stopped early (1: unreadable from
 * there on). Any output pointer may be NULL. For other games: 0, 0, 0. */
T5D_API t5d_status t5d_game_record_actions(const t5d_game* game, size_t* total, size_t* played,
                                           int32_t* truncated);

/* Replays the record's next action - its moves, then the submission, under
 * the game's legality (t5d_game_set_legality: by default legally, as the
 * gate adapter's full mode does) - to the next turn start (moves made: 0).
 * On failure the game is unchanged:
 * T5D_ERR_ILLEGAL (an illegal move or a submission the rules refuse),
 * T5D_ERR_NOTATION (a move that cannot be read, or fits several moves),
 * T5D_ERR_ARGUMENT (no action left, or the game has left its record by
 * t5d_game_apply or t5d_game_submit). */
T5D_API t5d_status t5d_game_replay_next(t5d_game* game, char** error);

/* Applies one move of the player to move: the generated move with the same
 * source and destination (timeline, half-turn, square) and promotion (0 for
 * none) - the other fields of `move` are not read. T5D_ERR_ILLEGAL if there is
 * none, or no move may be made now (under Q4's Guide-literal reading, once the
 * present has shifted). The action has one more move (moves made + 1). */
T5D_API t5d_status t5d_game_apply(t5d_game* game, const t5d_move* move, char** error);

/* Submits the action (T5): the opponent's turn start (moves made: 0).
 * T5D_ERR_ILLEGAL while the present has not shifted to the opponent, or if a
 * royal piece of the mover's is left capturable (C4, R1). */
T5D_API t5d_status t5d_game_submit(t5d_game* game, char** error);

/* What a game's submissions - and its record's replay - are tested for:
 * T5D_LEGAL (the default: the present's shift, T5, and every royal piece of
 * the mover's safe, C4/R1) or T5D_PSEUDO_LEGAL (the present's shift only -
 * for replaying pseudo-legal play, such as synthetic records, as the gate
 * adapter's parse mode does). T5D_ERR_ARGUMENT for any other value. */
#define T5D_PSEUDO_LEGAL 0
#define T5D_LEGAL 1
T5D_API t5d_status t5d_game_set_legality(t5d_game* game, int32_t legality, char** error);

/* The game's multiverse as a t5d_state (fingerprint, canonical string, boards). */
T5D_API t5d_status t5d_game_state(const t5d_game* game, t5d_state** out);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* T5D_CAPI_H */

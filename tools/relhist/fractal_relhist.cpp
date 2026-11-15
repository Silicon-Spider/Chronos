// Fractal - tools/relhist/fractal_relhist.cpp
//
// Phase 0 measurement tool (docs/SPEC.md 7.3, 7.4, 14.1): the raw histograms G0
// cuts the nine displacement classes of the attack-relation inputs and the six
// delta-t-to-present placement buckets from, over t0database, and an
// engine-derived reachability table for trimming impossible feature
// combinations. It writes raw counts only; the analysis is done afterwards.
//
// New code written for Fractal. It calls the rules engine copied from
// Temporal-5D (engine/, namespace t5d) through its public headers and
// implements no rule of its own: every move, capture and defence below is the
// engine's movegen output (P2). No evaluation of any kind.
//
// Usage (either mode, or both in one run):
//   fractal-relhist --records DIR [--games FILE] [--records-out FILE] [--out FILE]
//   fractal-relhist --probe FILE
//
//   --records DIR       the converted t0database (python -I data/t0database/to_5dpgn.py DIR):
//                       every *.5dpgn in DIR, in name order; GameNum = the number in
//                       t0db-NNNN.5dpgn.
//   --games FILE        GameNums whose positions enter the histograms, one per line,
//                       '#' starts a comment. Default: every record.
//   --records-out FILE  one TSV row per record (every record, whatever --games says).
//   --out FILE          the histograms, JSON.
//   --probe FILE        the reachability table, JSON (independent of --records).
//
// Relation semantics (also written into the output; see kSemantics): a relation
// is a pseudo-legal capture geometry of a piece on a head board onto a piece -
// onto an enemy, a capture Move; onto an own piece, a GeometricRelation of kind
// Defence - for both colours, from every head. Colour c's pieces on heads where c
// is to move are read on the real multiverse when c is the side to move at the
// turn start; every other (colour, head) pair is read on passed_c, the real
// multiverse with an unchanged copy (en passant cleared) appended to every head
// on which c is not to move (the construction of the engine's check status, R1,
// applied to every such head: "semantics B, turn-aware").

#include <algorithm>
#include <array>
#include <stdexcept>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "t5d/board.hpp"
#include "t5d/fingerprint.hpp"
#include "t5d/move.hpp"
#include "t5d/movegen.hpp"
#include "t5d/multiverse.hpp"
#include "t5d/pgn.hpp"
#include "t5d/rules.hpp"
#include "t5d/state.hpp"
#include "t5d/types.hpp"
#include "t5d/variants.hpp"

namespace {

using namespace t5d;

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

constexpr std::size_t kKinds = static_cast<std::size_t>(kPieceKindCount);

constexpr std::array<const char*, kKinds> kKindNames = {
    "Pawn",   "Knight",  "Bishop",   "Rook",  "Queen",      "King",
    "Unicorn", "Dragon", "Princess", "Brawn", "CommonKing", "RoyalQueen"};

const char* kind_name(PieceKind k) { return kKindNames[static_cast<std::size_t>(index_of(k))]; }
const char* kind_name(int k) { return kKindNames[static_cast<std::size_t>(k)]; }
const char* colour_text(Colour c) { return c == Colour::White ? "w" : "b"; }
const char* colour_text(int c) { return c == 0 ? "w" : "b"; }

/// The 2x2 zone of a square, 0..15: (rank >> 1) * 4 + (file >> 1).
int zone_of(int square) { return (rank_of(square) >> 1) * 4 + (file_of(square) >> 1); }

bool is_pawnlike(PieceKind k) { return k == PieceKind::Pawn || k == PieceKind::Brawn; }

std::string hex(std::uint64_t v, int digits) {
  static constexpr char kDigits[] = "0123456789abcdef";
  std::string s(static_cast<std::size_t>(digits), '0');
  for (std::size_t i = s.size(); i-- > 0;) {
    s[i] = kDigits[v & 15U];
    v >>= 4;
  }
  return s;
}

std::string fixed(double v, int decimals) {
  char buf[64];
  std::snprintf(buf, sizeof buf, "%.*f", decimals, v);
  return buf;
}

/// Tabs and line breaks to spaces (a TSV field).
std::string one_line(std::string_view s) {
  std::string r(s);
  for (char& ch : r) {
    if (ch == '\t' || ch == '\n' || ch == '\r') ch = ' ';
  }
  return r;
}

bool slurp(const std::filesystem::path& path, std::string& out) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return false;
  std::ostringstream ss;
  ss << f.rdbuf();
  out = ss.str();
  return !f.bad();
}

bool write_file(const std::string& path, const std::string& text) {
  const std::filesystem::path p(path);
  std::error_code ec;
  if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path(), ec);
  std::ofstream f(p, std::ios::binary);
  if (!f) return false;
  f << text;
  f.close();  // the last buffer is written here: a failure must not pass silently
  return !f.fail();
}

// ---------------------------------------------------------------------------
// A minimal JSON writer
// ---------------------------------------------------------------------------

class JsonWriter {
 public:
  void open_object(bool multiline = true) {
    element();
    out_ += '{';
    stack_.push_back({true, multiline});
  }
  void close_object() { close('}'); }
  void open_array(bool multiline = false) {
    element();
    out_ += '[';
    stack_.push_back({true, multiline});
  }
  void close_array() { close(']'); }
  void key(std::string_view k) {
    element();
    append_string(k);
    out_ += ": ";
    after_key_ = true;
  }
  void string(std::string_view s) {
    element();
    append_string(s);
  }
  void integer(std::int64_t v) {
    element();
    out_ += std::to_string(v);
  }
  void uinteger(std::uint64_t v) {
    element();
    out_ += std::to_string(v);
  }
  void number(double v, int decimals = 6) {
    element();
    out_ += fixed(v, decimals);
  }
  void boolean(bool v) {
    element();
    out_ += v ? "true" : "false";
  }
  void null() {
    element();
    out_ += "null";
  }
  // key: value shorthands
  void kv(std::string_view k, std::string_view v) { key(k); string(v); }
  void kv(std::string_view k, const char* v) { key(k); string(v); }
  void kv_int(std::string_view k, std::int64_t v) { key(k); integer(v); }
  void kv_uint(std::string_view k, std::uint64_t v) { key(k); uinteger(v); }
  void kv_bool(std::string_view k, bool v) { key(k); boolean(v); }
  void kv_num(std::string_view k, double v, int decimals = 6) { key(k); number(v, decimals); }

  std::string text() const { return out_ + "\n"; }

 private:
  struct Level {
    bool first;
    bool multiline;
  };
  void element() {
    if (after_key_) {
      after_key_ = false;
      return;
    }
    if (stack_.empty()) return;
    Level& l = stack_.back();
    if (!l.first) out_ += ',';
    if (l.multiline) {
      out_ += '\n';
      out_.append(2 * stack_.size(), ' ');
    }
    l.first = false;
  }
  void close(char c) {
    const Level l = stack_.back();
    stack_.pop_back();
    if (l.multiline && !l.first) {
      out_ += '\n';
      out_.append(2 * stack_.size(), ' ');
    }
    out_ += c;
  }
  void append_string(std::string_view s) {
    out_ += '"';
    for (const char ch : s) {
      const auto u = static_cast<unsigned char>(ch);
      if (ch == '"') {
        out_ += "\\\"";
      } else if (ch == '\\') {
        out_ += "\\\\";
      } else if (ch == '\n') {
        out_ += "\\n";
      } else if (ch == '\t') {
        out_ += "\\t";
      } else if (ch == '\r') {
        out_ += "\\r";
      } else if (u < 0x20) {
        out_ += "\\u00";
        out_ += hex(u, 2);
      } else {
        out_ += ch;
      }
    }
    out_ += '"';
  }

  // The open containers, innermost last, in a fixed-depth stack: a std::vector of this
  // 2-byte struct trips a false -Wstringop-overflow in g++ 13's libstdc++ at -O2.
  class LevelStack {
   public:
    void push_back(Level l) {
      if (size_ == data_.size()) throw std::length_error("JSON nested too deeply");
      data_[size_++] = l;
    }
    Level& back() { return data_[size_ - 1]; }
    void pop_back() { --size_; }
    bool empty() const { return size_ == 0; }
    std::size_t size() const { return size_; }

   private:
    std::array<Level, 32> data_{};
    std::size_t size_ = 0;
  };

  std::string out_;
  LevelStack stack_;
  bool after_key_ = false;
};

// ---------------------------------------------------------------------------
// Relations and placement (docs/SPEC.md 7.3, 7.4)
// ---------------------------------------------------------------------------

constexpr const char* kSemantics =
    "A relation is a pseudo-legal capture geometry of a piece on a HEAD board onto a piece, as "
    "the copied engine's movegen (append_moves_of) generates it: onto an enemy piece, a capture "
    "Move (en passant: the victim is the pawn on aux_square; a promoting capture, emitted once "
    "per promotion piece, is counted once); onto an own piece, a GeometricRelation of kind "
    "Defence (the same capture paths and landing rules). XRay and Pin relations are not "
    "counted. Pseudo-legal: pins and own-royal safety are ignored. Both colours, from every "
    "head board, whoever moves there next; each relation belongs to the attacker's real head. "
    "Semantics B, turn-aware: for colour c, c's pieces on heads where c is to move are read on "
    "the real multiverse if c is the side to move at the turn start; every other (colour, head) "
    "pair is read on passed_c = the real multiverse with an unchanged copy of the head (en "
    "passant cleared, unmoved flags kept) appended at half-turn h+1 to EVERY head on which c is "
    "not to move - check status's (R1) passed-multiverse construction applied to every such "
    "head - so that c is to move on every head of passed_c. A victim found on a copy (q, "
    "last(q)+1) is mapped back to the real head (q, last(q)) it copies. Positions: the turn "
    "starts (action starts) of complete records, replayed in parse mode "
    "(CheckRuling::none()), de-duplicated by (zobrist, side to move) over the selected games, "
    "first occurrence wins.";

enum RelField : std::size_t {
  kAKind,
  kACol,
  kAMover,
  kVKind,
  kVCol,
  kOwn,
  kDL,
  kDTh,
  kDTt,
  kPassed,
  kVHead,
  kLoc,
  kRelFields
};
using RelKey = std::array<int, kRelFields>;
constexpr std::array<const char*, kRelFields> kRelFieldNames = {
    "akind", "acol", "amover", "vkind", "vcol", "own",
    "dL",    "dTh",  "dTt",    "passed", "vhead", "loc"};

enum class Role : int { Mandatory = 0, Optional, Opponent, Inactive };
constexpr std::array<const char*, 4> kRoleNames = {"mandatory", "optional", "opponent",
                                                   "inactive"};
using PlaceKey = std::array<int, 4>;  // dt, role, active, mnext

struct Hist {
  std::map<RelKey, std::uint64_t> relations;
  std::map<PlaceKey, std::uint64_t> placement;
  std::uint64_t total = 0;
  std::uint64_t captures = 0;
  std::uint64_t defences = 0;
  std::uint64_t passed = 0;
  std::uint64_t heads = 0;
};

/// Invariants checked on every relation and head; every counter but the
/// informational ones (time_only, promotion_duplicates, en_passant,
/// past_self_defences, victims_on_copies) should be 0. past_self_defences:
/// defences with dL == 0 and dTh < 0 onto a piece of the attacker's kind on the
/// attacker's square - usually the attacker's own earlier self.
struct Checks {
  std::uint64_t same_board_mismatch = 0;     // (dL == 0 && dTh == 0) != the move stayed on its board
  std::uint64_t colour_mismatch = 0;         // a defence onto an enemy, or a capture onto an own piece
  std::uint64_t time_only = 0;               // informational: relations with dL == 0, dTh != 0
  std::uint64_t time_only_not_backward = 0;  // of those, dTt >= 0
  std::uint64_t odd_turn_step = 0;           // to_half_turn - from_half_turn odd
  std::uint64_t victim_mismatch = 0;         // the real board's piece is not the victim recorded
  std::uint64_t attacker_mismatch = 0;       // the head's piece is not the attacker recorded
  std::uint64_t source_mismatch = 0;         // generated from a board other than the expected source
  std::uint64_t promotion_duplicates = 0;    // informational: repeated promoting captures dropped
  std::uint64_t other_duplicates = 0;        // a repeated capture that was not a promotion
  std::uint64_t en_passant = 0;              // informational: en-passant captures
  std::uint64_t role_unexpected = 0;         // a head whose role disagrees with the state's predicates
  std::uint64_t past_self_defences = 0;      // informational: own, dL == 0, dTh < 0, same square and kind
  std::uint64_t victims_on_copies = 0;       // informational: victims found on a passed copy, mapped back

  void write(JsonWriter& j) const {
    j.open_object();
    j.kv_uint("same_board_mismatch", same_board_mismatch);
    j.kv_uint("colour_mismatch", colour_mismatch);
    j.kv_uint("time_only_relations", time_only);
    j.kv_uint("time_only_not_backward", time_only_not_backward);
    j.kv_uint("odd_turn_step", odd_turn_step);
    j.kv_uint("victim_mismatch", victim_mismatch);
    j.kv_uint("attacker_mismatch", attacker_mismatch);
    j.kv_uint("source_mismatch", source_mismatch);
    j.kv_uint("promotion_duplicates_dropped", promotion_duplicates);
    j.kv_uint("other_duplicates_dropped", other_duplicates);
    j.kv_uint("en_passant_captures", en_passant);
    j.kv_uint("role_unexpected", role_unexpected);
    j.kv_uint("past_self_defences", past_self_defences);
    j.kv_uint("victims_on_passed_copies", victims_on_copies);
    j.close_object();
  }
};

struct Scratch {
  MoveList moves;
  RelationList geo;
};

/// passed_c: the real multiverse with an unchanged copy (en passant cleared)
/// appended to every head on which `c` is not to move. On it `c` is to move on
/// every head.
Multiverse passed_for(const Multiverse& real, Colour c) {
  Multiverse mv = real;  // shares every timeline
  for (const TimelineRef& t : real.timelines()) {
    if (to_move_at(t->last_half_turn()) == c) continue;
    BoardBuilder b(*t->head());
    b.set_en_passant(kNoSquare);
    mv.append_board(t->position(), b.publish());
  }
  return mv;
}

/// Adds one turn start's head placements and relations to `hist`. Returns the
/// number of relations recorded.
std::uint64_t visit(const GameState& s, Hist& hist, Checks& ck, Scratch& sc) {
  const Multiverse& real = s.multiverse();
  const std::span<const int> inactive(s.inactive_timelines());
  const Colour mover = s.to_move();

  // Placement: every head board.
  for (const TimelineRef& t : real.timelines()) {
    const int p = t->position();
    const int h = t->last_half_turn();
    const bool act = s.active(p);
    const bool mnext = to_move_at(h) == mover;
    Role role = Role::Optional;
    if (!act) {
      role = Role::Inactive;
    } else if (!mnext) {
      role = Role::Opponent;
    } else if (s.mandatory(p)) {
      role = Role::Mandatory;
    } else if (!s.playable_by_mover(p)) {
      ++ck.role_unexpected;  // an active head with the mover to move is always playable
    }
    if (s.mandatory(p) && role != Role::Mandatory) ++ck.role_unexpected;
    if (s.playable_by_mover(p) != mnext) ++ck.role_unexpected;
    ++hist.placement[PlaceKey{{h - s.present(), static_cast<int>(role), act ? 1 : 0, mnext ? 1 : 0}}];
    ++hist.heads;
  }

  // A board found on a passed copy (q, last(q)+1) stands on the real head (q, last(q)).
  const auto real_half_turn = [&real](int q, int hq) {
    const Timeline* rt = real.timeline(q);
    return (rt != nullptr && hq == rt->last_half_turn() + 1) ? hq - 1 : hq;
  };

  const RelationKinds kinds{/*defence=*/true, /*xray=*/false, /*pin=*/false};
  std::uint64_t count = 0;
  for (const Colour c : {Colour::White, Colour::Black}) {
    const Multiverse passed = passed_for(real, c);
    const int amover = c == mover ? 1 : 0;
    for (const TimelineRef& t : real.timelines()) {
      const Board& head = *t->head();
      if (head.pieces(c) == 0) continue;
      const int p = t->position();
      const int h = t->last_half_turn();
      const bool c_moves_here = to_move_at(h) == c;
      const bool use_real = c_moves_here && c == mover;
      const Multiverse& mv = use_real ? real : passed;
      const int src_h = c_moves_here ? h : h + 1;  // the board c's pieces move from
      sc.moves.clear();
      sc.geo.clear();  // append_moves_of appends, never clears
      append_moves_of(mv, s.rules(), c, p, sc.moves, sc.geo, kinds, inactive);

      const auto add = [&](PieceKind ak, int asq, PieceKind vk, Colour vc, bool own, int vp,
                           int to_h, int from_h, int vsq, bool on_board) {
        const int vh = real_half_turn(vp, to_h);
        if (vh != to_h) ++ck.victims_on_copies;
        const Timeline* vt = real.timeline(vp);
        const int dl = vp - p;
        const int dth = vh - h;
        const int step = to_h - from_h;
        if (step % 2 != 0) ++ck.odd_turn_step;
        const int dtt = step / 2;
        const bool same = dl == 0 && dth == 0;
        if (same != on_board) ++ck.same_board_mismatch;
        if (dl == 0 && dth != 0) {
          ++ck.time_only;
          if (dtt >= 0) ++ck.time_only_not_backward;
        }
        if (own != (vc == c)) ++ck.colour_mismatch;
        if (own && dl == 0 && dth < 0 && vsq == asq && vk == ak) ++ck.past_self_defences;
        const int vhead = (vt != nullptr && vh == vt->last_half_turn()) ? 1 : 0;
        const RelKey key{{index_of(ak), index_of(c), amover, index_of(vk), index_of(vc), own ? 1 : 0,
                          dl, dth, dtt, use_real ? 0 : 1, vhead, same ? vsq : zone_of(vsq)}};
        ++hist.relations[key];
        ++hist.total;
        if (own) {
          ++hist.defences;
        } else {
          ++hist.captures;
        }
        if (!use_real) ++hist.passed;
        ++count;
      };

      // Enemy victims: the captures. A promoting capture repeats once per
      // promotion piece, consecutively.
      const Move* prev = nullptr;
      for (const Move& m : sc.moves) {
        if (!m.capture()) continue;
        if (m.from_timeline != p || m.from_half_turn != src_h) ++ck.source_mismatch;
        if (m.piece.empty() || m.piece.colour() != c || head.at(m.from_square) != m.piece) {
          ++ck.attacker_mismatch;
          continue;
        }
        if (prev != nullptr && prev->from_square == m.from_square &&
            prev->to_timeline == m.to_timeline && prev->to_half_turn == m.to_half_turn &&
            prev->to_square == m.to_square) {
          if (m.promotes() && prev->promotes()) {
            ++ck.promotion_duplicates;
          } else {
            ++ck.other_duplicates;
          }
          continue;
        }
        prev = &m;
        if (m.captured.empty()) {
          ++ck.victim_mismatch;
          continue;
        }
        const int vsq = m.en_passant() ? m.aux_square : m.to_square;
        if (m.en_passant()) ++ck.en_passant;
        const Board* vb = real.board_at(m.to_timeline, real_half_turn(m.to_timeline, m.to_half_turn));
        if (vb == nullptr || vb->at(vsq) != m.captured) ++ck.victim_mismatch;
        add(m.piece.kind(), m.from_square, m.captured.kind(), m.captured.colour(), false,
            m.to_timeline, m.to_half_turn, m.from_half_turn, vsq, m.physical());
      }

      // Own victims: the Defence relations. They carry no pieces: the attacker
      // is the head's piece (a copy's content is the head's), the victim the
      // real board's.
      for (const GeometricRelation& r : sc.geo) {
        if (r.kind != GeometricRelation::Kind::Defence) continue;
        if (r.from_timeline != p || r.from_half_turn != src_h) ++ck.source_mismatch;
        const Piece a = head.at(r.from_square);
        const Board* vb = real.board_at(r.to_timeline, real_half_turn(r.to_timeline, r.to_half_turn));
        const Piece v = vb != nullptr ? vb->at(r.to_square) : Piece{};
        if (a.empty() || a.colour() != c) {
          ++ck.attacker_mismatch;
          continue;
        }
        if (v.empty()) {
          ++ck.victim_mismatch;
          continue;
        }
        const bool on_board =
            r.from_timeline == r.to_timeline && r.from_half_turn == r.to_half_turn;
        add(a.kind(), r.from_square, v.kind(), v.colour(), true, r.to_timeline, r.to_half_turn,
            r.from_half_turn, r.to_square, on_board);
      }
    }
  }
  return count;
}

// ---------------------------------------------------------------------------
// --records
// ---------------------------------------------------------------------------

struct Options {
  std::string records;
  std::string games;
  std::string records_out;
  std::string out;
  std::string probe;
};

/// The number at the end of a file stem ("t0db-0042" -> 42), or -1.
int game_num_of(const std::string& stem) {
  std::size_t i = stem.size();
  while (i > 0 && stem[i - 1] >= '0' && stem[i - 1] <= '9') --i;
  if (i == stem.size() || stem.size() - i > 9) return -1;
  int n = 0;
  for (std::size_t k = i; k < stem.size(); ++k) n = n * 10 + (stem[k] - '0');
  return n;
}

bool read_games(const std::string& path, std::set<int>& games, std::string& error) {
  std::string text;
  if (!slurp(path, text)) {
    error = "cannot read " + path;
    return false;
  }
  // A UTF-8 byte-order mark (Windows PowerShell's Out-File and '>' write one).
  if (text.starts_with("\xEF\xBB\xBF")) text.erase(0, 3);
  std::istringstream in(text);
  std::string line;
  int line_no = 0;
  while (std::getline(in, line)) {
    ++line_no;
    const std::size_t hash = line.find('#');
    if (hash != std::string::npos) line.erase(hash);
    const std::size_t b = line.find_first_not_of(" \t\r");
    if (b == std::string::npos) continue;
    const std::size_t e = line.find_last_not_of(" \t\r");
    const std::string tok = line.substr(b, e - b + 1);
    int n = 0;
    bool ok = !tok.empty() && tok.size() <= 9;
    for (const char ch : tok) {
      if (ch < '0' || ch > '9') {
        ok = false;
        break;
      }
      n = n * 10 + (ch - '0');
    }
    if (!ok) {
      error = path + ":" + std::to_string(line_no) + ": not a GameNum: " + tok;
      return false;
    }
    games.insert(n);
  }
  return true;
}

struct RecordCounts {
  std::uint64_t read = 0;
  std::uint64_t complete = 0;
  std::uint64_t refused = 0;
  void write(JsonWriter& j) const {
    j.open_object(false);
    j.kv_uint("read", read);
    j.kv_uint("complete", complete);
    j.kv_uint("refused", refused);
    j.close_object();
  }
};

struct Bucket {
  std::uint64_t positions = 0;
  std::uint64_t boards = 0;
  std::uint64_t timelines = 0;
  std::uint64_t active = 0;
  std::uint64_t relations = 0;
};
constexpr std::array<const char*, 4> kBucketNames = {"<50", "50-99", "100-199", "200+"};

std::size_t bucket_of(std::size_t boards) {
  if (boards < 50) return 0;
  if (boards < 100) return 1;
  if (boards < 200) return 2;
  return 3;
}

/// The Turn Zero opening position's relations onto its T0 board (h = 1).
struct TurnZeroCheck {
  bool available = false;
  std::uint64_t key = 0;
  Colour to_move = Colour::White;
  int head_half_turn = -1;
  bool t0_board_exists = false;
  std::uint64_t relations = 0;
  std::uint64_t black_onto_t0_captures = 0;
  std::uint64_t black_onto_t0_defences = 0;
  std::uint64_t white_onto_t0 = 0;
};

TurnZeroCheck turn_zero_check() {
  TurnZeroCheck r;
  const Variant* v = find_variant("Standard - Turn Zero");
  if (v == nullptr) return r;
  const GameState s0 = variant_start(*v);
  r.available = true;
  r.key = s0.zobrist();
  r.to_move = s0.to_move();
  const Timeline* t0 = s0.multiverse().timeline(0);
  r.head_half_turn = t0 != nullptr ? t0->last_half_turn() : -1;
  r.t0_board_exists = s0.multiverse().board_at(0, 1) != nullptr;
  Hist h;
  Checks ck;
  Scratch sc;
  r.relations = visit(s0, h, ck, sc);
  // Timeline 0 is the only timeline; its head is h = 2, so dL == 0, dTh == -1 is the T0 board.
  for (const auto& [k, n] : h.relations) {
    if (k[kDL] != 0 || k[kDTh] != 1 - r.head_half_turn) continue;
    if (k[kACol] == index_of(Colour::Black)) {
      if (k[kOwn] != 0) {
        r.black_onto_t0_defences += n;
      } else {
        r.black_onto_t0_captures += n;
      }
    } else {
      r.white_onto_t0 += n;
    }
  }
  return r;
}

std::string key_text(const GameState& s) {
  return hex(s.zobrist(), 16) + colour_letter(s.to_move());
}

int run_records(const Options& opt) {
  const auto start = std::chrono::steady_clock::now();

  std::vector<std::filesystem::path> files;
  try {
    for (const auto& e : std::filesystem::directory_iterator(opt.records)) {
      if (e.is_regular_file() && e.path().extension() == ".5dpgn") files.push_back(e.path());
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "fractal-relhist: cannot list %s: %s\n", opt.records.c_str(), e.what());
    return 2;
  }
  std::sort(files.begin(), files.end(),
            [](const std::filesystem::path& a, const std::filesystem::path& b) {
              return a.filename().string() < b.filename().string();
            });
  if (files.empty()) {
    std::fprintf(stderr, "fractal-relhist: no .5dpgn records in %s\n", opt.records.c_str());
    return 2;
  }

  std::optional<std::set<int>> games;
  if (!opt.games.empty()) {
    std::set<int> g;
    std::string error;
    if (!read_games(opt.games, g, error)) {
      std::fprintf(stderr, "fractal-relhist: %s\n", error.c_str());
      return 2;
    }
    games = std::move(g);
  }

  const TurnZeroCheck tz = turn_zero_check();

  std::string tsv = "game_num\tstatus\tfault\tmessage\tactions\tturn_starts\tboards\ttimelines\tkeys\n";
  RecordCounts overall;
  RecordCounts selected_counts;
  Hist hist;
  Checks ck;
  Scratch sc;
  std::set<std::pair<std::uint64_t, int>> seen;
  std::set<std::pair<std::string, int>> seen_fp;
  std::array<Bucket, 4> buckets{};
  std::uint64_t turn_starts_used = 0;
  std::uint64_t starts_at_tz = 0;
  std::map<std::string, std::uint64_t> faults;
  std::map<std::string, std::uint64_t> variants;  // PgnReplay::game() of the selected complete records

  for (const std::filesystem::path& path : files) {
    const int game_num = game_num_of(path.stem().string());
    const bool selected = !games || games->count(game_num) != 0;
    std::string fault;
    std::string message;
    std::size_t actions = 0;
    std::size_t turn_starts = 0;
    std::size_t boards = 0;
    int timelines = 0;
    std::string game;
    std::string keys;
    std::vector<GameState> states;

    std::string text;
    if (!slurp(path, text)) {
      fault = "io";
      message = "cannot read " + path.string();
    } else {
      std::optional<PgnReplay> replay;
      try {
        replay.emplace(read_pgn(text));
        replay->set_check_ruling(CheckRuling::none());  // parse mode
        for (;;) {
          const GameState& s = replay->state();  // a turn start
          if (!keys.empty()) keys += ',';
          keys += key_text(s);
          ++turn_starts;
          if (selected) states.push_back(s);
          if (replay->finished()) break;
          replay->play_next();  // throws RecordError, leaving the state as it was
        }
        if (replay->record().unreadable.has_value()) {
          fault = fault_name(RecordFault::Unreadable);
          message = *replay->record().unreadable;
        }
      } catch (const RecordError& e) {
        fault = fault_name(e.fault());
        message = e.what();
      } catch (const std::exception& e) {
        fault = "exception";
        message = e.what();
      }
      if (replay.has_value()) {
        game = replay->game();
        actions = replay->actions_played();
        boards = replay->state().multiverse().board_count();
        timelines = replay->state().multiverse().timeline_count();
      }
    }
    const bool complete = fault.empty();

    ++overall.read;
    ++(complete ? overall.complete : overall.refused);
    if (selected) {
      ++selected_counts.read;
      ++(complete ? selected_counts.complete : selected_counts.refused);
    }
    if (!complete) ++faults[fault];

    tsv += std::to_string(game_num);
    tsv += complete ? "\tcomplete\t" : "\trefused\t";
    tsv += one_line(fault) + "\t" + one_line(message) + "\t" + std::to_string(actions) + "\t" +
           std::to_string(turn_starts) + "\t" + std::to_string(boards) + "\t" +
           std::to_string(timelines) + "\t" + keys + "\n";

    if (!complete || !selected) continue;
    ++variants[game];
    turn_starts_used += states.size();
    if (!states.empty() && tz.available && states.front().zobrist() == tz.key &&
        states.front().to_move() == tz.to_move) {
      ++starts_at_tz;
    }
    for (const GameState& s : states) {
      seen_fp.insert({t5d::fingerprint(s.multiverse()).fingerprint, index_of(s.to_move())});
      if (!seen.insert({s.zobrist(), index_of(s.to_move())}).second) continue;
      const std::uint64_t n = visit(s, hist, ck, sc);
      const Multiverse& mv = s.multiverse();
      Bucket& b = buckets[bucket_of(mv.board_count())];
      ++b.positions;
      b.boards += mv.board_count();
      b.timelines += static_cast<std::uint64_t>(mv.timeline_count());
      for (const TimelineRef& t : mv.timelines()) {
        if (s.active(t->position())) ++b.active;
      }
      b.relations += n;
    }
  }

  const double seconds =
      std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

  if (!opt.records_out.empty() && !write_file(opt.records_out, tsv)) {
    std::fprintf(stderr, "fractal-relhist: cannot write %s\n", opt.records_out.c_str());
    return 2;
  }

  if (!opt.out.empty()) {
    JsonWriter j;
    j.open_object();

    j.key("meta");
    j.open_object();
    j.kv("tool", "fractal-relhist --records (tools/relhist/fractal_relhist.cpp)");
    j.kv("engine", "Temporal-5D rules engine copied at c8a9c49 (engine/, namespace t5d)");
    j.key("options");
    j.open_object();
    j.kv("records", opt.records);
    j.key("games");
    if (opt.games.empty()) {
      j.null();
    } else {
      j.string(opt.games);
    }
    j.kv("records_out", opt.records_out);
    j.kv("out", opt.out);
    j.kv("replay", "read_pgn, PgnReplay, set_check_ruling(CheckRuling::none()) (parse mode)");
    j.kv("rules", "each record's own: the game its headers declare (PgnReplay), default "
                  "RuleOptions otherwise; counted in variants_of_selected_complete_records");
    j.close_object();
    j.key("variants_of_selected_complete_records");
    j.open_object(false);
    for (const auto& [g, n] : variants) j.kv_uint(g, n);
    j.close_object();
    j.kv("semantics", kSemantics);
    j.key("records");
    j.open_object();
    j.key("overall");
    overall.write(j);
    j.key("selected");
    selected_counts.write(j);
    j.key("refusals_by_fault_overall");
    j.open_object(false);
    for (const auto& [f, n] : faults) j.kv_uint(f, n);
    j.close_object();
    j.close_object();
    j.kv_uint("turn_starts_in_selected_complete_records", turn_starts_used);
    j.kv_uint("distinct_positions", seen.size());
    j.kv_uint("distinct_positions_by_fingerprint_and_side", seen_fp.size());
    j.kv_uint("selected_complete_records_starting_at_turn_zero_start", starts_at_tz);
    j.kv_uint("relations_recorded", hist.total);
    j.kv_uint("captures", hist.captures);
    j.kv_uint("defences", hist.defences);
    j.kv_uint("passed", hist.passed);
    j.kv_uint("distinct_relation_tuples", hist.relations.size());
    j.kv_uint("head_boards", hist.heads);
    j.key("checks");
    ck.write(j);
    j.key("turn_zero_start");
    j.open_object();
    j.kv_bool("available", tz.available);
    j.kv("key", hex(tz.key, 16) + colour_letter(tz.to_move));
    j.kv_int("head_half_turn", tz.head_half_turn);
    j.kv_bool("t0_board_exists", tz.t0_board_exists);
    j.kv_uint("relations", tz.relations);
    j.kv_uint("black_onto_t0_captures", tz.black_onto_t0_captures);
    j.kv_uint("black_onto_t0_defences", tz.black_onto_t0_defences);
    j.kv_uint("white_onto_t0", tz.white_onto_t0);
    j.close_object();
    j.kv_num("seconds", seconds, 3);
    j.close_object();

    j.key("relations");
    j.open_object();
    j.key("fields");
    j.open_array();
    for (const char* f : kRelFieldNames) j.string(f);
    j.string("count");
    j.close_array();
    j.key("legend");
    j.open_object();
    j.kv("akind", "attacker PieceKind name");
    j.kv("acol", "attacker colour, w or b");
    j.kv("amover", "1 if acol is the side to move at the turn start (acol_rel, folded in)");
    j.kv("vkind", "victim PieceKind name");
    j.kv("vcol", "victim colour, w or b");
    j.kv("own", "1 for a defence (victim of the attacker's colour), 0 for a capture");
    j.kv("dL", "victim timeline position - attacker timeline position (signed; adjacent "
               "timelines differ by 1, even games too)");
    j.kv("dTh", "victim REAL board half-turn - attacker REAL head half-turn (signed half-turns)");
    j.kv("dTt", "(move to_half_turn - from_half_turn) / 2: signed whole turns along the move, "
                "from_half_turn being the board the piece moved from (the head, or its passed "
                "copy at head + 1)");
    j.kv("passed", "1 if read on the passed multiverse passed_c");
    j.kv("vhead", "1 if the victim's real board is its timeline's head");
    j.kv("loc", "victim square 0..63 (rank*8 + file, a1 = 0) when dL == 0 and dTh == 0 (the "
                "attacker's own board); otherwise its 2x2 zone (rank>>1)*4 + (file>>1), 0..15");
    j.close_object();
    j.key("rows");
    j.open_array(true);
    for (const auto& [k, n] : hist.relations) {
      j.open_array();
      j.string(kind_name(k[kAKind]));
      j.string(colour_text(k[kACol]));
      j.integer(k[kAMover]);
      j.string(kind_name(k[kVKind]));
      j.string(colour_text(k[kVCol]));
      for (std::size_t f = kOwn; f < kRelFields; ++f) j.integer(k[f]);
      j.uinteger(n);
      j.close_array();
    }
    j.close_array();
    j.close_object();

    // acol_rel: folded into relations as amover; summed here by (acol, amover, passed).
    std::map<std::array<int, 3>, std::uint64_t> acol_rel;
    for (const auto& [k, n] : hist.relations) {
      acol_rel[std::array<int, 3>{{k[kACol], k[kAMover], k[kPassed]}}] += n;
    }
    j.key("acol_rel");
    j.open_object();
    j.kv("note", "folded into relations as the field amover; summed here by (acol, amover, passed)");
    j.key("fields");
    j.open_array();
    for (const char* f : {"acol", "amover", "passed", "count"}) j.string(f);
    j.close_array();
    j.key("rows");
    j.open_array(true);
    for (const auto& [k, n] : acol_rel) {
      j.open_array();
      j.string(colour_text(k[0]));
      j.integer(k[1]);
      j.integer(k[2]);
      j.uinteger(n);
      j.close_array();
    }
    j.close_array();
    j.close_object();

    j.key("placement");
    j.open_object();
    j.key("fields");
    j.open_array();
    for (const char* f : {"dt", "role", "active", "mnext", "count"}) j.string(f);
    j.close_array();
    j.key("legend");
    j.open_object();
    j.kv("dt", "head half-turn - state.present() (signed half-turns)");
    j.kv("role", "inactive: the timeline is inactive; else opponent: the opponent of the side to "
                 "move moves next on the head; else mandatory: state.mandatory(p); else "
                 "optional: playable_by_mover and not mandatory");
    j.kv("active", "state.active(p)");
    j.kv("mnext", "1 if the side to move at the turn start moves next on the head (extra field, "
                  "so inactive heads can be split; sum it out for (dt, role, active))");
    j.close_object();
    j.key("rows");
    j.open_array(true);
    for (const auto& [k, n] : hist.placement) {
      j.open_array();
      j.integer(k[0]);
      j.string(kRoleNames[static_cast<std::size_t>(k[1])]);
      j.integer(k[2]);
      j.integer(k[3]);
      j.uinteger(n);
      j.close_array();
    }
    j.close_array();
    j.close_object();

    j.key("positions");
    j.open_object();
    j.kv("note", "distinct positions by board count; sums and means per position");
    j.key("rows");
    j.open_array(true);
    for (std::size_t i = 0; i < buckets.size(); ++i) {
      const Bucket& b = buckets[i];
      const double n = b.positions > 0 ? static_cast<double>(b.positions) : 1.0;
      j.open_object(false);
      j.kv("boards", kBucketNames[i]);
      j.kv_uint("positions", b.positions);
      j.kv_uint("boards_sum", b.boards);
      j.kv_uint("timelines_sum", b.timelines);
      j.kv_uint("active_sum", b.active);
      j.kv_uint("relations_sum", b.relations);
      j.kv_num("mean_boards", static_cast<double>(b.boards) / n, 3);
      j.kv_num("mean_timelines", static_cast<double>(b.timelines) / n, 4);
      j.kv_num("mean_active", static_cast<double>(b.active) / n, 4);
      j.kv_num("mean_relations", static_cast<double>(b.relations) / n, 3);
      j.close_object();
    }
    j.close_array();
    j.close_object();

    j.close_object();
    if (!write_file(opt.out, j.text())) {
      std::fprintf(stderr, "fractal-relhist: cannot write %s\n", opt.out.c_str());
      return 2;
    }
  }

  std::printf("records: %llu read, %llu complete, %llu refused",
              static_cast<unsigned long long>(overall.read),
              static_cast<unsigned long long>(overall.complete),
              static_cast<unsigned long long>(overall.refused));
  if (games) {
    std::printf(" (selected: %llu read, %llu complete, %llu refused)",
                static_cast<unsigned long long>(selected_counts.read),
                static_cast<unsigned long long>(selected_counts.complete),
                static_cast<unsigned long long>(selected_counts.refused));
  }
  std::printf("\nrefusals:");
  for (const auto& [f, n] : faults) std::printf(" %s %llu", f.c_str(), static_cast<unsigned long long>(n));
  std::printf("\nvariants (selected complete records):");
  for (const auto& [g, n] : variants) std::printf(" \"%s\" %llu", g.c_str(), static_cast<unsigned long long>(n));
  std::printf("\nturn starts used: %llu; distinct positions: %zu (by fingerprint and side: %zu)\n",
              static_cast<unsigned long long>(turn_starts_used), seen.size(), seen_fp.size());
  std::printf("relations: %llu (captures %llu, defences %llu), passed %llu (%.2f%%), %zu tuples\n",
              static_cast<unsigned long long>(hist.total),
              static_cast<unsigned long long>(hist.captures),
              static_cast<unsigned long long>(hist.defences),
              static_cast<unsigned long long>(hist.passed),
              hist.total > 0 ? 100.0 * static_cast<double>(hist.passed) /
                                   static_cast<double>(hist.total)
                             : 0.0,
              hist.relations.size());
  std::printf("checks: same_board_mismatch %llu, colour_mismatch %llu, time_only %llu "
              "(not backward %llu), odd_turn_step %llu, victim_mismatch %llu, "
              "attacker_mismatch %llu, source_mismatch %llu, other_duplicates %llu, "
              "role_unexpected %llu; promotion duplicates dropped %llu, en passant %llu, "
              "past-self defences %llu, victims on passed copies %llu\n",
              static_cast<unsigned long long>(ck.same_board_mismatch),
              static_cast<unsigned long long>(ck.colour_mismatch),
              static_cast<unsigned long long>(ck.time_only),
              static_cast<unsigned long long>(ck.time_only_not_backward),
              static_cast<unsigned long long>(ck.odd_turn_step),
              static_cast<unsigned long long>(ck.victim_mismatch),
              static_cast<unsigned long long>(ck.attacker_mismatch),
              static_cast<unsigned long long>(ck.source_mismatch),
              static_cast<unsigned long long>(ck.other_duplicates),
              static_cast<unsigned long long>(ck.role_unexpected),
              static_cast<unsigned long long>(ck.promotion_duplicates),
              static_cast<unsigned long long>(ck.en_passant),
              static_cast<unsigned long long>(ck.past_self_defences),
              static_cast<unsigned long long>(ck.victims_on_copies));
  std::printf("turn zero start: %s, head h=%d, T0 board %s; black onto T0: %llu captures, "
              "%llu defences; white onto T0: %llu; records starting there: %llu\n",
              tz.available ? "found" : "MISSING", tz.head_half_turn,
              tz.t0_board_exists ? "present" : "absent",
              static_cast<unsigned long long>(tz.black_onto_t0_captures),
              static_cast<unsigned long long>(tz.black_onto_t0_defences),
              static_cast<unsigned long long>(tz.white_onto_t0),
              static_cast<unsigned long long>(starts_at_tz));
  std::printf("time: %.2f s\n", seconds);
  return 0;
}

// ---------------------------------------------------------------------------
// --probe: the engine-derived reachability table
// ---------------------------------------------------------------------------

constexpr int kProbeHeadTurn = 5;   // the attacker's head: timeline 0 at T5
constexpr int kProbeFarTurn = 9;    // every other timeline runs to T9
constexpr int kProbeTimelines = 4;  // timelines -4..+4
constexpr int kVerifySquare = 27;   // d4: the cross-board capture-equals-move check

struct AcrossCell {
  std::uint16_t zones = 0;
  int squares = 0;            // attacker squares that reach this displacement
  std::uint64_t targets = 0;  // (attacker square, target cell) pairs
  std::array<std::uint16_t, kSquareCount> per_square{};  // zones reached from each attacker square
};

struct KindProbe {
  PieceKind kind = PieceKind::Pawn;
  Colour colour = Colour::White;
  std::uint64_t same_union = 0;
  std::array<std::uint64_t, kSquareCount> same_masks{};
  std::array<bool, kSquareCount> valid{};
  std::map<std::pair<int, int>, AcrossCell> across;  // (dL, dTt) -> cell
};

struct ProbeChecks {
  std::uint64_t same_checked = 0;
  std::uint64_t same_mismatch = 0;
  std::uint64_t across_checked = 0;
  std::uint64_t across_mismatch = 0;
  std::uint64_t unexpected_quiet_capture = 0;  // a capture on an otherwise empty board
  std::uint64_t odd_turn_step = 0;
};

Multiverse single_board(const BoardRef& board, Colour c) {
  Multiverse mv(BoardSize{}, false);
  NewTimeline nt;
  nt.position = 0;
  nt.first_half_turn = half_turn(1, c);
  nt.boards.push_back(board);
  mv.add_timeline(std::move(nt));
  mv.set_to_move(c);
  return mv;
}

/// Timelines -4..+4, all initial (so all active): timeline 0 from T1 (white) to
/// T5 with c to move, the others from T1 to T9 with c to move, every board of
/// both colours present. board_for(p, h) gives each board.
template <class F>
Multiverse across_multiverse(Colour c, const F& board_for) {
  Multiverse mv(BoardSize{}, false);
  const int first = half_turn(1, Colour::White);
  std::vector<int> order{0};
  for (int k = 1; k <= kProbeTimelines; ++k) {
    order.push_back(k);
    order.push_back(-k);
  }
  for (const int tl : order) {
    NewTimeline nt;
    nt.position = tl;
    nt.first_half_turn = first;
    const int last = half_turn(tl == 0 ? kProbeHeadTurn : kProbeFarTurn, c);
    for (int ht = first; ht <= last; ++ht) nt.boards.push_back(board_for(tl, ht));
    mv.add_timeline(std::move(nt));
  }
  mv.set_to_move(c);
  return mv;
}

int run_probe(const Options& opt) {
  const auto start = std::chrono::steady_clock::now();
  const RuleOptions rules{};
  const BoardRef empty = BoardBuilder(BoardSize{}).publish();
  std::vector<KindProbe> probes;
  ProbeChecks pc;
  MoveList moves;
  MoveList scratch;

  struct PresentCheck {
    Colour colour;
    int present;
    int expected;
    Colour to_move;
    std::size_t inactive;
    int timelines;
  };
  std::vector<PresentCheck> present_checks;

  for (const Colour c : {Colour::White, Colour::Black}) {
    const Colour enemy = opposite(c);
    const int head_h = half_turn(kProbeHeadTurn, c);

    // The probe's multiverse as a game state: who is to move, the present, activity.
    {
      const GameState gs(across_multiverse(c, [&](int, int) { return empty; }), rules);
      present_checks.push_back({c, gs.present(), head_h, gs.to_move(),
                                gs.inactive_timelines().size(),
                                gs.multiverse().timeline_count()});
    }

    BoardBuilder fill(BoardSize{});
    for (int sq = 0; sq < kSquareCount; ++sq) fill.put(sq, Piece(PieceKind::Knight, enemy));
    const BoardRef full = fill.publish();
    std::vector<BoardRef> lone;
    for (int v = 0; v < kSquareCount; ++v) {
      BoardBuilder b(BoardSize{});
      b.put(v, Piece(PieceKind::Knight, enemy));
      lone.push_back(b.publish());
    }

    for (int ki = 0; ki < kPieceKindCount; ++ki) {
      const auto kind = static_cast<PieceKind>(ki);
      const bool pl = is_pawnlike(kind);
      const Piece attacker(kind, c);
      const int promo_rank = c == Colour::White ? 7 : 0;
      KindProbe kp;
      kp.kind = kind;
      kp.colour = c;

      for (int sq = 0; sq < kSquareCount; ++sq) {
        if (pl && rank_of(sq) == promo_rank) continue;  // a pawn or brawn never stands there
        BoardBuilder bb(pl ? *full : *empty);
        bb.put(sq, attacker);
        const BoardRef board = bb.publish();

        // (a) the same board.
        moves.clear();
        append_moves_of(single_board(board, c), rules, c, 0, moves);
        std::uint64_t mask = 0;
        for (const Move& m : moves) {
          if (!m.physical() || m.castle()) continue;
          if (pl && !m.capture()) continue;
          if (!pl && m.capture()) ++pc.unexpected_quiet_capture;
          mask |= square_bit(m.to_square);
        }
        kp.same_masks[static_cast<std::size_t>(sq)] = mask;
        kp.valid[static_cast<std::size_t>(sq)] = true;
        kp.same_union |= mask;

        // Capture geometry equals move geometry: a lone enemy knight on every
        // other square is captured exactly where the lone piece moves.
        if (!pl) {
          for (int v = 0; v < kSquareCount; ++v) {
            if (v == sq) continue;
            BoardBuilder vb(*lone[static_cast<std::size_t>(v)]);
            vb.put(sq, attacker);
            scratch.clear();
            append_moves_of(single_board(vb.publish(), c), rules, c, 0, scratch);
            bool captured = false;
            for (const Move& m : scratch) {
              if (m.physical() && m.capture() && m.to_square == v) captured = true;
            }
            ++pc.same_checked;
            if (captured != ((mask & square_bit(v)) != 0)) ++pc.same_mismatch;
          }
        }

        // (b) across boards.
        const BoardRef& background = pl ? full : empty;
        const Multiverse mv = across_multiverse(c, [&](int tl, int ht) {
          return (tl == 0 && ht == head_h) ? board : background;
        });
        moves.clear();
        append_moves_of(mv, rules, c, 0, moves);
        std::set<std::pair<int, int>> here;
        std::set<std::tuple<int, int, int>> reached;
        for (const Move& m : moves) {
          if (m.physical() || m.castle()) continue;
          if (pl && !m.capture()) continue;
          if (!pl && m.capture()) ++pc.unexpected_quiet_capture;
          const int step = m.to_half_turn - m.from_half_turn;
          if (step % 2 != 0) ++pc.odd_turn_step;
          const std::pair<int, int> d{m.to_timeline - m.from_timeline, step / 2};
          AcrossCell& cell = kp.across[d];
          const unsigned zbit = 1U << zone_of(m.to_square);
          cell.zones = static_cast<std::uint16_t>(cell.zones | zbit);
          std::uint16_t& from_here = cell.per_square[static_cast<std::size_t>(sq)];
          from_here = static_cast<std::uint16_t>(from_here | zbit);
          ++cell.targets;
          here.insert(d);
          reached.insert({m.to_timeline, m.to_half_turn, static_cast<int>(m.to_square)});
        }
        for (const auto& d : here) ++kp.across[d].squares;

        // Capture geometry equals move geometry across boards, at one square:
        // a lone enemy knight on every cell of every board of c's colour.
        if (!pl && sq == kVerifySquare) {
          for (int tl = -kProbeTimelines; tl <= kProbeTimelines; ++tl) {
            const Timeline* t = mv.timeline(tl);
            for (int ht = t->first_half_turn(); ht <= t->last_half_turn(); ++ht) {
              if (to_move_at(ht) != c || (tl == 0 && ht == head_h)) continue;
              for (int v = 0; v < kSquareCount; ++v) {
                const BoardRef& target = lone[static_cast<std::size_t>(v)];
                const Multiverse mv2 = across_multiverse(c, [&](int a, int b) {
                  if (a == 0 && b == head_h) return board;
                  if (a == tl && b == ht) return target;
                  return empty;
                });
                scratch.clear();
                append_moves_of(mv2, rules, c, 0, scratch);
                bool captured = false;
                for (const Move& m : scratch) {
                  if (m.capture() && m.to_timeline == tl && m.to_half_turn == ht &&
                      m.to_square == v) {
                    captured = true;
                  }
                }
                ++pc.across_checked;
                if (captured != (reached.count({tl, ht, v}) != 0)) ++pc.across_mismatch;
              }
            }
          }
        }
      }
      probes.push_back(std::move(kp));
    }
  }

  const double seconds =
      std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

  struct Summary {
    int victims = 0;
    int max_abs_dl = 0;
    int min_dtt = 0;
    int max_dtt = 0;
    bool pure_dl = false;
    bool pure_dtt = false;
    bool both = false;
  };
  const auto summarise = [](const KindProbe& kp) {
    Summary s;
    s.victims = std::popcount(kp.same_union);
    for (const auto& [d, cell] : kp.across) {
      s.max_abs_dl = std::max(s.max_abs_dl, d.first < 0 ? -d.first : d.first);
      s.min_dtt = std::min(s.min_dtt, d.second);
      s.max_dtt = std::max(s.max_dtt, d.second);
      if (d.first != 0 && d.second == 0) s.pure_dl = true;
      if (d.first == 0 && d.second != 0) s.pure_dtt = true;
      if (d.first != 0 && d.second != 0) s.both = true;
    }
    return s;
  };

  JsonWriter j;
  j.open_object();
  j.key("meta");
  j.open_object();
  j.kv("tool", "fractal-relhist --probe (tools/relhist/fractal_relhist.cpp)");
  j.kv("engine", "Temporal-5D rules engine copied at c8a9c49 (engine/, namespace t5d)");
  j.kv("rules", "RuleOptions{} (the defaults; promotion set {Queen})");
  j.kv("same_board_setup",
       "one 8x8 board, timeline 0 at T1 with the attacker's colour to move, read with "
       "append_moves_of. Non-pawn-like kinds: the attacker alone (unmoved flag off, so no "
       "castling), the destinations of its non-castling moves. Pawn and brawn: every other "
       "square holds an enemy knight; the destinations of its captures. Pawn and brawn are "
       "skipped on their promotion rank (rank 8 for White, rank 1 for Black).");
  j.kv("across_setup",
       "an odd 8x8 multiverse of timelines -4..+4, all initial (no creator, so all active): "
       "timeline 0 from T1 (white) to T5 with the attacker's colour to move, the attacker "
       "alone on its head; every other timeline from T1 to T9 with the attacker's colour to "
       "move; every board of both colours present. Non-pawn-like kinds: every other board "
       "empty, the attacker's moves off its board. Pawn and brawn: every other cell of every "
       "board holds an enemy knight, the attacker's captures off its board. dL = "
       "to_timeline - 0; dTt = (to_half_turn - from_half_turn) / 2 in whole turns.");
  j.kv("kings", "none placed: GameState's constructor and append_moves_of need no king; the "
                "multiverse is built with Multiverse::add_timeline (creator none = initial), "
                "not game_from_fen, and validated as a GameState");
  j.key("probe_bounds");
  j.open_object(false);
  j.kv_int("dL_min", -kProbeTimelines);
  j.kv_int("dL_max", kProbeTimelines);
  j.kv_int("dTt_min", 1 - kProbeHeadTurn);
  j.kv_int("dTt_max", kProbeFarTurn - kProbeHeadTurn);
  j.close_object();
  j.key("present_check");
  j.open_array(true);
  for (const PresentCheck& p : present_checks) {
    j.open_object(false);
    j.kv("colour", colour_text(p.colour));
    j.kv_int("present", p.present);
    j.kv_int("expected_present", p.expected);
    j.kv("to_move", colour_text(p.to_move));
    j.kv_uint("inactive_timelines", p.inactive);
    j.kv_int("timelines", p.timelines);
    j.kv_bool("ok", p.present == p.expected && p.to_move == p.colour && p.inactive == 0 &&
                        p.timelines == 2 * kProbeTimelines + 1);
    j.close_object();
  }
  j.close_array();
  j.key("verification");
  j.open_object();
  j.kv("claim", "for non-pawn-like kinds capture geometry equals move geometry: a lone enemy "
                "knight is captured exactly on the cells the lone attacker moves to");
  j.kv_uint("same_board_cases", pc.same_checked);
  j.kv_uint("same_board_mismatches", pc.same_mismatch);
  j.kv("across_cases_at", "attacker on d4 of timeline 0's head; a lone enemy knight on every "
                          "cell of every board of the attacker's colour");
  j.kv_uint("across_cases", pc.across_checked);
  j.kv_uint("across_mismatches", pc.across_mismatch);
  j.kv_uint("unexpected_captures_on_empty_boards", pc.unexpected_quiet_capture);
  j.kv_uint("odd_turn_steps", pc.odd_turn_step);
  j.close_object();
  j.kv_num("seconds", seconds, 3);
  j.close_object();

  j.key("legend");
  j.open_object();
  j.kv("same_board.union", "64-bit mask (hex, bit = square, a1 = 0) of victim squares reachable "
                           "from some attacker square");
  j.kv("same_board.per_square", "per attacker square 0..63, the mask of victim squares; null "
                                "where the kind never stands");
  j.kv("across.pairs", "(dL, dTt) displacements reached off the attacker's board; zones = 16-bit "
                       "mask of 2x2 target zones (rank>>1)*4 + (file>>1); attacker_squares = "
                       "how many attacker squares reach it; targets = (attacker square, target "
                       "cell) pairs; per_square_zones = per attacker square 0..63 the zone mask reached from it "
                       "(0x0000: not reached from there; null: the kind never stands there)");
  j.close_object();

  j.key("kinds");
  j.open_array(true);
  for (const KindProbe& kp : probes) {
    const Summary s = summarise(kp);
    j.open_object();
    j.kv("kind", kind_name(kp.kind));
    j.kv("colour", colour_text(kp.colour));
    j.kv_bool("pawnlike", is_pawnlike(kp.kind));
    j.key("same_board");
    j.open_object();
    j.kv("union", "0x" + hex(kp.same_union, 16));
    j.kv_int("victim_squares", s.victims);
    j.key("per_square");
    j.open_array();
    for (std::size_t sq = 0; sq < kp.same_masks.size(); ++sq) {
      if (kp.valid[sq]) {
        j.string("0x" + hex(kp.same_masks[sq], 16));
      } else {
        j.null();
      }
    }
    j.close_array();
    j.close_object();
    j.key("across");
    j.open_object();
    j.kv_int("max_abs_dL", s.max_abs_dl);
    j.kv_int("min_dTt", s.min_dtt);
    j.kv_int("max_dTt", s.max_dtt);
    j.kv_bool("pure_dL", s.pure_dl);
    j.kv_bool("pure_dTt", s.pure_dtt);
    j.kv_bool("both", s.both);
    j.key("pairs");
    j.open_array(true);
    for (const auto& [d, cell] : kp.across) {
      j.open_object(false);
      j.kv_int("dL", d.first);
      j.kv_int("dTt", d.second);
      j.kv("zones", "0x" + hex(cell.zones, 4));
      j.kv_int("zone_count", std::popcount(static_cast<unsigned>(cell.zones)));
      j.kv_int("attacker_squares", cell.squares);
      j.kv_uint("targets", cell.targets);
      j.key("per_square_zones");
      j.open_array();
      for (std::size_t sq = 0; sq < cell.per_square.size(); ++sq) {
        if (kp.valid[sq]) {
          j.string("0x" + hex(cell.per_square[sq], 4));
        } else {
          j.null();
        }
      }
      j.close_array();
      j.close_object();
    }
    j.close_array();
    j.close_object();
    j.close_object();
  }
  j.close_array();
  j.close_object();

  if (!write_file(opt.probe, j.text())) {
    std::fprintf(stderr, "fractal-relhist: cannot write %s\n", opt.probe.c_str());
    return 2;
  }

  std::printf("probe: same-board cases %llu, mismatches %llu; across cases %llu, mismatches %llu; "
              "unexpected captures %llu, odd steps %llu\n",
              static_cast<unsigned long long>(pc.same_checked),
              static_cast<unsigned long long>(pc.same_mismatch),
              static_cast<unsigned long long>(pc.across_checked),
              static_cast<unsigned long long>(pc.across_mismatch),
              static_cast<unsigned long long>(pc.unexpected_quiet_capture),
              static_cast<unsigned long long>(pc.odd_turn_step));
  for (const PresentCheck& p : present_checks) {
    std::printf("probe multiverse (%s): present h=%d (expected %d), to move %s, %zu inactive, "
                "%d timelines\n",
                colour_text(p.colour), p.present, p.expected, colour_text(p.to_move), p.inactive,
                p.timelines);
  }
  std::printf("probe bounds: dL in [%d, %d], dTt in [%d, %d]\n", -kProbeTimelines,
              kProbeTimelines, 1 - kProbeHeadTurn, kProbeFarTurn - kProbeHeadTurn);
  std::printf("%-11s %-3s %-6s %-8s %-6s %-6s %-6s %-5s %-5s\n", "kind", "col", "same", "max|dL|",
              "minTt", "maxTt", "pureL", "pureT", "both");
  for (const KindProbe& kp : probes) {
    const Summary s = summarise(kp);
    std::printf("%-11s %-3s %-6d %-8d %-6d %-6d %-6s %-5s %-5s\n", kind_name(kp.kind),
                colour_text(kp.colour), s.victims, s.max_abs_dl, s.min_dtt, s.max_dtt,
                s.pure_dl ? "yes" : "no", s.pure_dtt ? "yes" : "no", s.both ? "yes" : "no");
  }
  std::printf("probe time: %.2f s\n", seconds);
  return 0;
}

void usage() {
  std::fprintf(stderr,
               "usage: fractal-relhist --records DIR [--games FILE] [--records-out FILE] "
               "[--out FILE]\n"
               "       fractal-relhist --probe FILE\n"
               "(both modes may be given in one run)\n");
}

}  // namespace

int main(int argc, char** argv) {
  Options opt;
  for (int i = 1; i < argc; ++i) {
    const std::string_view a = argv[i];
    const auto value = [&](std::string& dst) {
      if (i + 1 >= argc) return false;
      dst = argv[++i];
      return true;
    };
    bool ok = true;
    if (a == "--records") {
      ok = value(opt.records);
    } else if (a == "--games") {
      ok = value(opt.games);
    } else if (a == "--records-out") {
      ok = value(opt.records_out);
    } else if (a == "--out") {
      ok = value(opt.out);
    } else if (a == "--probe") {
      ok = value(opt.probe);
    } else if (a == "--help" || a == "-h") {
      usage();
      return 0;
    } else {
      ok = false;
    }
    if (!ok) {
      usage();
      return 2;
    }
  }
  if (opt.records.empty() && opt.probe.empty()) {
    usage();
    return 2;
  }
  if (opt.records.empty() && (!opt.games.empty() || !opt.records_out.empty() || !opt.out.empty())) {
    std::fprintf(stderr, "fractal-relhist: --games, --records-out and --out need --records\n");
    return 2;
  }
  try {
    int rc = 0;
    if (!opt.probe.empty()) rc = run_probe(opt);
    if (rc == 0 && !opt.records.empty()) rc = run_records(opt);
    return rc;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "fractal-relhist: %s\n", e.what());
    return 1;
  }
}

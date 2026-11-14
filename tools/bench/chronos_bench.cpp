// tools/bench/chronos_bench.cpp - chronos-bench: Chronos's Phase 0 baseline speeds
// (docs/MEASUREMENTS.md), measured on the rules engine copied from Temporal-5D (spec
// 3.1; namespace t5d). New code: no tool of Temporal-5D's is copied here. It reads
// only the copied public headers (engine/include/t5d) and the copied synthetic
// player (engine/tools/synthetic.hpp), and holds no rule of chess or of 5D movement
// and no evaluation of any kind: every move, turn and verdict comes from the engine.
//
//   chronos-bench [--sections movegen,turngen,mate] [--rounds R] [--sample-ms S]
//                 [--games G] [--children C] [--turn-cap K] [--passes P]
//                 [--records DIR] [--json FILE] [--quick] [--no-pin]
//
// THREE SECTIONS, each on one thread (pinned to logical processor 2 unless --no-pin):
//
// movegen - the pseudo-legal move generation rate, defined as Temporal-5D's register
//   M9 defines its "movegen" configuration, so the figures are comparable with the
//   spec's reference (Temporal-5D docs/MEASUREMENTS.md, M9). SYNTHETIC classes - N
//   boards, T active timelines: (100,6) (160,7) (240,8) (400,10) (600,12) - of G
//   seeded random games each (t5d::synthetic, play_until_reached; a game's seed is
//   class_seed(1, N, T, g)), each stopped at the first action start with N boards.
//   The timed STREAM is the base positions' one-move children, C per base: the base's
//   pseudo-legal moves in a seeded shuffle, keeping a move only if its child can still
//   move; each child rebuilt on boards of its own (the game replayed from the start).
//   A sample generates the moves of stream positions in order, cycling, to a deadline;
//   samples are INTERLEAVED - a warm-up round, then R rounds, each measuring every
//   class (and every records bucket) once - so the machine's drifts spread over all of
//   them. Wall rate and thread CPU time per position are reported per class.
//   --records DIR adds every distinct action start of the *.5dpgn records in DIR that
//   replay to the end in parse mode (no royal-safety test at submission), bucketed by
//   board count, timed the same way (the positions themselves, not their children).
//
// turngen - the copied hypercuboid TurnGenerator (best-first whole turns), on the
//   classes' BASE positions (action starts) and the records buckets: per position, the
//   time to construct it and get the first turn, to get min(27, available) turns (27:
//   Temporal-5D's turns per search node), and to run it until done or K turns, with
//   its node count, under a budget of 2,000,000 nodes. The scorer knows no chess: a
//   move's score is a hash of its coordinates and the timeline it is offered on
//   (splitmix64, mapped to [0.001, 1)), and leaving a board scores 0.
//
// mate - the 5-timeline near-mate case (engine/tests/test_detectors.cpp,
//   detectors_the_hypercuboid_stays_small_near_mate): the hypercuboid and the naive
//   detector on timelines -n..+n for n = 1, 2, 3, and a TurnGenerator on n = 2.
//
// Every result feeds a printed sink, so the compiler cannot drop the work. --json FILE
// writes every figure, per round and per position included.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include "synthetic.hpp"
#include "t5d/move.hpp"
#include "t5d/movegen.hpp"
#include "t5d/multiverse.hpp"
#include "t5d/pgn.hpp"
#include "t5d/rules.hpp"
#include "t5d/search.hpp"
#include "t5d/state.hpp"
#include "t5d/turngen.hpp"
#include "t5d/types.hpp"
#include "t5d/variants.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__linux__)
#include <sched.h>
#include <time.h>
#endif

#if (defined(__x86_64__) || defined(__i386__)) && (defined(__GNUC__) || defined(__clang__))
#include <cpuid.h>
#define CHRONOS_BENCH_CPUID 1
#endif

#ifndef CHRONOS_COMPILER
#define CHRONOS_COMPILER "unknown"
#endif
#ifndef CHRONOS_BUILD_TYPE
#define CHRONOS_BUILD_TYPE "unknown"
#endif
#ifndef CHRONOS_CXX_FLAGS
#define CHRONOS_CXX_FLAGS "unknown"
#endif
#ifndef T5D_VERSION_STRING
#define T5D_VERSION_STRING "0.0.0"
#endif

namespace {

using Clock = std::chrono::steady_clock;

// Every timed result is added here and the total printed at the end.
std::atomic<std::uint64_t> g_sink{0};

// ---- small helpers ----------------------------------------------------------------------

std::string trim(std::string s) {
  const auto space = [](char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; };
  while (!s.empty() && space(s.back())) s.pop_back();
  std::size_t i = 0;
  while (i < s.size() && space(s[i])) ++i;
  return s.substr(i);
}

std::string fixed_text(const char* fmt, double v) {
  if (!std::isfinite(v)) return "-";
  char buf[64];
  std::snprintf(buf, sizeof buf, fmt, v);
  return buf;
}

// An integer with thousands separators: 171,804.
std::string grouped(double v) {
  if (!std::isfinite(v)) return "-";
  const bool negative = v < 0;
  const auto n = static_cast<std::uint64_t>(std::llround(std::fabs(v)));
  std::string digits = std::to_string(n);
  std::string out;
  const std::size_t len = digits.size();
  for (std::size_t i = 0; i < len; ++i) {
    if (i > 0 && (len - i) % 3 == 0) out += ',';
    out += digits[i];
  }
  return negative ? "-" + out : out;
}

double median(std::vector<double> v) {
  if (v.empty()) return std::nan("");
  std::sort(v.begin(), v.end());
  const std::size_t n = v.size();
  return n % 2 == 1 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

// The nearest-rank percentile (q in (0, 1]).
double percentile(std::vector<double> v, double q) {
  if (v.empty()) return std::nan("");
  std::sort(v.begin(), v.end());
  const auto rank = static_cast<std::size_t>(std::ceil(q * static_cast<double>(v.size())));
  return v[std::min(v.size(), std::max<std::size_t>(rank, 1)) - 1];
}

double min_of(const std::vector<double>& v) {
  return v.empty() ? std::nan("") : *std::min_element(v.begin(), v.end());
}
double max_of(const std::vector<double>& v) {
  return v.empty() ? std::nan("") : *std::max_element(v.begin(), v.end());
}

double seconds_between(Clock::time_point a, Clock::time_point b) {
  return std::chrono::duration<double>(b - a).count();
}

// splitmix64's finaliser.
std::uint64_t mix64(std::uint64_t z) noexcept {
  z += 0x9E3779B97F4A7C15ull;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  return z ^ (z >> 31);
}

std::uint64_t u64_of(std::int64_t v) noexcept { return static_cast<std::uint64_t>(v); }

const char* outcome_name(t5d::Outcome o) noexcept {
  switch (o) {
    case t5d::Outcome::Unknown:
      return "unknown";
    case t5d::Outcome::Ongoing:
      return "none";
    case t5d::Outcome::Checkmate:
      return "checkmate";
    case t5d::Outcome::Stalemate:
      return "stalemate";
  }
  return "?";
}

// ---- JSON ---------------------------------------------------------------------------------

class Json {
 public:
  Json& open_object() { return open('{'); }
  Json& open_array() { return open('['); }
  Json& close_object() { return close('}'); }
  Json& close_array() { return close(']'); }
  Json& key(std::string_view k) {
    separate();
    quote(k);
    out_ += ':';
    after_key_ = true;
    return *this;
  }
  Json& str(std::string_view v) {
    separate();
    quote(v);
    return *this;
  }
  Json& num(double v) {
    separate();
    if (!std::isfinite(v)) {
      out_ += "null";
    } else {
      char buf[40];
      std::snprintf(buf, sizeof buf, "%.10g", v);
      out_ += buf;
    }
    return *this;
  }
  Json& unum(std::uint64_t v) {
    separate();
    out_ += std::to_string(v);
    return *this;
  }
  Json& sint(std::int64_t v) {
    separate();
    out_ += std::to_string(v);
    return *this;
  }
  Json& boolean(bool v) {
    separate();
    out_ += v ? "true" : "false";
    return *this;
  }
  Json& nums(const std::vector<double>& v) {
    open_array();
    for (const double x : v) num(x);
    return close_array();
  }
  const std::string& text() const noexcept { return out_; }

 private:
  Json& open(char c) {
    separate();
    out_ += c;
    fresh_.push_back(true);
    return *this;
  }
  Json& close(char c) {
    out_ += c;
    if (!fresh_.empty()) fresh_.pop_back();
    return *this;
  }
  void separate() {
    if (after_key_) {
      after_key_ = false;
      return;
    }
    if (fresh_.empty()) return;
    if (!fresh_.back()) out_ += ',';
    fresh_.back() = false;
  }
  void quote(std::string_view s) {
    out_ += '"';
    for (const char ch : s) {
      const auto c = static_cast<unsigned char>(ch);
      switch (ch) {
        case '"':
          out_ += "\\\"";
          break;
        case '\\':
          out_ += "\\\\";
          break;
        case '\n':
          out_ += "\\n";
          break;
        case '\r':
          out_ += "\\r";
          break;
        case '\t':
          out_ += "\\t";
          break;
        default:
          if (c < 0x20) {
            char buf[8];
            std::snprintf(buf, sizeof buf, "\\u%04x", static_cast<unsigned>(c));
            out_ += buf;
          } else {
            out_ += ch;
          }
      }
    }
    out_ += '"';
  }

  std::string out_;
  std::vector<bool> fresh_;
  bool after_key_ = false;
};

// ---- the machine ------------------------------------------------------------------------

std::string cpu_brand() {
#if defined(CHRONOS_BENCH_CPUID)
  unsigned a = 0, b = 0, c = 0, d = 0;
  if (__get_cpuid(0x80000000u, &a, &b, &c, &d) == 0 || a < 0x80000004u) return "unknown";
  char brand[49] = {};
  for (unsigned i = 0; i < 3; ++i) {
    unsigned r[4] = {0, 0, 0, 0};
    if (__get_cpuid(0x80000002u + i, &r[0], &r[1], &r[2], &r[3]) == 0) return "unknown";
    std::memcpy(brand + 16 * i, r, 16);
  }
  const std::string text = trim(brand);
  return text.empty() ? "unknown" : text;
#else
  return "unknown";
#endif
}

// This thread's CPU time (user + kernel), in seconds; negative where the platform
// does not say. Windows: GetThreadTimes, which advances in clock ticks (about
// 15.6 ms by default), so a short sample's figure is coarse.
double thread_cpu_seconds() {
#if defined(_WIN32)
  FILETIME created, exited, kernel, user;
  if (GetThreadTimes(GetCurrentThread(), &created, &exited, &kernel, &user) == 0) return -1.0;
  const auto ticks = [](const FILETIME& f) {
    return (static_cast<std::uint64_t>(f.dwHighDateTime) << 32) |
           static_cast<std::uint64_t>(f.dwLowDateTime);
  };
  return static_cast<double>(ticks(kernel) + ticks(user)) * 1e-7;
#elif defined(__linux__)
  timespec ts{};
  if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) != 0) return -1.0;
  return static_cast<double>(ts.tv_sec) + static_cast<double>(ts.tv_nsec) * 1e-9;
#else
  return -1.0;
#endif
}

const char* thread_cpu_method() {
#if defined(_WIN32)
  return "GetThreadTimes";
#elif defined(__linux__)
  return "clock_gettime(CLOCK_THREAD_CPUTIME_ID)";
#else
  return "none";
#endif
}

struct Pinning {
  bool requested = false;
  bool pinned = false;
  int cpu = -1;
  bool priority_raised = false;
};

// Pins the calling thread to logical processor 2 (0 on a machine with fewer than
// three) and, where it is one call, raises its priority a step.
Pinning pin_this_thread(bool want) {
  Pinning p;
  p.requested = want;
  if (!want) return p;
  const unsigned n = std::thread::hardware_concurrency();
  const int cpu = (n > 2 || n == 0) ? 2 : 0;
#if defined(_WIN32)
  const DWORD_PTR mask = DWORD_PTR{1} << cpu;
  if (SetThreadAffinityMask(GetCurrentThread(), mask) != 0) {
    p.pinned = true;
    p.cpu = cpu;
  }
  p.priority_raised = SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL) != 0;
#elif defined(__linux__) && defined(CPU_SET)
  cpu_set_t set;
  CPU_ZERO(&set);
  const auto index = static_cast<std::size_t>(cpu);
  (void)CPU_SET(index, &set);
  if (sched_setaffinity(0, sizeof set, &set) == 0) {
    p.pinned = true;
    p.cpu = cpu;
  }
#else
  (void)cpu;
#endif
  return p;
}

// ---- options ----------------------------------------------------------------------------

struct Options {
  bool movegen = true;
  bool turngen = true;
  bool mate = true;
  int rounds = 15;
  int sample_ms = 200;
  int games = 16;
  int children = 6;
  int turn_cap = 1000;
  int passes = 3;
  std::string records;
  std::string json;
  bool quick = false;
  bool pin = true;
  // Fixed by the measurement's definition; printed, not options.
  std::uint64_t seed = 1;
  int branch_percent = 25;
  std::uint64_t turn_nodes = 2'000'000;
  int turns_per_node = 27;
};

const std::vector<std::pair<int, int>> kClasses = {{100, 6}, {160, 7}, {240, 8}, {400, 10}, {600, 12}};

void usage(std::FILE* f) {
  std::fputs(
      "chronos-bench: Chronos's Phase 0 baseline speeds (one thread).\n"
      "  --sections LIST   movegen,turngen,mate (default: all)\n"
      "  --rounds R        movegen: interleaved rounds after the warm-up (default 15)\n"
      "  --sample-ms S     movegen: the length of a sample (default 200)\n"
      "  --games G         synthetic games per class (default 16)\n"
      "  --children C      movegen: one-move children per base position (default 6)\n"
      "  --turn-cap K      turngen: turns asked of a generator at most (default 1000)\n"
      "  --passes P        turngen: passes over the positions (default 3; median kept)\n"
      "  --records DIR     also every distinct action start of DIR/*.5dpgn, by board count\n"
      "  --json FILE       write every figure as JSON\n"
      "  --quick           smoke test: games 2, rounds 2, sample-ms 50, passes 1\n"
      "  --no-pin          do not pin the thread to a logical processor\n",
      f);
}

int positive_int(std::string_view name, const char* text) {
  char* end = nullptr;
  const long v = std::strtol(text, &end, 10);
  if (end == text || *end != '\0' || v < 1 || v > 1'000'000'000L) {
    throw std::invalid_argument(std::string(name) + ": expected a positive integer, got '" + text + "'");
  }
  return static_cast<int>(v);
}

Options parse_options(int argc, char** argv) {
  Options o;
  // --quick first, so that explicit options given with it override its values.
  for (int i = 1; i < argc; ++i) {
    if (std::string_view(argv[i]) == "--quick") {
      o.quick = true;
      o.games = 2;
      o.rounds = 2;
      o.sample_ms = 50;
      o.passes = 1;
    }
  }
  for (int i = 1; i < argc; ++i) {
    const std::string_view a = argv[i];
    const auto value = [&]() -> const char* {
      if (i + 1 >= argc) throw std::invalid_argument(std::string(a) + ": missing value");
      return argv[++i];
    };
    if (a == "--quick") {
      continue;
    } else if (a == "--help" || a == "-h") {
      usage(stdout);
      std::exit(0);
    } else if (a == "--no-pin") {
      o.pin = false;
    } else if (a == "--sections") {
      o.movegen = o.turngen = o.mate = false;
      std::stringstream list(value());
      std::string item;
      while (std::getline(list, item, ',')) {
        item = trim(item);
        if (item == "movegen") {
          o.movegen = true;
        } else if (item == "turngen") {
          o.turngen = true;
        } else if (item == "mate") {
          o.mate = true;
        } else if (item == "all") {
          o.movegen = o.turngen = o.mate = true;
        } else if (!item.empty()) {
          throw std::invalid_argument("--sections: unknown section '" + item + "'");
        }
      }
    } else if (a == "--rounds") {
      o.rounds = positive_int(a, value());
    } else if (a == "--sample-ms") {
      o.sample_ms = positive_int(a, value());
    } else if (a == "--games") {
      o.games = positive_int(a, value());
    } else if (a == "--children") {
      o.children = positive_int(a, value());
    } else if (a == "--turn-cap") {
      o.turn_cap = positive_int(a, value());
    } else if (a == "--passes") {
      o.passes = positive_int(a, value());
    } else if (a == "--records") {
      o.records = value();
    } else if (a == "--json") {
      o.json = value();
    } else {
      throw std::invalid_argument("unknown option '" + std::string(a) + "' (--help lists them)");
    }
  }
  return o;
}

// ---- positions ----------------------------------------------------------------------------

std::uint64_t class_seed(std::uint64_t seed, int boards, int active, int game) {
  return seed + std::uint64_t{1000003} * static_cast<std::uint64_t>(boards) +
         std::uint64_t{10007} * static_cast<std::uint64_t>(active) +
         std::uint64_t{37} * static_cast<std::uint64_t>(game);
}

int active_timelines(const t5d::GameState& s) {
  int n = 0;
  for (const t5d::TimelineRef& t : s.multiverse().timelines()) {
    if (s.active(t->position())) ++n;
  }
  return n;
}

// A class (synthetic) or a bucket (records): its base positions (action starts) and,
// for movegen, the stream it times.
struct PositionSet {
  std::string name;
  std::string kind;  // "synthetic" or "records"
  int boards = 0;    // synthetic: N; records: the bucket's floor
  int boards_hi = 0; // records: the bucket's last board count (0: open)
  int active = 0;    // synthetic: T
  int games = 0;     // synthetic: games used; records: records contributing
  int games_skipped = 0;
  std::vector<std::uint64_t> seeds;
  std::vector<t5d::GameState> bases;
  std::vector<t5d::GameState> stream;
  std::size_t bases_without_child = 0;  // movegen: a base timed itself (no usable child)
  // The stream described.
  double mean_moves = 0;
  double mean_boards = 0;
  double mean_active = 0;
  // movegen results, one per round.
  std::vector<double> rates;
  std::vector<double> cpu_ns;
  std::vector<std::uint64_t> sample_positions;
  // Thread CPU time pooled over the rounds (the clock's granularity averages out).
  double cpu_seconds_total = 0;
  std::uint64_t cpu_positions_total = 0;
  std::size_t cursor = 0;

  double cpu_ns_pooled() const {
    return cpu_positions_total > 0 ? cpu_seconds_total * 1e9 / static_cast<double>(cpu_positions_total)
                                   : std::nan("");
  }
};

void describe_stream(PositionSet& set) {
  t5d::MoveList moves;
  double m = 0, b = 0, a = 0;
  for (const t5d::GameState& s : set.stream) {
    t5d::generate_moves(s, moves);
    m += static_cast<double>(moves.size());
    b += static_cast<double>(s.multiverse().board_count());
    a += static_cast<double>(active_timelines(s));
  }
  const double n = static_cast<double>(std::max<std::size_t>(1, set.stream.size()));
  set.mean_moves = m / n;
  set.mean_boards = b / n;
  set.mean_active = a / n;
}

// Up to `count` moves of `base` whose children can still move, in a shuffle of the
// pseudo-legal moves fixed by `key` (Fisher-Yates from the back).
std::vector<t5d::Move> choose_children(const t5d::GameState& base, std::uint64_t key,
                                       std::size_t count) {
  std::vector<t5d::Move> chosen;
  t5d::MoveList moves;
  t5d::generate_moves(base, moves);
  std::vector<std::size_t> order(moves.size());
  for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
  t5d::synthetic::Rng rng(key);
  for (std::size_t i = order.size(); i > 1; --i) std::swap(order[i - 1], order[rng.below(i)]);
  t5d::MoveList scratch;
  for (const std::size_t i : order) {
    if (chosen.size() >= count) break;
    t5d::GameState child = base;
    child.apply(moves[i]);
    if (t5d::count_moves(child, scratch) > 0) chosen.push_back(moves[i]);
  }
  return chosen;
}

std::vector<PositionSet> synthetic_classes(const Options& opt, bool build_streams) {
  std::vector<PositionSet> out;
  const t5d::Variant* standard = t5d::find_variant("Standard");
  if (standard == nullptr) throw std::runtime_error("the engine does not set up Standard");
  for (const auto& [n, t] : kClasses) {
    PositionSet set;
    set.name = "N" + std::to_string(n) + " T" + std::to_string(t);
    set.kind = "synthetic";
    set.boards = n;
    set.active = t;
    const Clock::time_point t0 = Clock::now();
    for (int g = 0; g < opt.games; ++g) {
      t5d::synthetic::Spec spec;
      spec.variant = "Standard";
      spec.rules = {};
      spec.seed = class_seed(opt.seed, n, t, g);
      spec.target_active = t;
      spec.branch_percent = opt.branch_percent;
      spec.target_boards = n;
      const t5d::synthetic::Game game = t5d::synthetic::play_until_reached(spec);
      if (game.stop != t5d::synthetic::Stop::Target) {
        std::fprintf(stderr, "chronos-bench: synthetic %s game %d: no game reached %d boards (seed %llu: %s); skipped\n",
                     set.name.c_str(), g, n, static_cast<unsigned long long>(game.seed),
                     t5d::synthetic::stop_name(game.stop));
        ++set.games_skipped;
        continue;
      }
      ++set.games;
      set.seeds.push_back(game.seed);
      set.bases.push_back(game.state);
      if (!build_streams) continue;
      // The children, each on boards of its own: the game replayed, then the move.
      const std::vector<t5d::Move> chosen =
          choose_children(game.state, mix64(game.seed ^ 0x6368726F6E6F73ull),
                          static_cast<std::size_t>(opt.children));
      const t5d::GameState own = t5d::synthetic::replay(t5d::variant_start(*standard, spec.rules),
                                                        game.actions, game.actions.size());
      if (own.zobrist() != game.state.zobrist()) {
        throw std::runtime_error("synthetic replay of " + set.name + " seed " +
                                 std::to_string(game.seed) + " did not reach the game's position");
      }
      for (const t5d::Move& m : chosen) {
        t5d::GameState child = own;
        child.apply(m);
        set.stream.push_back(std::move(child));
      }
      if (chosen.empty()) {
        set.stream.push_back(own);  // no child can move: the position itself
        ++set.bases_without_child;
      }
    }
    std::fprintf(stderr, "chronos-bench: synthetic %s: %d games (%.1f s)\n", set.name.c_str(),
                 set.games, seconds_between(t0, Clock::now()));
    out.push_back(std::move(set));
  }
  return out;
}

struct RecordsSummary {
  std::size_t files = 0;
  std::size_t unreadable = 0;  // read_pgn set `unreadable`
  std::size_t refused = 0;     // the set-up or a play_next threw
  std::size_t replayed = 0;    // replayed to the end
  std::size_t action_starts = 0;
  std::size_t distinct = 0;
  std::vector<std::string> refusals;  // "file: fault: message"
};

std::string read_whole_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("cannot read " + path.string());
  std::ostringstream text;
  text << in.rdbuf();
  return text.str();
}

struct Bucket {
  const char* name;
  int lo;
  int hi;  // 0: open
};
constexpr Bucket kBuckets[] = {{"<50", 0, 49}, {"50-99", 50, 99}, {"100-199", 100, 199}, {"200+", 200, 0}};

std::vector<PositionSet> records_buckets(const std::string& dir, RecordsSummary& summary) {
  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".5dpgn") files.push_back(entry.path());
  }
  std::sort(files.begin(), files.end(),
            [](const auto& a, const auto& b) { return a.filename().string() < b.filename().string(); });
  summary.files = files.size();

  std::vector<PositionSet> buckets;
  for (const Bucket& b : kBuckets) {
    PositionSet set;
    set.name = std::string("records ") + b.name;
    set.kind = "records";
    set.boards = b.lo;
    set.boards_hi = b.hi;
    buckets.push_back(std::move(set));
  }
  const auto bucket_of = [](std::size_t boards) {
    for (std::size_t i = 0; i < std::size(kBuckets); ++i) {
      if (kBuckets[i].hi == 0 || boards <= static_cast<std::size_t>(kBuckets[i].hi)) return i;
    }
    return std::size(kBuckets) - 1;
  };

  struct KeyHash {
    std::size_t operator()(const std::pair<std::uint64_t, int>& k) const noexcept {
      return static_cast<std::size_t>(mix64(k.first ^ static_cast<std::uint64_t>(k.second)));
    }
  };
  std::unordered_set<std::pair<std::uint64_t, int>, KeyHash> seen;
  for (const std::filesystem::path& path : files) {
    const std::string name = path.filename().string();
    t5d::PgnRecord record = t5d::read_pgn(read_whole_file(path));
    if (record.unreadable) {
      ++summary.unreadable;
      summary.refusals.push_back(name + ": unreadable: " + *record.unreadable);
      continue;
    }
    std::vector<t5d::GameState> starts;
    try {
      t5d::PgnReplay replay(std::move(record));
      replay.set_check_ruling(t5d::CheckRuling::none());
      for (;;) {
        starts.push_back(replay.state());
        if (replay.finished()) break;
        replay.play_next();
      }
    } catch (const t5d::RecordError& e) {
      ++summary.refused;
      summary.refusals.push_back(name + ": " + t5d::fault_name(e.fault()) + ": " + e.what());
      continue;
    } catch (const std::exception& e) {  // not documented to happen; refused all the same
      ++summary.refused;
      summary.refusals.push_back(name + ": error: " + e.what());
      continue;
    }
    ++summary.replayed;
    std::vector<bool> contributed(buckets.size(), false);
    for (t5d::GameState& s : starts) {
      ++summary.action_starts;
      const std::pair<std::uint64_t, int> key{s.zobrist(), t5d::index_of(s.to_move())};
      if (!seen.insert(key).second) continue;
      const std::size_t i = bucket_of(s.multiverse().board_count());
      contributed[i] = true;
      buckets[i].bases.push_back(std::move(s));
    }
    for (std::size_t i = 0; i < buckets.size(); ++i) buckets[i].games += contributed[i] ? 1 : 0;
  }
  summary.distinct = seen.size();
  std::vector<PositionSet> out;
  for (PositionSet& set : buckets) {
    if (set.bases.empty()) continue;
    set.stream = set.bases;  // records are timed as they stand (shared boards: one thread)
    out.push_back(std::move(set));
  }
  return out;
}

// ---- movegen ------------------------------------------------------------------------------

struct Sample {
  std::uint64_t positions = 0;
  double wall = 0;
  double cpu = -1;
};

Sample time_movegen(PositionSet& set, t5d::MoveList& moves, std::chrono::milliseconds length) {
  Sample s;
  const std::vector<t5d::GameState>& stream = set.stream;
  const std::size_t n = stream.size();
  if (n == 0) return s;
  std::size_t j = set.cursor;
  std::uint64_t done = 0;
  std::uint64_t sink = 0;
  const double c0 = thread_cpu_seconds();
  const Clock::time_point t0 = Clock::now();
  const Clock::time_point deadline = t0 + length;
  do {
    t5d::generate_moves(stream[j], moves);
    sink += moves.size();
    ++done;
    if (++j == n) j = 0;
  } while (Clock::now() < deadline);
  const Clock::time_point t1 = Clock::now();
  const double c1 = thread_cpu_seconds();
  set.cursor = j;
  g_sink.fetch_add(sink, std::memory_order_relaxed);
  s.positions = done;
  s.wall = seconds_between(t0, t1);
  s.cpu = c0 >= 0 && c1 >= 0 ? c1 - c0 : -1.0;
  return s;
}

void run_movegen(const Options& opt, std::vector<PositionSet*>& sets) {
  t5d::MoveList moves;
  const std::chrono::milliseconds length(opt.sample_ms);
  // The warm-up round, discarded.
  for (PositionSet* set : sets) (void)time_movegen(*set, moves, length);
  for (int r = 0; r < opt.rounds; ++r) {
    for (PositionSet* set : sets) {
      const Sample s = time_movegen(*set, moves, length);
      set->sample_positions.push_back(s.positions);
      set->rates.push_back(s.wall > 0 ? static_cast<double>(s.positions) / s.wall : std::nan(""));
      set->cpu_ns.push_back(s.cpu >= 0 && s.positions > 0
                                ? s.cpu * 1e9 / static_cast<double>(s.positions)
                                : std::nan(""));
      if (s.cpu >= 0) {
        set->cpu_seconds_total += s.cpu;
        set->cpu_positions_total += s.positions;
      }
    }
  }
}

void print_movegen(const std::vector<PositionSet*>& sets) {
  std::printf("\n== movegen: pseudo-legal generate_moves, one thread ==\n");
  std::printf("positions/s: wall, median [min, max] over rounds; CPU ns/pos: thread CPU time per\n"
              "position, median over rounds and pooled over all rounds; moves, boards, active: means\n"
              "over the timed positions\n");
  std::printf("%-17s %6s %7s %14s %24s %10s %8s %8s %8s %7s\n", "class/bucket", "games", "stream",
              "positions/s", "[min, max] over rounds", "CPU ns/pos", "(pooled)", "moves", "boards",
              "active");
  for (const PositionSet* set : sets) {
    std::printf("%-17s %6d %7zu %14s %24s %10s %8s %8.1f %8.1f %7.2f\n", set->name.c_str(),
                set->games, set->stream.size(), grouped(median(set->rates)).c_str(),
                ("[" + grouped(min_of(set->rates)) + ", " + grouped(max_of(set->rates)) + "]").c_str(),
                grouped(median(set->cpu_ns)).c_str(), grouped(set->cpu_ns_pooled()).c_str(),
                set->mean_moves, set->mean_boards, set->mean_active);
  }
}

// ---- turngen ------------------------------------------------------------------------------

// The scores of a turn's options: no chess knowledge, deterministic, effectively
// tie-free. A move's score is a splitmix64 hash of its coordinates (source and
// destination timeline, half-turn and square), its promotion kind, its branching
// flag and the timeline it is offered on, mapped to [0.001, 1.0); leaving a board
// unplayed scores 0.
class HashScorer final : public t5d::TurnScorer {
 public:
  double leave(int /*timeline*/) const override { return 0.0; }
  double move(const t5d::Move& m, int timeline) const override {
    std::uint64_t h = 0x43687230626E6368ull;
    const auto feed = [&h](std::int64_t v) { h = mix64(h ^ u64_of(v)); };
    feed(m.from_timeline);
    feed(m.from_half_turn);
    feed(m.to_timeline);
    feed(m.to_half_turn);
    feed(m.from_square);
    feed(m.to_square);
    feed(m.promotes() ? static_cast<std::int64_t>(t5d::index_of(m.promotion)) + 1 : 0);
    feed(m.branching() ? 1 : 0);
    feed(timeline);
    const double unit = static_cast<double>(mix64(h) >> 11) * 0x1.0p-53;  // [0, 1)
    return 0.001 + 0.999 * unit;
  }
};

constexpr const char* kScorerText =
    "move(m, timeline) = 0.001 + 0.999 * u, u = splitmix64 chain over (from_timeline, "
    "from_half_turn, to_timeline, to_half_turn, from_square, to_square, promotion kind + 1 "
    "or 0, branching flag, timeline) >> 11 * 2^-53; leave(timeline) = 0";

struct TurngenRun {
  double first_us = 0;  // construction + first next()
  double to_k27_us = 0; // until min(27, available) turns (or done, if fewer)
  double done_us = 0;   // until nullopt or K turns
  std::uint64_t turns = 0;
  std::uint64_t nodes = 0;
  bool exhausted = false;
  bool stopped = false;
  bool refused = false; // the generator threw (no turn start)
  std::uint64_t sink = 0;
};

TurngenRun run_generator(const t5d::GameState& s, const t5d::TurnScorer& scorer, const Options& opt) {
  TurngenRun r;
  t5d::SearchBudget budget = t5d::SearchBudget::nodes(opt.turn_nodes);
  const auto k27 = static_cast<std::uint64_t>(opt.turns_per_node);
  const auto cap = static_cast<std::uint64_t>(opt.turn_cap);
  const Clock::time_point t0 = Clock::now();
  try {
    t5d::TurnGenerator gen(s, scorer, t5d::TurnUnit::Successors, budget);
    bool k27_seen = false;
    for (;;) {
      std::optional<t5d::GeneratedTurn> turn = gen.next();
      const Clock::time_point now = Clock::now();
      if (r.turns == 0) r.first_us = seconds_between(t0, now) * 1e6;
      if (!turn) {
        if (!k27_seen) r.to_k27_us = seconds_between(t0, now) * 1e6;
        r.done_us = seconds_between(t0, now) * 1e6;
        break;
      }
      ++r.turns;
      r.sink += turn->moves.size() + static_cast<std::uint64_t>(turn->result.present() & 0xFF);
      if (r.turns == k27) {
        r.to_k27_us = seconds_between(t0, now) * 1e6;
        k27_seen = true;
      }
      if (r.turns >= cap) {
        if (!k27_seen) r.to_k27_us = seconds_between(t0, now) * 1e6;
        r.done_us = seconds_between(t0, now) * 1e6;
        break;
      }
    }
    r.nodes = gen.stats().nodes;
    r.exhausted = gen.exhausted();
    r.stopped = gen.stopped();
  } catch (const std::invalid_argument&) {
    r.refused = true;
    r.first_us = r.to_k27_us = r.done_us = seconds_between(t0, Clock::now()) * 1e6;
  }
  return r;
}

struct TurngenPosition {
  std::vector<double> first_us, to_k27_us, done_us;  // one per pass
  TurngenRun last;  // the deterministic figures (the same in every pass)
  bool consistent = true;
};

struct TurngenSet {
  const PositionSet* set = nullptr;
  std::vector<TurngenPosition> positions;
  double wall_s = 0;
};

void run_turngen(const Options& opt, std::vector<TurngenSet>& sets) {
  const HashScorer scorer;
  for (TurngenSet& ts : sets) ts.positions.assign(ts.set->bases.size(), TurngenPosition{});
  for (int pass = 0; pass < opt.passes; ++pass) {
    for (TurngenSet& ts : sets) {
      const Clock::time_point t0 = Clock::now();
      for (std::size_t i = 0; i < ts.set->bases.size(); ++i) {
        const TurngenRun r = run_generator(ts.set->bases[i], scorer, opt);
        g_sink.fetch_add(r.sink, std::memory_order_relaxed);
        TurngenPosition& p = ts.positions[i];
        if (pass > 0 && (r.turns != p.last.turns || r.nodes != p.last.nodes ||
                         r.exhausted != p.last.exhausted || r.stopped != p.last.stopped)) {
          p.consistent = false;
        }
        p.first_us.push_back(r.first_us);
        p.to_k27_us.push_back(r.to_k27_us);
        p.done_us.push_back(r.done_us);
        p.last = r;
      }
      ts.wall_s += seconds_between(t0, Clock::now());
      std::fprintf(stderr, "chronos-bench: turngen pass %d/%d %s: %zu positions\n", pass + 1,
                   opt.passes, ts.set->name.c_str(), ts.set->bases.size());
    }
  }
}

struct TurngenSummary {
  std::size_t positions = 0;
  std::size_t refused = 0;
  double first_median = 0, first_p90 = 0;
  double k27_median = 0;
  double us_per_turn_median = 0;
  double turns_median = 0;
  std::size_t hit_cap = 0;
  std::size_t stopped = 0;
  std::size_t exhausted = 0;
  std::size_t no_turn = 0;  // exhausted with no turn: no legal turn (mate or stalemate)
  std::size_t inconsistent = 0;
  std::uint64_t turns_total = 0;
  double seconds_total = 0;
  double turns_per_second = 0;
  std::uint64_t nodes_total = 0;
};

TurngenSummary summarise(const TurngenSet& ts, const Options& opt) {
  TurngenSummary s;
  std::vector<double> first, k27, per_turn, turns;
  for (const TurngenPosition& p : ts.positions) {
    ++s.positions;
    if (p.last.refused) {
      ++s.refused;
      continue;
    }
    const double f = median(p.first_us);
    const double k = median(p.to_k27_us);
    const double d = median(p.done_us);
    first.push_back(f);
    k27.push_back(k);
    turns.push_back(static_cast<double>(p.last.turns));
    if (p.last.turns > 0) per_turn.push_back(d / static_cast<double>(p.last.turns));
    if (p.last.turns >= static_cast<std::uint64_t>(opt.turn_cap)) ++s.hit_cap;
    if (p.last.stopped) ++s.stopped;
    if (p.last.exhausted) ++s.exhausted;
    if (p.last.exhausted && p.last.turns == 0) ++s.no_turn;
    if (!p.consistent) ++s.inconsistent;
    s.turns_total += p.last.turns;
    s.nodes_total += p.last.nodes;
    s.seconds_total += d * 1e-6;
  }
  s.first_median = median(first);
  s.first_p90 = percentile(first, 0.9);
  s.k27_median = median(k27);
  s.us_per_turn_median = median(per_turn);
  s.turns_median = median(turns);
  s.turns_per_second = s.seconds_total > 0 ? static_cast<double>(s.turns_total) / s.seconds_total : 0;
  return s;
}

void print_turngen(const std::vector<TurngenSet>& sets, const Options& opt) {
  std::printf("\n== turngen: hypercuboid TurnGenerator (Successors), one thread ==\n");
  std::printf("scorer: %s\n", kScorerText);
  std::printf("budget %s nodes; at most K = %d turns; %d pass(es), median time per position\n",
              grouped(static_cast<double>(opt.turn_nodes)).c_str(), opt.turn_cap, opt.passes);
  std::printf(
      "first: construction + first next(); 27 turns: until min(27, available) turns (until done\n"
      "if fewer); us/turn: median over positions of (time to done or K) / turns; K cap, budget,\n"
      "exh.: positions that hit K, that the node budget stopped, that were exhausted; none:\n"
      "exhausted with no turn (no legal turn: synthetic play is pseudo-legal, so a synthetic\n"
      "base may have none); turns/s: all turns / all time\n");
  std::printf("%-17s %6s %11s %11s %11s %10s %8s %6s %6s %6s %6s %12s\n", "class/bucket", "pos",
              "first us", "first p90", "27 turns us", "us/turn", "turns", "K cap", "budget",
              "exh.", "none", "turns/s");
  for (const TurngenSet& ts : sets) {
    const TurngenSummary s = summarise(ts, opt);
    std::printf("%-17s %6zu %11s %11s %11s %10s %8s %6zu %6zu %6zu %6zu %12s\n",
                ts.set->name.c_str(), s.positions, fixed_text("%.1f", s.first_median).c_str(),
                fixed_text("%.1f", s.first_p90).c_str(), fixed_text("%.1f", s.k27_median).c_str(),
                fixed_text("%.2f", s.us_per_turn_median).c_str(), fixed_text("%.0f", s.turns_median).c_str(),
                s.hit_cap, s.stopped, s.exhausted, s.no_turn, grouped(s.turns_per_second).c_str());
    if (s.refused > 0) std::printf("  %zu position(s) refused by the generator (no turn start)\n", s.refused);
    if (s.inconsistent > 0) {
      std::printf("  WARNING: %zu position(s) gave different turns/nodes across passes\n", s.inconsistent);
    }
  }
}

// ---- mate -----------------------------------------------------------------------------------

// engine/tests/test_detectors.cpp, detectors_the_hypercuboid_stays_small_near_mate:
// timelines -n..+n at T1, Black to move; a checkmate on 0, a useless knight elsewhere.
t5d::GameState mate_position(int n) {
  std::vector<std::string> boards;
  for (int p = -n; p <= n; ++p) {
    const std::string label = p == 0 ? "0" : (p > 0 ? "+" : "") + std::to_string(p);
    boards.push_back("[" + std::string(p == 0 ? "k4/5/5/5/RR3" : "5/5/4n/5/RR3") + ":" + label +
                     ":1:b]");
  }
  return t5d::game_from_fen(boards, t5d::BoardSize{5, 5}, t5d::Parity::Infer);
}

struct MateCase {
  int n = 0;
  std::string detector;
  int calls = 0;
  double median_us = 0;
  double min_us = 0;
  double max_us = 0;
  std::uint64_t nodes = 0;
  std::string verdict;
  bool exhaustive = false;
  bool stopped_by_budget = false;
  bool stopped_by_memory = false;
  bool in_check = false;
  bool expect_checkmate = false;
  bool ok = true;
};

MateCase time_mate(int n, t5d::Detector detector, int min_calls, double min_seconds) {
  const t5d::GameState s = mate_position(n);
  MateCase c;
  c.n = n;
  c.detector = t5d::detector_name(detector);
  std::vector<double> us;
  const Clock::time_point start = Clock::now();
  t5d::MateResult last;
  do {
    const Clock::time_point t0 = Clock::now();
    last = t5d::detect_mate(s, detector);
    const Clock::time_point t1 = Clock::now();
    us.push_back(seconds_between(t0, t1) * 1e6);
    g_sink.fetch_add(last.stats.nodes, std::memory_order_relaxed);
  } while (static_cast<int>(us.size()) < min_calls || seconds_between(start, Clock::now()) < min_seconds);
  c.calls = static_cast<int>(us.size());
  c.median_us = median(us);
  c.min_us = min_of(us);
  c.max_us = max_of(us);
  c.nodes = last.stats.nodes;
  c.verdict = outcome_name(last.verdict);
  c.exhaustive = last.stats.exhaustive;
  c.stopped_by_budget = last.stats.stopped_by_budget;
  c.stopped_by_memory = last.stats.stopped_by_memory;
  c.in_check = last.in_check;
  return c;
}

struct MateTurngen {
  std::uint64_t turns = 0;
  bool exhausted = false;
  bool stopped = false;
  std::uint64_t nodes = 0;
  double median_us = 0;
  int calls = 0;
  bool ok = true;
};

struct MateResults {
  std::vector<MateCase> cases;
  MateTurngen turngen;
  bool ok = true;
};

MateResults run_mate(const Options& opt) {
  MateResults out;
  for (int n = 1; n <= 3; ++n) {
    MateCase c = time_mate(n, t5d::Detector::Hypercuboid, 5, 0.5);
    c.expect_checkmate = true;
    c.ok = c.verdict == "checkmate" && c.exhaustive;
    out.cases.push_back(c);
    std::fprintf(stderr, "chronos-bench: mate n=%d hypercuboid done\n", n);
  }
  for (int n = 1; n <= 3; ++n) {
    MateCase c = time_mate(n, t5d::Detector::Naive, n <= 2 ? 3 : 1, 0.0);
    // n = 1, 2: a proof expected; n = 3: the memory ceiling, Unknown (never a wrong verdict).
    c.expect_checkmate = n <= 2;
    c.ok = n <= 2 ? c.verdict == "checkmate" && c.exhaustive
                  : c.verdict == "checkmate" || c.verdict == "unknown";
    out.cases.push_back(c);
    std::fprintf(stderr, "chronos-bench: mate n=%d naive done\n", n);
  }
  {
    const t5d::GameState s = mate_position(2);
    const HashScorer scorer;
    std::vector<double> us;
    TurngenRun r;
    const Clock::time_point start = Clock::now();
    do {
      r = run_generator(s, scorer, opt);
      us.push_back(r.done_us);
    } while (us.size() < 5 || seconds_between(start, Clock::now()) < 0.2);
    out.turngen.turns = r.turns;
    out.turngen.exhausted = r.exhausted;
    out.turngen.stopped = r.stopped;
    out.turngen.nodes = r.nodes;
    out.turngen.median_us = median(us);
    out.turngen.calls = static_cast<int>(us.size());
    out.turngen.ok = !r.refused && r.turns == 0 && r.exhausted;
  }
  for (const MateCase& c : out.cases) out.ok = out.ok && c.ok;
  out.ok = out.ok && out.turngen.ok;
  return out;
}

void print_mate(const MateResults& m) {
  std::printf("\n== mate: the near-mate family (timelines -n..+n; n = 2 is the 5-timeline case) ==\n");
  std::printf("%-3s %-12s %6s %14s %12s %-10s %-10s %-8s %s\n", "n", "detector", "calls", "median",
              "nodes", "verdict", "exhaustive", "memory", "check");
  for (const MateCase& c : m.cases) {
    const std::string time = c.median_us >= 1e5 ? fixed_text("%.3f s", c.median_us * 1e-6)
                                                : fixed_text("%.1f us", c.median_us);
    std::printf("%-3d %-12s %6d %14s %12s %-10s %-10s %-8s %s\n", c.n, c.detector.c_str(), c.calls,
                time.c_str(), grouped(static_cast<double>(c.nodes)).c_str(), c.verdict.c_str(),
                c.exhaustive ? "yes" : "no", c.stopped_by_memory ? "ceiling" : "-",
                c.ok ? "OK" : "MISMATCH");
  }
  std::printf("TurnGenerator on n = 2: %llu turns, exhausted %s, stopped %s, %s nodes, %.1f us median "
              "(%d calls): %s\n",
              static_cast<unsigned long long>(m.turngen.turns), m.turngen.exhausted ? "yes" : "no",
              m.turngen.stopped ? "yes" : "no", grouped(static_cast<double>(m.turngen.nodes)).c_str(),
              m.turngen.median_us, m.turngen.calls, m.turngen.ok ? "OK" : "MISMATCH");
}

// ---- output -------------------------------------------------------------------------------

void json_position_set(Json& j, const PositionSet& set) {
  j.key("name").str(set.name);
  j.key("kind").str(set.kind);
  if (set.kind == "synthetic") {
    j.key("boards").sint(set.boards);
    j.key("active").sint(set.active);
    j.key("games").sint(set.games);
    j.key("games_skipped").sint(set.games_skipped);
    j.key("seeds").open_array();
    for (const std::uint64_t s : set.seeds) j.unum(s);
    j.close_array();
  } else {
    j.key("boards_min").sint(set.boards);
    if (set.boards_hi > 0) {
      j.key("boards_max").sint(set.boards_hi);
    } else {
      j.key("boards_max").str("open");
    }
    j.key("records").sint(set.games);
  }
  j.key("base_positions").unum(set.bases.size());
}

void write_json(const std::string& path, const Options& opt, const Pinning& pin,
                const std::vector<PositionSet*>& movegen_sets, const std::vector<TurngenSet>& turngen_sets,
                const std::optional<MateResults>& mate, const std::optional<RecordsSummary>& records,
                double total_s) {
  Json j;
  j.open_object();
  j.key("tool").str("chronos-bench");
  j.key("version").str(T5D_VERSION_STRING);
  j.key("compiler").str(CHRONOS_COMPILER);
  j.key("build_type").str(CHRONOS_BUILD_TYPE);
  j.key("cxx_flags").str(CHRONOS_CXX_FLAGS);
  j.key("cpu").str(cpu_brand());
  j.key("hardware_concurrency").unum(std::thread::hardware_concurrency());
  j.key("thread_cpu_time").str(thread_cpu_method());
  j.key("pin").open_object();
  j.key("requested").boolean(pin.requested);
  j.key("pinned").boolean(pin.pinned);
  j.key("cpu").sint(pin.cpu);
  j.key("priority_raised").boolean(pin.priority_raised);
  j.close_object();
  j.key("options").open_object();
  j.key("sections").open_array();
  if (opt.movegen) j.str("movegen");
  if (opt.turngen) j.str("turngen");
  if (opt.mate) j.str("mate");
  j.close_array();
  j.key("rounds").sint(opt.rounds);
  j.key("sample_ms").sint(opt.sample_ms);
  j.key("games").sint(opt.games);
  j.key("children").sint(opt.children);
  j.key("turn_cap").sint(opt.turn_cap);
  j.key("passes").sint(opt.passes);
  j.key("records").str(opt.records);
  j.key("quick").boolean(opt.quick);
  j.key("seed").unum(opt.seed);
  j.key("branch_percent").sint(opt.branch_percent);
  j.key("turn_nodes").unum(opt.turn_nodes);
  j.key("turns_per_node").sint(opt.turns_per_node);
  j.close_object();
  if (records) {
    j.key("records").open_object();
    j.key("files").unum(records->files);
    j.key("unreadable").unum(records->unreadable);
    j.key("refused").unum(records->refused);
    j.key("replayed").unum(records->replayed);
    j.key("action_starts").unum(records->action_starts);
    j.key("distinct_positions").unum(records->distinct);
    j.key("refusals").open_array();
    for (const std::string& r : records->refusals) j.str(r);
    j.close_array();
    j.close_object();
  }
  if (opt.movegen) {
    j.key("movegen").open_object();
    j.key("what").str("generate_moves (pseudo-legal), one thread, interleaved samples; synthetic: "
                      "one-move children of the class positions; records: the positions themselves");
    j.key("sets").open_array();
    for (const PositionSet* set : movegen_sets) {
      j.open_object();
      json_position_set(j, *set);
      j.key("stream_positions").unum(set->stream.size());
      j.key("bases_without_child").unum(set->bases_without_child);
      j.key("mean_moves").num(set->mean_moves);
      j.key("mean_boards").num(set->mean_boards);
      j.key("mean_active").num(set->mean_active);
      j.key("rate_median").num(median(set->rates));
      j.key("rate_min").num(min_of(set->rates));
      j.key("rate_max").num(max_of(set->rates));
      j.key("cpu_ns_per_position_median").num(median(set->cpu_ns));
      j.key("cpu_ns_per_position_pooled").num(set->cpu_ns_pooled());
      j.key("rates").nums(set->rates);
      j.key("cpu_ns_per_position").nums(set->cpu_ns);
      j.key("sample_positions").open_array();
      for (const std::uint64_t n : set->sample_positions) j.unum(n);
      j.close_array();
      j.close_object();
    }
    j.close_array();
    j.close_object();
  }
  if (opt.turngen) {
    j.key("turngen").open_object();
    j.key("unit").str("Successors");
    j.key("scorer").str(kScorerText);
    j.key("sets").open_array();
    for (const TurngenSet& ts : turngen_sets) {
      const TurngenSummary s = summarise(ts, opt);
      j.open_object();
      json_position_set(j, *ts.set);
      j.key("positions").unum(s.positions);
      j.key("refused").unum(s.refused);
      j.key("first_us_median").num(s.first_median);
      j.key("first_us_p90").num(s.first_p90);
      j.key("turns27_us_median").num(s.k27_median);
      j.key("us_per_turn_median").num(s.us_per_turn_median);
      j.key("turns_median").num(s.turns_median);
      j.key("hit_turn_cap").unum(s.hit_cap);
      j.key("budget_stopped").unum(s.stopped);
      j.key("exhausted").unum(s.exhausted);
      j.key("no_legal_turn").unum(s.no_turn);
      j.key("inconsistent_across_passes").unum(s.inconsistent);
      j.key("turns_total").unum(s.turns_total);
      j.key("nodes_total").unum(s.nodes_total);
      j.key("seconds_total").num(s.seconds_total);
      j.key("turns_per_second").num(s.turns_per_second);
      j.key("wall_seconds_all_passes").num(ts.wall_s);
      // Per position, medians over the passes: [first_us, turns27_us, done_us, turns,
      // nodes, exhausted, stopped, refused].
      j.key("per_position").open_array();
      for (const TurngenPosition& p : ts.positions) {
        j.open_array();
        j.num(median(p.first_us)).num(median(p.to_k27_us)).num(median(p.done_us));
        j.unum(p.last.turns).unum(p.last.nodes);
        j.boolean(p.last.exhausted).boolean(p.last.stopped).boolean(p.last.refused);
        j.close_array();
      }
      j.close_array();
      j.close_object();
    }
    j.close_array();
    j.close_object();
  }
  if (mate) {
    j.key("mate").open_object();
    j.key("cases").open_array();
    for (const MateCase& c : mate->cases) {
      j.open_object();
      j.key("n").sint(c.n);
      j.key("timelines").sint(2 * c.n + 1);
      j.key("detector").str(c.detector);
      j.key("calls").sint(c.calls);
      j.key("median_us").num(c.median_us);
      j.key("min_us").num(c.min_us);
      j.key("max_us").num(c.max_us);
      j.key("nodes").unum(c.nodes);
      j.key("verdict").str(c.verdict);
      j.key("in_check").boolean(c.in_check);
      j.key("exhaustive").boolean(c.exhaustive);
      j.key("stopped_by_budget").boolean(c.stopped_by_budget);
      j.key("stopped_by_memory").boolean(c.stopped_by_memory);
      j.key("ok").boolean(c.ok);
      j.close_object();
    }
    j.close_array();
    j.key("turngen_n2").open_object();
    j.key("turns").unum(mate->turngen.turns);
    j.key("exhausted").boolean(mate->turngen.exhausted);
    j.key("stopped").boolean(mate->turngen.stopped);
    j.key("nodes").unum(mate->turngen.nodes);
    j.key("median_us").num(mate->turngen.median_us);
    j.key("calls").sint(mate->turngen.calls);
    j.key("ok").boolean(mate->turngen.ok);
    j.close_object();
    j.key("ok").boolean(mate->ok);
    j.close_object();
  }
  j.key("total_seconds").num(total_s);
  j.key("sink").unum(g_sink.load());
  j.close_object();

  std::ofstream out(path, std::ios::binary);
  if (!out) throw std::runtime_error("cannot write " + path);
  out << j.text() << '\n';
  if (!out) throw std::runtime_error("cannot write " + path);
}

int run(int argc, char** argv) {
  const Options opt = parse_options(argc, argv);
  if (!opt.records.empty() && (opt.movegen || opt.turngen) && !std::filesystem::is_directory(opt.records)) {
    throw std::invalid_argument("--records: not a directory: " + opt.records);
  }
  const Clock::time_point start = Clock::now();
  const Pinning pin = pin_this_thread(opt.pin);
  std::string sections;
  for (const auto& [on, name] : {std::pair{opt.movegen, "movegen"}, std::pair{opt.turngen, "turngen"},
                                 std::pair{opt.mate, "mate"}}) {
    if (on) sections += (sections.empty() ? "" : ",") + std::string(name);
  }

  std::printf("chronos-bench %s: Chronos Phase 0 baseline speeds (rules engine copied from Temporal-5D)\n",
              T5D_VERSION_STRING);
  std::printf("compiler:   %s\n", CHRONOS_COMPILER);
  std::printf("build type: %s\n", CHRONOS_BUILD_TYPE);
  std::printf("flags:      %s\n", CHRONOS_CXX_FLAGS);
  std::printf("cpu:        %s\n", cpu_brand().c_str());
  std::printf("logical processors: %u; thread %s%s; thread CPU time: %s\n",
              std::thread::hardware_concurrency(),
              pin.pinned ? ("pinned to processor " + std::to_string(pin.cpu)).c_str()
                         : (pin.requested ? "NOT pinned (pinning failed or unsupported)" : "not pinned (--no-pin)"),
              pin.priority_raised ? ", priority above normal" : "", thread_cpu_method());
  std::printf("options:    sections %s; rounds %d, sample %d ms, games %d, children %d, turn cap %d, "
              "passes %d%s%s%s\n",
              sections.c_str(), opt.rounds, opt.sample_ms, opt.games, opt.children, opt.turn_cap, opt.passes,
              opt.records.empty() ? "" : ", records ", opt.records.c_str(), opt.quick ? " (--quick)" : "");
  std::printf("synthetic:  SYNTHETIC random play (t5d::synthetic), Standard, seed %llu, branch %d%%, "
              "class_seed = seed + 1000003 N + 10007 T + 37 g\n",
              static_cast<unsigned long long>(opt.seed), opt.branch_percent);
  std::fflush(stdout);

  std::vector<PositionSet> classes;
  std::vector<PositionSet> buckets;
  std::optional<RecordsSummary> records;
  if (opt.movegen || opt.turngen) {
    classes = synthetic_classes(opt, opt.movegen);
    if (!opt.records.empty()) {
      RecordsSummary summary;
      const Clock::time_point t0 = Clock::now();
      buckets = records_buckets(opt.records, summary);
      std::printf("records:    %zu files in %s: %zu replayed to the end (parse mode), %zu unreadable, "
                  "%zu refused; %zu action starts, %zu distinct (zobrist, to move) (%.1f s)\n",
                  summary.files, opt.records.c_str(), summary.replayed, summary.unreadable,
                  summary.refused, summary.action_starts, summary.distinct,
                  seconds_between(t0, Clock::now()));
      for (const PositionSet& b : buckets) {
        std::printf("            %-17s %6zu positions from %d records\n", b.name.c_str(), b.bases.size(), b.games);
      }
      records = std::move(summary);
      std::fflush(stdout);
    }
  }

  std::vector<PositionSet*> movegen_sets;
  if (opt.movegen) {
    for (PositionSet& s : classes) movegen_sets.push_back(&s);
    for (PositionSet& s : buckets) movegen_sets.push_back(&s);
    for (PositionSet* s : movegen_sets) describe_stream(*s);
    std::fprintf(stderr, "chronos-bench: movegen: warm-up + %d rounds x %zu sets x %d ms\n", opt.rounds,
                 movegen_sets.size(), opt.sample_ms);
    run_movegen(opt, movegen_sets);
    print_movegen(movegen_sets);
    std::fflush(stdout);
  }

  std::vector<TurngenSet> turngen_sets;
  if (opt.turngen) {
    for (const PositionSet& s : classes) turngen_sets.push_back(TurngenSet{&s, {}, 0});
    for (const PositionSet& s : buckets) turngen_sets.push_back(TurngenSet{&s, {}, 0});
    run_turngen(opt, turngen_sets);
    print_turngen(turngen_sets, opt);
    std::fflush(stdout);
  }

  std::optional<MateResults> mate;
  if (opt.mate) {
    mate = run_mate(opt);
    print_mate(*mate);
  }

  const double total = seconds_between(start, Clock::now());
  std::printf("\ntotal %.1f s; sink %llu\n", total, static_cast<unsigned long long>(g_sink.load()));
  if (!opt.json.empty()) {
    write_json(opt.json, opt, pin, movegen_sets, turngen_sets, mate, records, total);
    std::printf("json: %s\n", opt.json.c_str());
  }
  if (mate && !mate->ok) {
    std::fprintf(stderr, "chronos-bench: a mate verdict differs from the expected one\n");
    return 1;
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(argc, argv);
  } catch (const std::invalid_argument& e) {
    std::fprintf(stderr, "chronos-bench: %s\n", e.what());
    return 2;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "chronos-bench: %s\n", e.what());
    return 1;
  }
}

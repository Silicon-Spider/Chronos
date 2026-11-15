// Chronos provenance: copied from Temporal-5D (https://github.com/Silicon-Spider/temporal-5d)
//   source: engine/tests/test_alloc.hpp
//   commit: c8a9c490bfde650466609531f8083d69513c6036
//   copied: 2026-11-08 (Chronos spec 3.1 and 3.4; decisions NR1, NR5)
//   state:  unchanged: below this header the file is byte for byte the source
// The test runner's allocation hooks (tests/test_alloc.cpp replaces the global
// operator new and delete for the whole runner - t5d-tests is linked
// statically, so every allocation passes through them). Per thread, and off
// unless a test turns them on:
//
//  * FailAfter(n): the n-th allocation from now (0: the next) throws
//    std::bad_alloc - fault injection;
//  * Counting: the bytes allocated and not yet freed while it is on - what a
//    structure actually holds on the heap (the size asked for, not malloc's
//    own overhead).

#ifndef T5D_TEST_ALLOC_HPP
#define T5D_TEST_ALLOC_HPP

namespace t5d_test::alloc {

extern thread_local long long fail_countdown;  // -1: off
extern thread_local bool counting;
extern thread_local long long live_bytes;

class FailAfter {
 public:
  explicit FailAfter(long long n) noexcept { fail_countdown = n; }
  ~FailAfter() { fail_countdown = -1; }
  FailAfter(const FailAfter&) = delete;
  FailAfter& operator=(const FailAfter&) = delete;
};

class Counting {
 public:
  Counting() noexcept {
    live_bytes = 0;
    counting = true;
  }
  ~Counting() { counting = false; }
  Counting(const Counting&) = delete;
  Counting& operator=(const Counting&) = delete;
};

}  // namespace t5d_test::alloc

#endif  // T5D_TEST_ALLOC_HPP

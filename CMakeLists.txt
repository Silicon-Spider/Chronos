# Fractal: a CPU-only engine for 5D Chess With Multiverse Time Travel (docs/SPEC.md).
#
# Phase 0 builds the rules engine copied from Temporal-5D (spec 3.1; PROVENANCE.md)
# exactly as copied, with its own tests and the gate's engine adapter, plus the
# Phase 0 measurement tools in tools/.
#
#   t5d_core       static library: the copied rules engine
#   t5d            shared library: the C API (engine/include/t5d/capi.h)
#   t5d-tests      the copied tests, in their own in-house runner
#   t5d-gate       the differential harness's engine adapter
#   fractal-bench  Phase 0 baseline speeds (docs/MEASUREMENTS.md)
#   fractal-relhist  attack-distance and placement histograms over t0database
#
# Configure into build/engine: the harness runs ./build/engine/t5d-gate.
#   cmake -S . -B build/engine -G Ninja -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/engine
#   python tools/run_tests.py --exe build/engine/t5d-tests
#
# Nothing here may refer to anything under the harness's directory: the engine's
# build never depends on the test harness or an oracle. Nothing here fetches code,
# and nothing depends on Temporal-5D (NS2).

cmake_minimum_required(VERSION 3.20)

project(fractal
  VERSION 0.1.0
  DESCRIPTION "5D chess engine: alpha-beta over whole turns, scored by an NNUE"
  LANGUAGES CXX)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Build type: Release, Debug, RelWithDebInfo or MinSizeRel" FORCE)
endif()

option(FRACTAL_WERROR "Treat compiler warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Every binary at the top of the build tree, as the copied tests and the harness expect.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Release optimises at -O2, as the copied engine was built and measured
# (CMake's own default for GCC and Clang is -O3).
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  string(REPLACE "-O3" "-O2" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

set(T5D_ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/engine)

# ---- options shared by every target ---------------------------------------------

add_library(t5d_options INTERFACE)
target_compile_features(t5d_options INTERFACE cxx_std_20)
if(MSVC)
  target_compile_options(t5d_options INTERFACE /W4 /permissive- /utf-8
    $<$<BOOL:${FRACTAL_WERROR}>:/WX>)
else()
  target_compile_options(t5d_options INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
    $<$<BOOL:${FRACTAL_WERROR}>:-Werror>)
endif()
target_compile_definitions(t5d_options INTERFACE T5D_VERSION_STRING="${PROJECT_VERSION}")

# ---- the copied rules engine (spec 3.1) -------------------------------------------
#
# Every copied module except certificate: engine/include/t5d/certificate.hpp
# includes t5d/prover.hpp, a spec 3.2 module that is not copied, so certificate.cpp
# can't be built here (docs/gates/G0.md).

add_library(t5d_core STATIC
  engine/src/attacks.cpp
  engine/src/board.cpp
  engine/src/check.cpp
  engine/src/fen.cpp
  engine/src/fingerprint.cpp
  engine/src/gate.cpp
  engine/src/hypercuboid.cpp
  engine/src/legal.cpp
  engine/src/md5.cpp
  engine/src/move.cpp
  engine/src/movegen.cpp
  engine/src/multiverse.cpp
  engine/src/pgn.cpp
  engine/src/rules.cpp
  engine/src/search.cpp
  engine/src/state.cpp
  engine/src/timeline.cpp
  engine/src/types.cpp
  engine/src/variants.cpp
  engine/src/zobrist.cpp)
target_include_directories(t5d_core PUBLIC ${T5D_ENGINE_DIR}/include)
target_link_libraries(t5d_core PUBLIC t5d_options)
# The core is linked into the shared library, so it must be position-independent.
set_target_properties(t5d_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---- the C API shared library -------------------------------------------------------

add_library(t5d SHARED engine/src/capi.cpp)
target_link_libraries(t5d PRIVATE t5d_core)
target_compile_definitions(t5d PRIVATE T5D_BUILDING_DLL)
if(WIN32)
  set_target_properties(t5d PROPERTIES PREFIX "")  # t5d.dll, not libt5d.dll
else()
  # Export the C API only, not the C++ core inside the library.
  set_target_properties(t5d_core t5d PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)
endif()

# ---- the copied tools the tests and the gate need -----------------------------------

add_library(t5d_tools STATIC
  engine/tools/synthetic.cpp
  engine/tools/tool_support.cpp
  engine/tools/turn_width.cpp)
target_include_directories(t5d_tools PUBLIC ${T5D_ENGINE_DIR}/tools)
target_link_libraries(t5d_tools PUBLIC t5d_core)

add_executable(t5d-gate engine/tools/t5d_gate.cpp)
target_link_libraries(t5d-gate PRIVATE t5d_core)

# ---- the copied tests ------------------------------------------------------------
#
# All 64 files in engine/tests are copied; these are the test sources that build
# without a spec 3.2 module, an uncopied tool or a removed capi entry point
# (docs/gates/G0.md lists the rest and why). As upstream, one runner holds them all
# and the C API is tested linked in statically, from the same source as the library.
#
# Not built (each needs something outside the copy):
#   test_alloc test_assets test_audit test_capi_game test_capi_search
#   test_certificate test_expand test_expand_reference test_guard test_hunt
#   test_hunt_truth test_ladder test_mcts test_mcts_memory test_mcts_tree
#   test_play_exe test_prover test_scanner test_search test_selfplay
#   test_shared_stores test_sixbpp test_turn_state test_views

add_executable(t5d-tests
  engine/tests/test_main.cpp
  engine/tests/test_apply.cpp
  engine/tests/test_attacks.cpp
  engine/tests/test_board.cpp
  engine/tests/test_capi.cpp
  engine/tests/test_check.cpp
  engine/tests/test_detectors.cpp
  engine/tests/test_fen.cpp
  engine/tests/test_fingerprint.cpp
  engine/tests/test_gate.cpp
  engine/tests/test_gate_exe.cpp
  engine/tests/test_md5.cpp
  engine/tests/test_movegen.cpp
  engine/tests/test_movegen_timelines.cpp
  engine/tests/test_multiverse.cpp
  engine/tests/test_pgn_examples.cpp
  engine/tests/test_pgn_read.cpp
  engine/tests/test_pgn_replay.cpp
  engine/tests/test_random_play.cpp
  engine/tests/test_rules.cpp
  engine/tests/test_special_moves.cpp
  engine/tests/test_state.cpp
  engine/tests/test_synthetic.cpp
  engine/tests/test_timeline.cpp
  engine/tests/test_turngen.cpp
  engine/tests/test_turnwidth.cpp
  engine/tests/test_types.cpp
  engine/tests/test_variants.cpp
  engine/tests/test_zobrist.cpp
  engine/src/capi.cpp)
target_include_directories(t5d-tests PRIVATE ${T5D_ENGINE_DIR}/tests)
target_link_libraries(t5d-tests PRIVATE t5d_core t5d_tools)
# test_gate_exe.cpp runs the adapter itself. test_turnwidth.cpp's three measure_exe
# cases run t5d-measure, which is not copied (it needs a spec 3.2 module): the path
# is defined so the file builds unchanged, and tools/run_tests.py skips those cases.
add_dependencies(t5d-tests t5d-gate)
target_compile_definitions(t5d-tests PRIVATE
  "T5D_GATE_EXE=\"$<TARGET_FILE:t5d-gate>\""
  "T5D_MEASURE_EXE=\"${CMAKE_BINARY_DIR}/t5d-measure${CMAKE_EXECUTABLE_SUFFIX}\""
  "T5D_TEST_SCRATCH=\"${CMAKE_CURRENT_BINARY_DIR}/test-scratch\""
  "T5D_ENGINE_DIR=\"${T5D_ENGINE_DIR}\"")

# ---- Fractal's Phase 0 tools (new code, not copied) ---------------------------------

add_executable(fractal-bench tools/bench/fractal_bench.cpp)
target_link_libraries(fractal-bench PRIVATE t5d_tools)
string(TOUPPER "${CMAKE_BUILD_TYPE}" FRACTAL_BUILD_TYPE_UPPER)
string(STRIP "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${FRACTAL_BUILD_TYPE_UPPER}}" FRACTAL_FLAGS_TEXT)
target_compile_definitions(fractal-bench PRIVATE
  "FRACTAL_BUILD_TYPE=\"${CMAKE_BUILD_TYPE}\""
  "FRACTAL_CXX_FLAGS=\"${FRACTAL_FLAGS_TEXT}\""
  "FRACTAL_COMPILER=\"${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\"")

add_executable(fractal-relhist tools/relhist/fractal_relhist.cpp)
target_link_libraries(fractal-relhist PRIVATE t5d_core)

# ---- MinGW: no dependence on the toolchain's runtime DLLs ---------------------------
#
# With MSYS2's MinGW-w64 g++, libstdc++, libgcc and winpthreads are DLLs of the
# toolchain. Link them in, so the binaries and the DLL run without MSYS2 on the PATH.

if(MINGW)
  target_link_options(t5d PRIVATE -static-libgcc -static-libstdc++ -static)
  foreach(exe t5d-tests t5d-gate fractal-bench fractal-relhist)
    target_link_options(${exe} PRIVATE -static)
  endforeach()
endif()

# ---- tests ---------------------------------------------------------------------

enable_testing()
find_package(Python3 3.10 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
  add_test(NAME t5d-tests
    COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_tests.py
            --exe $<TARGET_FILE:t5d-tests> --tests-dir ${T5D_ENGINE_DIR}/tests)
endif()

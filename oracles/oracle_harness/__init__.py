# Fractal provenance: copied from Temporal-5D (https://github.com/Silicon-Spider/temporal-5d)
#   source: oracles/oracle_harness/__init__.py
#   commit: c8a9c490bfde650466609531f8083d69513c6036
#   copied: 2026-11-08 (Fractal spec 3.1 and 3.4; decisions NR1, NR5)
#   state:  unchanged: below this header the file is byte for byte the source
"""Differential-testing harness over the pinned rules oracles.

This package is test infrastructure. Nothing outside ``oracles/`` may import it,
link against it, or reference the third-party code it drives; ``boundary.py``
enforces that on every commit. The engine reaches the gate through a process
boundary (see ``adapters/engine.py``), never through an import.
"""

__version__ = "1"

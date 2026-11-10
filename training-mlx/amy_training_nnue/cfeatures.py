"""ctypes bindings for amy's C feature-extraction library.

The C library ``libamy_nn_extract`` (built by ``make`` in the amy
engine checkout) is the reference implementation of feature
generation: piece planes (own/opponent plus the opponent-king plane
10), shared pawn-pair rows, and attack-map rows, all in the side's
canonical frame.  The training pipeline calls it instead of
re-implementing the index math, so the training data and the engine
can never drift apart (PLAN-attack-map-features.md, T2).

The library is found via the ``AMY_NN_LIB`` environment variable, or
in the ``src/`` directory at the amy repository root (this package's
grandparent directory; built by ``make -C src`` — dylib on macOS, .so
elsewhere).
"""

from __future__ import annotations

import ctypes
import os
from dataclasses import dataclass
from pathlib import Path

import numpy as np

# One BPD record / BinaryPosition in bytes.  Must match
# bpd.RECORD_STRUCT.size and amy's struct BinaryPosition.
RECORD_SIZE = 40

# The feature-generation caps of the C library's ABI (nn_extract.h /
# neural_net.h): the row widths of the padded output arrays.  Python
# literals, kept in sync by test_cfeatures.test_constants_sync,
# which compares them against the live nn_export_constants() values.
MAX_PIECE_FEATURES = 36
MAX_PAWN_FEATURES = 128
MAX_ATTACK_FEATURES = 32


class NnExtractError(RuntimeError):
    """The C library is missing or failed."""


@dataclass(frozen=True)
class NnExtractConstants:
    """Architecture constants exported by the C library."""

    n_features: int
    n_pawn_features: int
    n_attack_features: int
    accumulator_size: int
    max_piece_features: int
    max_pawn_features: int
    max_attack_features: int
    phase_count: int


@dataclass
class Extracted:
    """Feature rows for a batch of BPD records.

    Every feature array is (n_records, max_*) int32, front-packed
    with the active indices and padded with -1.  Records the C
    validation (bpd_record_valid) rejected carry all-(-1) rows and
    ``valid[i] == 0``; their turn/score values are undefined.

    ``phase`` is (n_records, nn_phase_count) int32, the per-record
    active phase-head 0/1 masks of nn_phase_masks() (the engine's
    phase of the position plus, at the top of a bucket, the adjacent
    higher phase); invalid records carry an all-zero row.
    """

    feat_w: np.ndarray
    feat_b: np.ndarray
    pfeat_w: np.ndarray
    pfeat_b: np.ndarray
    afeat_w: np.ndarray
    afeat_b: np.ndarray
    turn: np.ndarray
    score: np.ndarray
    valid: np.ndarray
    phase: np.ndarray


class _CConstants(ctypes.Structure):
    _fields_ = [
        ("n_features", ctypes.c_int32),
        ("n_pawn_features", ctypes.c_int32),
        ("n_attack_features", ctypes.c_int32),
        ("accumulator_size", ctypes.c_int32),
        ("max_piece_features", ctypes.c_int32),
        ("max_pawn_features", ctypes.c_int32),
        ("max_attack_features", ctypes.c_int32),
        ("phase_count", ctypes.c_int32),
    ]


_lib: ctypes.CDLL | None = None
_constants: NnExtractConstants | None = None


def _find_lib_path() -> str:
    env = os.environ.get("AMY_NN_LIB")
    if env:
        return env
    amy_dir = Path(__file__).resolve().parents[2] / "src"
    for name in ("libamy_nn_extract.dylib", "libamy_nn_extract.so"):
        candidate = amy_dir / name
        if candidate.exists():
            return str(candidate)
    raise NnExtractError(
        "libamy_nn_extract not found: run `make -C src` in the engine, "
        "or point AMY_NN_LIB at the library."
    )


def _load() -> ctypes.CDLL:
    global _lib, _constants
    if _lib is None:
        lib = ctypes.CDLL(_find_lib_path())

        c = _CConstants()
        lib.nn_export_constants.argtypes = [ctypes.POINTER(_CConstants)]
        lib.nn_export_constants.restype = None
        lib.nn_export_constants(ctypes.byref(c))
        _constants = NnExtractConstants(
            *(getattr(c, name) for name, _ in _CConstants._fields_)
        )

        ptr = ctypes.c_void_p
        lib.nn_extract_features.argtypes = [
            ctypes.c_char_p,  # data
            ctypes.c_size_t,  # n_records
            ptr,  # feat_w
            ptr,  # feat_b
            ptr,  # pfeat_w
            ptr,  # pfeat_b
            ptr,  # afeat_w
            ptr,  # afeat_b
            ptr,  # turn (uint8 *)
            ptr,  # score (int32 *)
            ptr,  # valid (int32 *)
            ptr,  # phase (int32 *), n_records x phase_count 0/1 masks
        ]
        lib.nn_extract_features.restype = ctypes.c_int
        _lib = lib
    return _lib


def constants() -> NnExtractConstants:
    """The architecture constants of the C library."""
    _load()
    assert _constants is not None
    return _constants


def extract(data: bytes | np.ndarray, n_records: int) -> Extracted:
    """Extract feature rows for ``n_records`` 40-byte BPD records.

    ``data`` must be a contiguous buffer of at least
    ``n_records * RECORD_SIZE`` bytes (the raw file bytes).  The
    output arrays cover all ``n_records`` records; use ``.valid`` to
    keep the ones the C validation accepted.
    """
    lib = _load()
    k = constants()
    if len(data) < n_records * RECORD_SIZE:
        raise ValueError(
            f"data holds {len(data)} bytes, need {n_records * RECORD_SIZE}"
        )

    feat_w = np.empty((n_records, k.max_piece_features), dtype=np.int32)
    feat_b = np.empty((n_records, k.max_piece_features), dtype=np.int32)
    pfeat_w = np.empty((n_records, k.max_pawn_features), dtype=np.int32)
    pfeat_b = np.empty((n_records, k.max_pawn_features), dtype=np.int32)
    afeat_w = np.empty((n_records, k.max_attack_features), dtype=np.int32)
    afeat_b = np.empty((n_records, k.max_attack_features), dtype=np.int32)
    turn = np.empty(n_records, dtype=np.uint8)
    score = np.empty(n_records, dtype=np.int32)
    valid = np.empty(n_records, dtype=np.int32)
    phase = np.empty((n_records, k.phase_count), dtype=np.int32)

    if isinstance(data, np.ndarray):
        buf = data.ctypes.data_as(ctypes.c_char_p)
    else:
        buf = data
    n_valid = lib.nn_extract_features(
        buf,
        n_records,
        feat_w.ctypes.data_as(ctypes.c_void_p),
        feat_b.ctypes.data_as(ctypes.c_void_p),
        pfeat_w.ctypes.data_as(ctypes.c_void_p),
        pfeat_b.ctypes.data_as(ctypes.c_void_p),
        afeat_w.ctypes.data_as(ctypes.c_void_p),
        afeat_b.ctypes.data_as(ctypes.c_void_p),
        turn.ctypes.data_as(ctypes.c_void_p),
        score.ctypes.data_as(ctypes.c_void_p),
        valid.ctypes.data_as(ctypes.c_void_p),
        phase.ctypes.data_as(ctypes.c_void_p),
    )
    if n_valid < 0:
        raise NnExtractError(f"nn_extract_features failed: {n_valid}")

    return Extracted(
        feat_w=feat_w,
        feat_b=feat_b,
        pfeat_w=pfeat_w,
        pfeat_b=pfeat_b,
        afeat_w=afeat_w,
        afeat_b=afeat_b,
        turn=turn,
        score=score,
        valid=valid,
        phase=phase,
    )

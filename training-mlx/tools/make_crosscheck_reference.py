"""Regenerate the cross-check reference (tests/data/crosscheck_reference.json).

Re-runs amy's C implementation (src/nn_crosscheck.c in the amy NNUE
checkout) with a random in-range network generated with a fixed seed,
and saves its output.  Usage:

    python tools/make_crosscheck_reference.py [AMY_DIR] [CROSSCHECK_BIN]

Defaults: AMY_DIR=<amy repo root>
"""

import json
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from amy_training_nnue.features import (  # noqa: E402
    ACCUMULATOR_SIZE,
    HIDDEN_SIZE_1,
    HIDDEN_SIZE_2,
    N_ATTACK_FEATURES,
    N_FEATURES,
    N_PAWN_FEATURES,
    NN_PHASE_COUNT,
)
from amy_training_nnue.quantize import (  # noqa: E402
    quantize_parameters,
    write_network_quant,
)

AMY = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT.parent
BIN = Path(sys.argv[2]) if len(sys.argv) > 2 else AMY / "src" / "nn_crosscheck"
OUT = ROOT / "tests" / "data" / "crosscheck_reference.json"

# Must mirror the EPD table of nn_crosscheck.c exactly (same order,
# same strings): the reference stores EPS[n] against the position with
# index n in the C dump.  Phases covered: 3, 3, 3, 2, 3, 3, 0,
# 0/1 (boundary), 1.
EPS = [
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq -",
    "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq -",
    "3r1k2/p3qppP/8/3p4/3r4/5QP1/P4PB1/4R1K1 w - -",
    "rnbqkbnr/1pp1pppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6",
    # Both kings on the left half (non-mirrored frame): the horizontal
    # mirror of the previous EPD.
    "2k1r3/Pppq3p/8/4p3/4r3/1PQ5/1BP4P/1K1R4 w - -",
    "r7/8/8/8/8/8/4K2k/8 w - -",
    # 2 non-pawn pieces, top of phase 0 (nn_phase_masks activates
    # phases 0 and 1).
    "3r1r2/8/8/8/8/8/4K2k/8 w - -",
    "2R1r1r1/8/8/8/8/8/4K2k/8 w - -",
]

SEED = 1234

# input_w, pawn_w and attack_w are drawn from [-1, 1]: the range
# representable by int8 at scale 64 (and the range trained weights are
# kept in).  Draw order matters: it must mirror _make_network() in
# tests/test_crosscheck.py.  The phase branches are fully independent
# draws, one (h1_w, h1_b, h2_w, h2_b, out_w, out_b) tuple per phase.
rng = np.random.default_rng(SEED)
params = {
    "input_w": rng.uniform(-1, 1, (N_FEATURES, ACCUMULATOR_SIZE)),
    "input_b": rng.uniform(-1, 1, (ACCUMULATOR_SIZE,)),
    "pawn_w": rng.uniform(-1, 1, (N_PAWN_FEATURES, ACCUMULATOR_SIZE)),
    "attack_w": rng.uniform(-1, 1, (N_ATTACK_FEATURES, ACCUMULATOR_SIZE)),
    "h1_w": rng.uniform(-2, 2, (NN_PHASE_COUNT, 2 * ACCUMULATOR_SIZE, HIDDEN_SIZE_1)),
    "h1_b": rng.uniform(-2, 2, (NN_PHASE_COUNT, HIDDEN_SIZE_1)),
    "h2_w": rng.uniform(-2, 2, (NN_PHASE_COUNT, HIDDEN_SIZE_1, HIDDEN_SIZE_2)),
    "h2_b": rng.uniform(-2, 2, (NN_PHASE_COUNT, HIDDEN_SIZE_2)),
    "out_w": rng.uniform(-1, 1, (NN_PHASE_COUNT, HIDDEN_SIZE_2)),
    "out_b": rng.uniform(-1, 1, (NN_PHASE_COUNT,)),
}
quantized = quantize_parameters(params)

tmp = Path(tempfile.mkdtemp(prefix="nnue_ref_"))
write_network_quant(tmp / "amy.net", quantized)

proc = subprocess.run([str(BIN)], cwd=tmp, capture_output=True, text=True)
if proc.returncode != 0:
    print(proc.stderr)
    sys.exit(1)

lines = [ln for ln in proc.stdout.splitlines() if ln != ""]
ref = {"seed": SEED, "positions": []}
i = 0
while i < len(lines):
    if lines[i].startswith("POS "):
        n = int(lines[i].split()[1])
        turn = int(lines[i].split()[-1])
        # PHASE = the engine's nn_phase_index() (single hard switch),
        # PMASKS = the 0/1 active-head masks of nn_phase_masks().
        assert lines[i + 1].startswith("PHASE "), lines[i + 1]
        phase = int(lines[i + 1].split()[1])
        assert lines[i + 2].startswith("PMASKS "), lines[i + 2]
        pmasks = [int(x) for x in lines[i + 2].split()[1:]]
        assert all(x in (0, 1) for x in pmasks)
        assert phase == min(j for j, x in enumerate(pmasks) if x), (
            phase,
            pmasks,
        )
        acc_w = np.array(lines[i + 3].split(), dtype=np.int32)
        acc_b = np.array(lines[i + 4].split(), dtype=np.int32)
        i += 5
        assert lines[i].startswith("SCORE ")
        score = int(lines[i].split()[-1])
        i += 1
        while i < len(lines) and lines[i].startswith("I "):
            i += 1
        assert lines[i].startswith("SCOREI ")
        assert int(lines[i].split()[-1]) == score
        i += 1
        # The six feature index sets dumped by nn_crosscheck.c (the
        # engine's own generation).  FEAT = piece, PFEAT = pawn pair,
        # AFEAT = attack.
        idx = {}
        for _ in range(6):
            tok = lines[i].split()
            key = {"FEAT": "feat", "PFEAT": "pfeat", "AFEAT": "afeat"}[tok[0]]
            key += "_w" if tok[1] == "W" else "_b"
            assert int(tok[2]) == len(tok) - 3, tok
            idx[key] = [int(x) for x in tok[3:]]
            i += 1
        assert sorted(idx) == [
            "afeat_b",
            "afeat_w",
            "feat_b",
            "feat_w",
            "pfeat_b",
            "pfeat_w",
        ]
        ref["positions"].append(
            {
                "epd": EPS[n],
                "turn": turn,
                "phase": phase,
                "pmasks": pmasks,
                "score": score,
                "acc_w": acc_w.tolist(),
                "acc_b": acc_b.tolist(),
                **idx,
            }
        )
    else:
        i += 1

assert len(ref["positions"]) == len(EPS)
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(ref))
print(f"wrote {OUT}")

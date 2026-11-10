# amy-training-nnue

Training application for amy's NNUE evaluation function.

Trains the weights of the neural network used by amy's
`feature/neural-network-with-pawns` branch from amy selfplay data in
[BPD format](https://github.com/thgreiner/amy), and exports the raw
`amy.net` weight file that amy loads at startup.

## Architecture

The network mirrors amy's C implementation (`src/neural_net.c`) exactly:

| Stage | amy C | Python |
|---|---|---|
| Features | 22528 king-relative features in a canonical frame per side: 10 piece types (5 own + 5 opponent) + the opponent king (plane 10) x 32 canonical king positions x 64 squares, index `2048*plane + 64*king_index + sq'` (the own king is not a feature; it only defines the frame). Each frame is rank-flipped for black (`sq ^ 0x38`) and, if the own king stands on files e..h, mirrored horizontally (`sq ^ 0x07`), so the king is always in the left half; `king_index` = file + 4*rank of the canonical king (a1 = 0 ... d8 = 31). Plus 4560 pawn *pair* features: one per unordered pair of the 96 color-square ids (`hi*(hi-1)/2 + lo`), only 1488 ever active (the two pawns within one file of distance), indexed in each side's own frame; both sides read the same shared matrix | `amy_training_nnue/features.py` |
| Accumulator | per-side int16 vector of 256 (units of 1/64), clamped to `[0, 64]` at evaluation | `model.py` (float), `quantize.py` (int16) |
| Network | 2x256 -> 32 -> 32 -> 1; h1 clamped ReLU `(h + 4095) / 8192` in `[0, 64]`, h2 `(h + 31) / 64` in `[0, 8192]` | `model.py` (float, `SCALE_INPUT_HALF` clamps), `quantize.py` (integer) |
| Output | `((out + 4095) / 8192) * 10000` then `(r + 4095) / 8192` -> millipawns, C truncating division | same, `quantize.py` |
| Constants | `SCALE = 8192`, `SCALE_INPUT = 64`, `SCALE_INPUT_HALF = 31`, `SCALE_H1 = 524288`, `SCALE_HALF = 4095`, `SCALE_OUTPUT = 10000` | identical |

Training happens in floating point (MLX, `NnueNet`), then the parameters are
quantized into the exact integer pipeline amy runs: the accumulator is kept
in 1/64 units (input weights `int8` x 64, the shared 4560-row pawn-pair
block `int8` x 64, input bias `int16` x 64), hidden 1
uses `int16` x 8192 weights with an `int32` x 64 x 8192 bias (activations in
[0, 64]), hidden 2 uses `int16` x 8192 weights with an `int32` x 8192 x 64
bias (activations in [0, 8192]), and the output layer `int32` x 8192 /
`int32` x 8192^2. `QuantizedNet` in `quantize.py` simulates that integer
pipeline bit-exactly and is used to report quantized metrics. The file is
6,970,268 bytes (24-byte header + fields, including the 1,167,360-byte
shared pawn block after the piece input weights).

## Setup

```bash
conda env create -f environment.yml   # or: conda env update
conda activate amy-training-nnue
pip install -e .
```

## Usage

All commands run from this directory (the package is importable from here
or after `pip install -e .`):

```bash
# Dataset statistics
python -m amy_training_nnue stats evaluation_log.bin

# Inspect records (EPD, score, validity)
python -m amy_training_nnue inspect evaluation_log.bin --count 10

# Train (float) + export quantized amy.net
python -m amy_training_nnue train evaluation_log.bin \
    --output checkpoints/default --epochs 30 --batch-size 256 --seed 0

# Re-export a network from a saved float checkpoint
python -m amy_training_nnue export checkpoints/default/final --out amy.net

# Evaluate a quantized network on a BPD file
python -m amy_training_nnue evaluate checkpoints/default/amy.net \
    evaluation_log.bin --show 5
```

Copy the produced `amy.net` into the engine's `nets/` directory at the
repository root (as `nets/amy.net`, or the engine will compress it to
`nets/amy.net.z` on first load) and it is picked up at startup.

Training options of note: `--lr`, `--weight-decay`, `--val-fraction`,
`--save-every N` (checkpoint cadence; every checkpoint is ~11.5 MB),
`--resume <dir>` (continue from a float checkpoint). No dataset
augmentation: with the canonical king-half frame, a position and its
horizontal mirror image already produce identical feature vectors,
so mirroring would only duplicate rows.

Loss: MSE in tanh-squeezed space — the model output and the target
(`score / 10000`) are both passed through `tanh` before the squared
error, squeezing large score swings into (-1, 1).  For playing
strength it matters more to capture the subtleties of about-even
positions (eval near 0) than to predict large positive/negative scores
accurately, and the tanh gradient focuses the network on that regime.
Float-val metrics are measured in the same squeezed space.

## Data

Input is amy's selfplay evaluation log: 40-byte BPD records
(position + turn + score in millipawns, filtered by amy's
`bpd_log_eval_diff_threshold`). Labels are `score / 10000` in model
units (tanh-squeezed in the loss).

## Verification

The Python pipeline is verified **bit-exact** against amy's C code:

- `tests/test_crosscheck.py` + `tests/data/crosscheck_reference.json`:
  a random in-range network (fixed seed) is evaluated with amy's own
  `neural_net.c` (via the engine's `src/nn_crosscheck.c` at the repository
  root) and with
  `QuantizedNet`; raw int16 accumulators and final scores must be identical.
- A production-trained `amy.net` loads in amy (`nn_read_weights_quantized`),
  amy's full test suite (`Amy --test`, including `test_all_neural_net`)
  passes with it, and amy's scores match `QuantizedNet` for the same
  positions.

Regenerate the cross-check reference (needs the engine at the repo root):

```bash
# in the repository root:
make -C src   # builds src/Amy and src/nn_crosscheck
# then, from this directory:
python tools/make_crosscheck_reference.py [AMY_DIR]
```

Run the full suite:

```bash
python -m pytest tests/ -q   # 47 tests
```

## Current results

On the 3.28M-record selfplay log (`evaluation_log.bin` in this directory),
30 epochs,
AdamW lr=1e-3, batch 2048, seed 0, on an M1 Pro. Metrics on the full log
(`python -m amy_training_nnue evaluate`, 10-plane net). *These numbers
are from the previous (45056-feature, unmirrored) architecture; the net
must be retrained on the current 22528-feature king-half features (with
the opponent-king plane 10) and the new hidden-1 quantization.*

```
quantized: rmse 213 mp, mae 152 mp, corr 0.9911, r2 0.9823, within 25cp 1.000
float    : rmse 204 mp, mae 144 mp, corr 0.9919, r2 0.9838, within 25cp 1.000
```

`tests/data/selfplay_sample.bin` is a frozen 5% snapshot of the selfplay
log (seed 42, ~60k records) used by the training smoke test; the live log
keeps growing and is never read by the tests. Refresh the snapshot with
`python tools/make_sample.py` if it becomes stale, and commit the result.

The residual is close to the noise floor of the labels (engine scores from
selfplay). More/better data (e.g. deeper search scores, more games) is the
main path to a stronger net.

# amy-training-nnue — agent guide

Training app for amy's NNUE (branch `feature/phase-branches`; the engine
lives in the root of this repository). Reads amy selfplay BPD logs, trains a
float network with MLX, quantizes it into amy's exact integer pipeline, and
emits the `amy.net` file amy loads at startup (into the engine's
`nets/` directory at the repo root).

The network has **4 independent per-phase hidden/output branches**
(`NN_PHASE_COUNT`), one per game phase, over a *shared* input side
(piece/pawn/attack weights, accumulators).  The engine hard-switches
to the phase of the position at evaluation; training co-trains the
adjacent phase at bucket boundaries (see "Phase branches" below).

## Feature generation: C is the source of truth

Feature indices (piece planes, pawn pairs, attack maps) are generated
by the C engine, not by Python:

- `nn_compute_piece_indices` / `nn_compute_pawn_indices`
  (`neural_net.c`) and `am_compute_attack_indices` (`attack_map.c`)
  are the single definitions, shared by the engine (accumulator
  rebuild) and the feature-extraction library `nn_extract.c`.
  `dataset.py` builds its tensors through that library
  (`cfeatures.py`; `libamy_nn_extract.dylib`/`.so` built by `make` in
  the amy checkout, override with `AMY_NN_LIB`).
- `features.py` (king-relative indices, pawn pairs) is a **parity
  oracle only** — it is not on the runtime path and will be retired
  once the cross-check no longer needs it
  (PLAN-attack-map-features.md T3/T4).  Its index formulas describe
  the C implementation; keep the two in lockstep until retirement.
- Architecture constants: the C library's `nn_export_constants` is
  the arbiter; the Python literals (`features.py`, the `cfeatures.py`
  row-width caps) are checked against it by
  `tests/test_cfeatures.py::test_constants_sync`.
- BPD validation: C's `bpd_record_valid` (`nn_extract.c`) is the
  arbiter for what may enter the training data;
  `BpdRecord.validate()` mirrors it, cross-tested in
  `tests/test_cfeatures.py::test_invalid_records_cross`.

## Hard invariant: bit-exact with amy's C

`QuantizedNet` (`amy_training_nnue/quantize.py`) must reproduce amy's C
`neural_net.c` **bit-for-bit**, not approximately:

- int16 accumulators with **wrap-around** (numpy `.astype(np.int16)` matches
  arm64 C; the raw accumulator is stored unclamped, clamping to
  `[0, SCALE_INPUT]` happens only on the 2*ACCUMULATOR_SIZE (512)-dim
  local copy at evaluation time).
- The pipeline works in **1/64 units** from the accumulator through
  hidden layer 1: input bias int16 @ 64, input weights int8 @ 64 (the hot
  loop is a plain `int16 += int8`, no per-feature rescale), plus — for
  each active pawn pair — the shared pawn weight row int8 @ 64 (same
  `int16 += int8` add, one row for each side's accumulator), plus — for
  each active attack feature — the shared attack weight row int8 @ 64
  (same add; see the attack features bullet below); h1 weights
  int16 @ 8192 with an int32 @ 64*8192 bias (sum in 64*8192 units), h2
  weights int16 @ 8192 with an int32 @ 8192*64 bias (sum in 8192*64
  units).  The sums are converted to the 64 / 8192 units of the
  activations (h1 in `[0, 64]`, h2 in `[0, 8192]`) by
  `trunc((q + 4095) / 8192)` for h1 and `trunc((q + 31) / 64)` for h2.
- Clamped ReLU: h1: `h = 0` if `q <= 0`, else `64` if `q > 64*8192`
  (strict `>`), else `trunc((q + 4095) / 8192)`.  h2: `h = 0` if `q <= 0`,
  else `8192` if `q >= 8192*64` (`>=`), else `trunc((q + 31) / 64)` —
  match the C per layer.
- Integer division **truncates toward zero** (C), not floor (Python).
  Use `_trunc_div`, never `//` on possibly-negative int values.
- Output transform is two steps:
  `r = trunc((out_q + 4095) / 8192); score = trunc((r * 10000 + 4095) / 8192)`.
  Both `+ 4095` terms are required.
- Black-side feature squares are rank-flipped: `sq ^ 0x38`, and if the
  side's own king (in its own frame) stands on files e..h, the *whole*
  frame is additionally mirrored horizontally (`sq ^ 0x07`), so the king
  is always canonicalized to the left half (files a..d).  The two masks
  commute.  A position and its horizontal mirror image therefore produce
  identical feature vectors.
- Feature index: `2048 * plane + 64 * king_index + sq'`, planes 0..4 =
  own P N B R Q, 5..9 = opponent P N B R Q, 10 = the opponent king in
  the side's own canonical frame (the own king is not a feature; the
  opponent king is plane 10; see `nn_build_accumulator()` in
  neural_net.c).  `king_index` = file + 4 * rank of the canonical
  own king (0..31: a1 = 0 ... d1 = 3, ... a8 = 28 ... d8 = 31);
  `sq'` is the piece square (for plane 10: the opponent king's
  square) in the same frame (pieces are not
  restricted to the left half, only the king).  N_FEATURES =
  11 * 32 * 64 = 22528.
- Pawn pair features (see `nn_init_pawns()` in neural_net.c): every
  unordered pair of pawns within one file of distance (same file, any
  rank, or adjacent files) contributes one row of a *shared*
  N_PAWN_FEATURES x ACCUMULATOR_SIZE int8 matrix (4560 x 256, one row
  per unordered pair of the 96 color-square ids; both sides read the
  same matrix, but each pair is indexed in that side's own canonical
  frame, where white pawns have color 0 and black pawns 1 — swapped for
  black).  Id per pawn: `color * 48 + (sq' - 8)` (ranks 2..7 only —
  positions with pawns on rank 1 or 8 must not be fed to the pipeline,
  and bpd.py rejects them); pair index: combadic `hi * (hi - 1) / 2 +
  lo` (hi = max, lo = min), in 0..4559.  Only 1488 of the 4560 rows are
  ever active (the rest may hold zeros); both sides get the same pair
  *set*, mapped through their own frame.  Worked example (starting
  position): (b2, a2) is index 27 in the white frame (ids 6/7) and
  index 4559 in the black frame (ids 94/95, colors swapped).
- Attack features (see `nn_update_attack_maps()` in neural_net.c and
  `am_compute_attack_maps()` in attack_map.c): own pieces attacking
  opponent pieces/pawns (kings never attacked; no pawn→pawn; redundant
  pairs excluded — queen only on the rays a bishop/rook doesn't cover,
  bishop/queen pawn attacks only where no pawn attacks) form one row of
  a *shared* N_ATTACK_FEATURES x ACCUMULATOR_SIZE int8 matrix
  (98,304 x 256), indexed `64 * 64 * plane + from' * 64 + to'` in the
  side's canonical frame (both `from` and `to` — folds under
  mirroring), 24 planes = attacker P..Q × victim P..Q minus pawn→pawn
  (`plane = (attacker-1)*5 + (victim-2)`, attacker/victim = P..Q =
  1..5).  Up to `INTSET_SORTED_MAX` (32) active rows per side; more is
  a fatal error in the engine.  Both sides read the same matrix; each
  side's rows feed that side's accumulator.

Any change to the pipeline must be re-validated:

1. `python -m pytest tests/ -q` — 76 tests, including
   `tests/test_crosscheck.py`, which compares against C ground truth stored
   in `tests/data/crosscheck_reference.json` (regenerate with
   `python tools/make_crosscheck_reference.py [AMY_DIR]`, which needs
   `src/nn_crosscheck` built by a plain `make -C src` in the engine).
   The reference also freezes the engine's per-position
   piece/pawn/attack index sets and the `PHASE`/`PMASKS` selection
   (checked in `test_crosscheck.py` against the Python oracles and the
   C extraction library).
2. For export changes: export an `amy.net` and load it in the engine
   (`src/Amy`) — the loader gate is `nn_crosscheck` (bit-exact) or plain
   `Amy` (must print "Read weights.").  Note: `Amy --test` does *not*
   load weights (the `--test` flag exits inside option parsing, before
   `nn_read_weights()`), so it is a build/test-suite gate only.

## Phase branches (shared contract with the engine)

Per-phase hidden layers (amy's `PLAN-phase-branches.md`):

- **Criterion**: total non-pawn pieces, both sides (kings excluded),
  buckets of three: counts 0..2 → phase 0, 3..5 → 1, 6..8 → 2,
  9..12 → 3.  C is the arbiter: `nn_phase_index()` / `nn_phase_masks()`
  in `neural_net.c` (the criterion *and* the buckets *and* the
  boundary rule); `NN_PHASE_COUNT` is exported via
  `nn_export_constants` (Python literal in `features.py`, cross-checked
  by `test_constants_sync`).
- **`nn_phase_masks()`** = the record's active heads: always
  `nn_phase_index(p)`, **plus** the higher phase at the top of a
  bucket (counts 2, 5, 8).  The C extraction API returns the masks per
  record (`phase` out-param, all-zero for invalid records);
  `BpdDataset` exposes them as `ds.phase` (N x 4 0/1), with
  `ds.phase_index` = first active = the engine's hard switch.
- **Training**: the float net forwards all 4 branches; with mask
  `a` the prediction is `blended = (sum a*out) / sum a`, and the loss
  (train.py `_loss`) is the tanh-squeezed |.|^2.6 on the blend **plus
  the same term per active head**, so co-trained boundary heads cannot
  drift apart (t+d / t-d would fit the blend with zero loss).
- **Engine evaluation** uses exactly one branch (`nn_phase_index`;
  `QuantizedNet.evaluate(..., phase)` mirrors it bit-exactly, with the
  per-phase block of the selected phase; cross-checked against
  `nn_crosscheck`'s `PHASE`/`PMASKS` dumps).
- **No format soft-fallback**: old single-branch `amy.net` files
  fail the `wh_phase_count` header check (like any header mismatch);
  the pipeline must always emit the 4-branch format.

## BPD format

40-byte records: 4 x u64 board (64 nibbles; square `sq` = nibble `sq & 15`
of `pos[sq >> 4]`), u64 castling/EP specials, i32 score (millipawns,
white perspective), u8 turn, padding. Nibbles: 0 empty, 1-7 white
P..K, 8-14 black P..K, 5/12 also mark castling rooks (position-dependent),
15 = EP target square. Use `Position.epd()` / `position_from_record()` —
do not hand-roll nibble extraction. `BpdRecord.validate()` rejects
positions with pawns on rank 1 or 8: the pawn pair feature index
(`sq' - 8`) would run out of bounds, as in the engine.

## MLX notes (version-specific gotchas)

- `import mlx.optimizers as mxopt` (top-level module);
  `mxopt.AdamW(learning_rate=..., weight_decay=...)`.
- Do **not** use `mx.compile` with `outputs=model.state` around
  `nn.value_and_grad` + optimizer update: parameter arrays end up
  "without a primitive" and `mx.eval`/`np.array` fail. Plain (uncompiled)
  steps are fine — an epoch is ~2 s on M1 Pro.
- `NnueNet.save()` calls `mx.eval` first; keep that if you add compiled
  code paths.
- Model checkpoints are `weights.npz` (numpy), not safetensors.

## Conventions

- The network structure must stay changeable for experiments: all
  shapes and sizes derive from the constants in `features.py`
  (`N_FEATURES`, `ACCUMULATOR_SIZE`, `HIDDEN_SIZE_1/2`, `NN_PHASE_COUNT`,
  `PLANE_SIZE`) and `quantize.py` (`SCALE*`, `FIELD_ORDER`). Never hardcode
  architecture numbers (256, 32, 512, 8192, ...) in library code or
  tests; hand-computed test expectations are expressed in terms of
  those constants. Note that `tests/data/crosscheck_reference.json`
  is pinned to amy's C architecture — if the structure changes, the
  C side must be updated and the reference regenerated.
- Scores are millipawns everywhere on the outside; the float model works in
  units of `1/10000` pawn (`target = score_mp / 10000`, `SCALE_OUTPUT=10000`),
  and the float forward pass and the quantized pipeline stay linear in
  those units.  The *loss* is the exception: it is |.|^2.6 on `tanh(out)` vs
  `tanh(target)` (see `_loss` in train.py), so the trained outputs are
  squeezed into (-1, 1); float-val metrics are measured in that same
  squeezed space.  With per-phase branches the loss is taken on the
  active-head blend plus per-active-head (see "Phase branches").
- No mirror augmentation: with the king-half canonical frame, a
  position and its horizontal mirror image produce *identical* feature
  vectors (and the same target), so augmenting would only duplicate
  rows.  The old `--mirror-augment` flag (pre-canonicalization pipeline)
  has been removed.  `mirror_board` in position.py remains available for
  tests (e.g. the mirror-folding invariant in test_features.py).
- Keep the `amy.net` export byte-identical to amy's layout: a
  **96-byte** little-endian `WeightsHeader` (magic `0x57594D41`, whose file
  bytes read as "AMYW",
  then accumulator size 256, number of piece input features 22528,
  number of pawn input features 4560, 2 bytes of alignment padding, the
  number of attack input features 98304 as a **u32** (it doesn't fit in
  a u16, hence the padding), hidden layer sizes 32/32, per-field
  quantization codes 1/2/3 = int8/16/32 — input weights 1,
  input bias 2, h1 weights 2, h1 bias 3, h2 weights 2, h2 bias 3,
  output weights 3, output bias 3 (shared by all phases); the pawn and
  attack weights have *no* code and are read unconditionally as int8,
  then **u16 `wh_phase_count` = 4**, then `char[32]` `wh_name`
  (a codename from `codename()` in `code_name.py`, NUL-padded)
  and `char[32]` `wh_date` (the training date/time, full ISO8601
  timestamp, NUL-padded; both written via `write_network_quant`).
  See `weights_header()`.  The 94 field bytes are padded to
  sizeof(WeightsHeader) = 96 (4-byte alignment; sizeof is the
  arbiter) — pack with
  `struct.pack("<IHHH2xIHH8BH32s32s2x", ...)`; any other layout
  silently misaligns the whole file.  Followed by 32,241,680 bytes of
  field data (32,241,776 bytes total) in `write_network_quant`'s
  order: piece input weights, the shared 4560 x 256 int8 pawn block,
  the shared 98,304 x 256 int8 attack block, input bias, then
  **4 x {h1w, h1b, h2w, h2b, ow, ob}**, one phase at a time (exactly
  the read loop of `nn_read_weights_quantized()`).
- Style: `black` + `isort` (configured in `pyproject.toml`).
- Data lives in `evaluation_log.bin` in this directory (gitignored;
  grows with every selfplay session — tests must not read it). `tests/data/selfplay_sample.bin` is
  a frozen 5% snapshot (seed 42) used by the training smoke test;
  deliberately refresh it with `python tools/make_sample.py` and commit
  the result. `tests/data/bpd_test.bin` is a 5-record
  cross-implementation fixture (produced by amy's `bpd-test`).

## Layout

```
amy_training_nnue/
  bpd.py       40-byte record parse/encode, validation
  position.py  Position dataclass, EPD, nibble helpers
  features.py  king-relative feature indices + pawn pair indices
               (parity oracle for the C extraction; no longer on
               the runtime path)
  cfeatures.py ctypes binding to libamy_nn_extract (built by `make -C src`
               in the engine at the repo root, or AMY_NN_LIB): batch BPD ->
               piece/pawn/attack index rows + per-record phase masks,
               the C side is the reference implementation and
               validation arbiter
  dataset.py   BPD -> padded feature tensors + targets + phase masks
               (one C call per file; also stores the attack blocks)
  model.py     NnueNet (MLX float; shared input + 4 per-phase branches,
               blended call, `heads()` for per-branch outputs)
  quantize.py  QuantizedNet (int simulation, one Phase per branch),
               amy.net I/O (96-byte header, 4-branch payload)
  train.py     training loop, checkpoints, metrics, export
  cli.py       stats / inspect / train / export / evaluate
tests/         pytest suite incl. bit-exact C cross-check
tools/         make_crosscheck_reference.py
```

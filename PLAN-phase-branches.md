# Plan: Per-game-phase hidden layers (multi-branch NNUE)

## Goal

Increase NNUE evaluation quality by replacing the single hidden-layer + output
head with **4 parallel sets of identical structure**, one per game phase,
selected at evaluation time by total non-pawn piece count (both sides).
Hard switch between phases for now; each phase has its own output head.

## Fixed decisions (agreed)

| Decision              | Chosen                                                        |
|-----------------------|---------------------------------------------------------------|
| Number of phases      | 4                                                             |
| Phase criterion       | total non-pawn pieces, both sides (range 0..12)               |
| Switch                | hard switch (piece count crosses a boundary)                  |
| Output layer          | per phase (one head per phase set)                            |
| Input side            | **shared** (accumulators + piece/pawn/attack weights unchanged) |
| Hash table            | no invalidation; phase is a function of material, which is a function of `hkey` — same key ⟹ same phase, so scores stay consistent |
| Fast path             | `eval_material_balance` + `MaxPos` (search.c:818, next.c:770) remain phase-independent, untouched |

### Phase boundaries (final)

`nn_phase_index()` = `MIN(total_non_pawn / 3, NN_PHASE_COUNT - 1)`,
counted via bitboard population of the N/B/R/Q masks (kings excluded by
construction):

| Phase | total non-pawn pieces (W+B) | Typical stage            |
|-------|-----------------------------|--------------------------|
| 0     | 0..2                        | deep endgame             |
| 1     | 3..5                        | simplified midgame       |
| 2     | 6..8                        | typical opening/midgame  |
| 3     | 9..12                       | start of game            |

The mapping lives in one function (`nn_phase_index()`); the shared
contract with the training pipeline is the criterion *and* the buckets.

## Scope of code changes

### 1. `include/neural_net.h`
- Add `#define NN_PHASE_COUNT 4`.
- Declare `int nn_phase_index(const struct Position *p);` (shared contract
  with the Python training pipeline; also used by `nn_crosscheck`).
- No ABI-cap changes for `nn_extract` (input side untouched).

### 2. `src/neural_net.c`
- Weight globals become 3-D, e.g.
  ```c
  int16_t hidden_layer_1_weights_q[NN_PHASE_COUNT][HIDDEN_1_SIZE][2 * ACCUMULATOR_SIZE];
  int32_t hidden_layer_1_bias_q[NN_PHASE_COUNT][HIDDEN_1_SIZE];
  int16_t hidden_layer_2_weights_q[NN_PHASE_COUNT][HIDDEN_2_SIZE][HIDDEN_1_SIZE];
  int32_t hidden_layer_2_bias_q[NN_PHASE_COUNT][HIDDEN_2_SIZE];
  int32_t output_layer_weights_q[NN_PHASE_COUNT][HIDDEN_2_SIZE];
  int32_t output_layer_bias_q[NN_PHASE_COUNT][1];
  ```
  (~54 KiB per phase, trivial memory impact.)
- `nn_read_weights_quantized()`:
  - new `WeightsHeader` field `uint16_t wh_phase_count` (+ keep all existing
    asserts; a mismatched/old file fails the new assert exactly like any
    other header mismatch — no soft fallback, per decision "no hash table
    invalidation / loud failure on wrong file");
  - after the unchanged input-layer block, loop over phases reading the six
    per-phase blocks in fixed order: H1W, H1B, H2W, H2B, OW, OB.
- `nn_phase_index()`: `bitboard_count_bits()` over the N/B/R/Q masks of
  both sides, bucketed `MIN(count / 3, NN_PHASE_COUNT - 1)` (kings
  excluded by construction).
- `nn_evaluate_position()` split:
  - `nn_compute_score(p)`: the pure pipeline — attack-map refresh, phase
    selection, accumulator clamping, H1, quantize/ReLU, H2, quantize/ReLU,
    phase's own output head — indexed by `nn_phase_index(p)`. No score
    hash table, no fuzz, no MaxPos.
  - `nn_evaluate_position(p)`: caching wrapper — hash probe/store,
    fuzz, MaxPos. Phase selection stays deterministic per position, so
    cached scores remain internally consistent (no key changes needed).
  - Motivation: `ht_clear()` does not actually reset the score hash
    entries, so the phase-selection tests had no way to distinguish a
    recomputed score from a stale one; the pure function gives them a
    direct entry point into the real pipeline.
- `nn_randomize_weights()`: fill all `NN_PHASE_COUNT` sets, then **copy phase 0
  onto phases 1..3** so existing `test_neural_net.c` invariants (score
  invariance across piece moves etc.) keep passing; tests that need distinct
  phases call it in a "diverge" mode (see tests below).
- `nn_show_weights()` (DEBUG): dump one phase (phase 0) — or loop phases,
  cheap either way.
- `nn_validate_weights()`: unchanged (input side only).

### 3. `src/nn_crosscheck.c`
- Update the `extern` declarations to the 3-D arrays.
- `instrumented_eval()`: use the same `nn_phase_index()` from neural_net.c
  (no duplication of the bucket logic).
- New dump line `PHASE <i>` before the weight dumps so
  `amy-training-nnue/quantize.py` can index the right block and verify
  bit-exactly; dump the selected phase's arrays unchanged in layout.

### 4. `src/test_neural_net.c`
- **Unit test `nn_phase_index()`**: construct positions with 0..12 total
  non-pawn pieces (e.g. K + n rooks/queens on both sides, or reuse
  (e.g. K + n rooks/queens on both sides, or reuse
  `dbase_create_position_from_epd`) and assert the exact bucket for each
  count at the boundaries (2/3, 5/6, 8/9).
- **Phase-selection test**: on a phase-0 and a phase-3 position, call
  `nn_compute_score()` directly (no hash table involved); perturb
  `output_layer_bias_q[3][0]` and assert the phase-3 score changes while
  the phase-0 score is bit-identical; symmetric check for phase 0; and an
  untouched phase (1) must have no effect on either.
- **Existing tests must pass unchanged** — this is the regression gate that
  `nn_randomize_weights()` replicating phase 0 into all phases satisfies.
  (Also proves accumulator/NNUE update machinery is untouched.)

### 5. Training pipeline contract (sibling repo `amy-training-nnue`, doc-only here)
- Binary layout of `network.quant`:
  `WeightsHeader` (with new `wh_phase_count`) ‖ unchanged input blocks ‖
  `NN_PHASE_COUNT × (H1W ‖ H1B ‖ H2W ‖ H2B ‖ OW ‖ OB)`.
- Each training sample must be **labeled with the phase of its position using
  the same `nn_phase_index()` contract** (same criterion + boundaries) and
  its hidden/output losses computed only against that phase's weights.
- Shared (one) input/accumulator weights; 4 independent {H1, H2, output}
  branches. All existing quantization scales (SCALE_INPUT/SCALE/SCALE_H1,
  int16/int32 clamps) reused per branch without change.
- `QuantizedNet` in quantize.py gains a `phase` parameter for
  cross-checking (mirrors the C change in step 3).

### 6. Misc
- `CHANGELOG.md` entry (0.9.8-dev): new weight format, `network.quant` must be
  retrained; per-phase hidden layers selected by non-pawn piece count.
- `doc/Handbook.md`: one paragraph if it mentions the network format.
- `make format` after edits; no new files in the binary distribution
  (plan doc is repo-only).

## Out of scope (explicitly deferred)
- Soft/linear blending between adjacent phases near boundaries.
- Per-phase input/accumulator weights (would break incremental updates'
  shared assumption — requires a dedicated design, not this change).
- Any phase-dependent changes in the A/B fast path or hash keys.

## Implementation order (each step: `make && make check` clean)
1. `NN_PHASE_COUNT`, header field, 3-D arrays, loader loop,
   `nn_phase_index()` — build + `make check` (with replicated random weights,
   behaviour must be bit-identical to today for all test positions).
2. Wire phase into `nn_compute_score()` (split out of
   `nn_evaluate_position()`).
3. Add the new tests (phase index unit test + selection test).
4. `nn_crosscheck.c` + `PHASE` dump line.
5. Training-side changes in `amy-training-nnue` (separate PR/repo), bit-exact
   cross-check on sample EPDs covering all four phases.
6. `make distcheck` + final `make format`.

## Verification
- `make check`, `make distcheck` (CI runs the same).
- `./Amy --test` covers the new tests via `main_run_tests`.
- nn_crosscheck bit-exact dump comparison on: initial position (phase 3),
  one position per of the other phases, and positions at both sides of each
  boundary (e.g. 3 vs 4 total non-pawn pieces).
- Before/after strength: standard self-play/arena eval once real weights are
  trained (outside engine-repo scope).

## Risks / notes
- Eval discontinuity at phase boundaries: search may see a jump when a
  capture changes the count; accepted for now (hard switch is easier to train
  and debug). Revisit with blending if strength eval shows boundary
  artifacts.
- Training data near boundaries is unlambigous by construction (each position
  has exactly one phase), so no special handling needed.
- Old `network.quant` files abort on the new assert — intended; training
  pipeline must always produce the new format.

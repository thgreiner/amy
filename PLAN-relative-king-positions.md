# Plan: Relative King Positions Feature (Plane 10)

**Goal:** Add a new feature plane encoding the opponent king's position relative to
own king, to each side's accumulator. Especially valuable in endgames (king
proximity, opposition, shielding).

- New plane: `4096 * 10` (i.e. 11th plane), `N_FEATURES` goes from `64*64*10` to `64*64*11` (45056).
- One plane per accumulator: "enemy king relative to my king". Own king is the implicit
  origin of all planes, so this single plane fully determines both kings.
- Chosen approach: **Option B** — on a king move, full-rebuild the side-to-move
  accumulator + incremental delta (2 × `ACCUMULATOR_SIZE` int16 ops) on plane 10 of the
  opponent's accumulator. No other call sites needed.
- Weights file header gains a `wh_num_features` sanity field.

## Background: current feature layout

Per side, the accumulator (`accumulator_w_q` / `accumulator_b_q`) holds
bias + sum of contributions, in that side's own coordinate frame:

- **White frame:** no flipping. Origin = `kingSq[White]`.
- **Black frame:** all squares XORed with `0x38` (rank mirror, a1↔a8).
  Origin = `kingSq[Black] ^ 0x38`.

Index formulas (existing planes, `tp` in `Pawn..Queen` = 1..5):

| Accumulator | Feature | Index |
|---|---|---|
| own side | own piece type `tp` at `sq` | `4096*(tp-1) + 64*own_king + sq'` |
| own side | opponent piece type `tp` at `sq` | `4096*(tp+4) + 64*own_king + sq'` |

`sq'` / `own_king` are expressed in the owning side's frame (flipped for black).

Maintenance today:

- `nn_update_weights_for_piece()` — incremental, for every non-king move (both
  accumulators; a piece update touches its own plane in *both* sides' accumulators).
- `nn_update_weights_for_king()` — called from the 4 king-move sites in `dbase.c`
  (make/undo king move, make/undo castle); **full-rebuilds the side-to-move accumulator
  only**. This works today because each accumulator's features reference only its own
  king square.
- `nn_init_accumulator()` — from-scratch build at root/EPD setup.
- `nn_validate_weights()` — independent from-scratch recomputation of both accumulators,
  used as cross-check in tests.

## New feature definition

**Plane 10: opponent king relative to own king.**

```
idx(side, own_king, opp_king) = 4096*10 + 64*own_king' + opp_king'
```

concretely:

- `accumulator_w_q`: `40960 + 64*kingSq[White] + kingSq[Black]`
- `accumulator_b_q`: `40960 + 64*(kingSq[Black]^0x38) + (kingSq[White]^0x38)`

Max index = `4096*10 + 64*63 + 63 = 45055 = N_FEATURES - 1`. ✓

### Why the opponent's accumulator also changes on a king move

When White's king moves, plane 10 of **both** accumulators changes:

- white accumulator: "black king relative to white king" — white king moved.
- black accumulator: "white king relative to black king" — white king moved.

Option B handles this: side-to-move gets a full rebuild (already the case), the
opponent accumulator only needs the plane-10 delta, because all its other planes are
invariant under a side-to-move king move (its own pieces/king unchanged; side-to-move
non-king pieces unchanged, and in castling the rook is handled by the existing
`nn_update_weights_for_piece()` calls).

Delta for side `side` moving its king `old_king_sq -> p->kingSq[side]`
(called after `p->kingSq[side]` is already updated, as today):

```
opp  = OPP(side)
flip = (opp == Black) ? 0x38 : 0          // coordinate frame of opponent accumulator
base = 4096*10 + 64*(p->kingSq[opp] ^ flip)
nn_update_accumulator_sub(opp_acc, base + (old_king_sq ^ flip));
nn_update_accumulator_add(opp_acc, base + (p->kingSq[side] ^ flip));
```

Only one king ever moves per move, and only `dbase.c` lines 356/415/446/655 ever write
`kingSq`, each followed by `nn_update_weights_for_king()` — so no new incremental call
sites are needed anywhere.

## Changes

### 1. `src/neural_net.c`

- [x] `#define N_FEATURES (64 * 64 * 11)` (input weights grow 5.24 → 5.77 MB, static
      global, no problem).
- [x] `WeightsHeader`: insert `uint16_t wh_num_features;` after `wh_accumulator_size`.
- [x] `nn_read_weights_quantized()`: add
      `assert(header.wh_num_features == N_FEATURES);` next to the existing header
      asserts. (Catches stale weight files with a clear message instead of the opaque
      `fread` count assert. **Requires the external quantization script to write this
      field — see section "Training code".**)
- [x] New static helper (place after `nn_update_accumulator_sub`), used by init,
      king-rebuild, and validate — one shared from-scratch code path:

      ```c
      static void nn_build_accumulator(int16_t *accumulator_q, struct Position *p,
                                       int side) {
          int flip = (side == Black) ? 0x38 : 0;
          int king_sq = p->kingSq[side] ^ flip;

          memcpy(accumulator_q, input_layer_bias_q,
                 sizeof(int16_t) * ACCUMULATOR_SIZE);

          for (int tp = Pawn; tp <= Queen; tp++) {
              BitBoard mask = p->mask[side][tp];
              while (mask) {
                  int sq = bitboard_find_set_bit(mask);
                  mask &= mask - 1;
                  int index = 4096 * (tp - 1) + 64 * king_sq + (sq ^ flip);
                  nn_update_accumulator_add(accumulator_q, index);
              }

              mask = p->mask[OPP(side)][tp];
              while (mask) {
                  int sq = bitboard_find_set_bit(mask);
                  mask &= mask - 1;
                  int index = 4096 * (tp + 4) + 64 * king_sq + (sq ^ flip);
                  nn_update_accumulator_add(accumulator_q, index);
              }
          }

          /* NEW: opponent king relative to own king (plane 10) */
          int index = 4096 * 10 + 64 * king_sq + (p->kingSq[OPP(side)] ^ flip);
          nn_update_accumulator_add(accumulator_q, index);
      }
      ```

- [x] `nn_update_weights_for_king()` — new signature
      `void nn_update_weights_for_king(struct Position *p, int old_king_sq)`:
      full rebuild of the side-to-move accumulator via the helper, plus the plane-10
      delta on the opponent accumulator (see formulas above):

      ```c
      void nn_update_weights_for_king(struct Position *p, int old_king_sq) {
          int side = p->turn;
          int opp = OPP(side);
          int flip = (opp == Black) ? 0x38 : 0;

          nn_build_accumulator((side == White) ? p->accumulator_w_q
                                               : p->accumulator_b_q,
                               p, side);

          int16_t *opp_acc = (opp == White) ? p->accumulator_w_q
                                            : p->accumulator_b_q;
          int base = 4096 * 10 + 64 * (p->kingSq[opp] ^ flip);
          nn_update_accumulator_sub(opp_acc, base + (old_king_sq ^ flip));
          nn_update_accumulator_add(opp_acc, base + (p->kingSq[side] ^ flip));
      }
      ```

- [x] `nn_init_accumulator()`: replace the piece loops with
      `nn_build_accumulator(p->accumulator_w_q, p, White);`
      `nn_build_accumulator(p->accumulator_b_q, p, Black);`
      (Without the plane-10 contribution here, every freshly loaded position would
      silently miss the feature.)
- [x] `nn_validate_weights()`: replace the two inline from-scratch loops with
      `nn_build_accumulator()` on the local reference accumulators; keep the
      comparison/abort logic (and the `verbose` guard around the calls).
- [x] No changes needed: `nn_update_weights_for_piece()`, `nn_evaluate_position()`,
      `nn_randomize_weights()` (loops over `N_FEATURES`), `nn_show_weights()`.

### 2. `include/neural_net.h`

- [x] Update prototype:
      `void nn_update_weights_for_king(struct Position *p, int old_king_sq);`

### 3. `src/dbase.c` — the 4 call sites (all have `from`/`to` in scope)

- [x] `dbase_do_castle()` (~line 360): `nn_update_weights_for_king(p, from);`
- [x] `dbase_undo_castle()` (~line 419): `nn_update_weights_for_king(p, to);`
- [x] `dbase_do_move()` king branch (~line 571): `nn_update_weights_for_king(p, from);`
- [x] `dbase_undo_move()` king branch (~line 739): `nn_update_weights_for_king(p, to);`

Note: in castling the preceding `nn_update_weights_for_piece()` rook calls remain —
they are required for the *opponent's* accumulator (rook is a plane 5–9 piece there);
for the side-to-move accumulator they are redundant (overwritten by the rebuild),
which is harmless. Leave as is.

### 4. `src/test_neural_net.c`

- [x] Add `test_king_move()`: a bare king move plus a black king move (exercises the
      flip path), with do/undo validation — same pattern as the existing tests:

      ```c
      static void test_king_move(void) {
          struct Position *p =
              dbase_create_position_from_epd("4k3/8/8/8/8/8/8/4K3 w - -");

          nn_init_accumulator(p);
          nn_validate_weights(p);

          int move = dbase_make_move(e1, d2, 0);
          dbase_do_move(p, move);
          nn_validate_weights(p);

          dbase_undo_move(p, move);
          nn_validate_weights(p);

          move = dbase_make_move(e8, d8, 0);
          dbase_do_move(p, move);
          nn_validate_weights(p);

          dbase_undo_move(p, move);
          nn_validate_weights(p);

          dbase_free_position(p);
      }
      ```

- [x] Call it from `test_all_neural_net()`.
 - [x] Existing `test_castle()` already covers king + rook (delta path).

### 5. Training / quantization code (external repo — do after this lands)

- [ ] Input dimension per side: 11 planes instead of 10. New plane 10 semantics per
      side: **enemy king square relative to own king**, in the same coordinate frame as
      all other features (for black's side: board mirrored by XOR `0x38`, i.e. a1↔a8,
      own king at origin).
- [ ] `network.quant` writer: emit 45056 feature rows and the new
      `wh_num_features` header field (uint16, value 45056, placed after
      `wh_accumulator_size` — keep byte layout in sync with `WeightsHeader`).

## Verification / definition of done

- [x] Builds cleanly.
- [x] `Amy --test` → "All tests passed." (runs `test_all_neural_net()`, which
      randomizes weights first, so it does not depend on `network.quant`).
      ⚠️ If a stale `network.quant` (old header, 10 planes) is present, startup
      asserts on the new header field — rename/delete it until the retrained file
      exists (missing file just prints "No weights found."). (Verified: stale file
      triggers `Assertion failed: (header.wh_num_features == N_FEATURES)`.)
- [ ] After retraining: `network.quant` loads, and a simple game/eval smoke test runs
      with sensible scores.

## Performance impact

- King moves (incl. castle): 1 full accumulator rebuild (~32 × 128 int16 adds, already
  paid today) + 2 × 128 adds/subs for the opponent delta. Negligible.
- All other moves: unchanged.
- Memory: input weights +10% (≈ 0.5 MB).

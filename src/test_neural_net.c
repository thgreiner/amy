#include "attack_map.h"
#include "dbase.h"
#include "hashtable.h"
#include "inline.h"
#include "neural_net.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * The tests run on randomized in-memory weights (nn_randomize_weights
 * fills input_attack_weights_q); the file-loaded weights are exercised
 * by the engine itself on startup. The guard test below checks that
 * the randomization is non-zero, so that the attack-related tests in
 * this file are not vacuous.
 */
extern int8_t input_attack_weights_q[N_ATTACK_FEATURES][ACCUMULATOR_SIZE];
extern int32_t output_layer_bias_q[NN_PHASE_COUNT][1];

/* Must mirror neural_net.c: fixed-point scale of the output layer. */
#define NN_TEST_SCALE 8192

/*
 * Guard against a vacuous test suite: nn_randomize_weights() must have
 * produced non-zero attack weights, otherwise every attack-related test
 * in this file would pass even if the attack contribution were never
 * applied to the accumulators.
 */
static void test_randomized_attack_weights(void) {
    bool any_nonzero = false;

    for (int i = 0; i < N_ATTACK_FEATURES && !any_nonzero; i++) {
        for (int j = 0; j < ACCUMULATOR_SIZE; j++) {
            if (input_attack_weights_q[i][j] != 0) {
                any_nonzero = true;
                break;
            }
        }
    }

    assert(any_nonzero);
}

/*
 * Assert that the stored attack map equals the expected list of
 * indexes (exact match, including order: the maps are sorted).
 */
static void assert_attack_map_eq(const IntSetSorted *actual,
                                 const int expected[], size_t n) {
    assert(intset_sorted_count(actual) == n);
    for (size_t i = 0; i < n; i++) {
        assert(actual->data[i] == expected[i]);
    }
}

/*
 * Assert that the stored attack maps equal freshly computed ones, i.e.
 * that they are up to date for the current position.
 */
static void assert_attack_maps_up_to_date(Position *p) {
    IntSetSorted w, b;

    intset_sorted_init(&w);
    intset_sorted_init(&b);
    am_compute_attack_maps(p, &w, &b);

    assert_attack_map_eq(&p->attack_map_w, w.data, w.count);
    assert_attack_map_eq(&p->attack_map_b, b.data, b.count);
}

/*
 * Assert that a hand-crafted move is legal in the given position.
 * dbase_do_move() does not check legality; an illegal move would
 * corrupt the position state (pieces and masks), and any failure
 * afterwards would be a ghost.
 */
static void assert_move_legal(Position *p, move_t move) {
    heap_t heap = heap_allocate();
    bool legal = false;

    int count = dbase_legal_moves(p, heap);

    for (int i = 0; i < count; i++) {
        if (heap->data[heap->current_section->start + i] == move) {
            legal = true;
            break;
        }
    }

    heap_free(heap);
    assert(legal);
}

/*
 * A knight move changes the piece accumulator only; the attack maps
 * (all defence features in the initial position) are unaffected, but
 * the update round must leave them (correctly) unchanged.
 */
static void test_knight_move(void) {
    Position *p = dbase_initial_position();

    nn_init_accumulator(p);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    nn_validate_weights(p);

    int move = dbase_make_move(g1, f3, 0);
    assert_move_legal(p, move);
    dbase_do_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    nn_validate_weights(p);

    dbase_undo_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * A promotion changes the piece accumulator (rook -> queen). The
 * promoted queen on h8 attacks the g7 pawn, so the update after the
 * move must add exactly that one feature to the black map (the white
 * queen's attack on the black pawn is stored in the black frame,
 * keyed on the black king f8, mirror 0x3f):
 *   Qh8xg7: plane 19, sq 0*64+9=9 -> 77833
 * (h8 63^0x3f = 0, g7 54^0x3f = 9)
 */
static void test_promotion(void) {
    Position *p = dbase_create_position_from_epd(
        "3r1k2/p3qppP/8/3p4/3r4/5QP1/P4PB1/4R1K1 w - -");

    int expected_w_before[] = {168530, 193674, 193681, 213066, 221257};
    int expected_w_after[] = {168530, 193674, 193681, 213066, 221257};

    int expected_b_before[] = {77515,  80522,  80540,  172316, 174364,
                               193226, 193231, 205508, 213129, 213130};
    int expected_b_after[] = {77515,  77833,  80522,  80540,  172316, 174364,
                              193226, 193231, 205508, 213129, 213130};

    nn_init_accumulator(p);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w_before, 5);
    assert_attack_map_eq(&p->attack_map_b, expected_b_before, 10);
    nn_validate_weights(p);

    int move = dbase_make_promotion(h7, h8, Queen, 0);
    assert_move_legal(p, move);
    dbase_do_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w_after, 5);
    assert_attack_map_eq(&p->attack_map_b, expected_b_after, 11);
    nn_validate_weights(p);

    dbase_undo_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w_before, 5);
    assert_attack_map_eq(&p->attack_map_b, expected_b_before, 10);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * Castling moves the king (full accumulator rebuild) and a rook
 * (piece update), so the stored attack maps must be refreshed.
 * The white king e1 -> g1 stays in the same mirror frame (both on
 * files e..h), but the king and the rook (h1 -> f1) now defend
 * different squares, so the white map changes; the black map is
 * unchanged.  The attack (offset 0) features are:

   black map (black frame, king e8, mirror 0x3f):
     Nf3xe5: plane 4,  sq 42*64+27  -> 19099
     Bb5xc6: plane 10, sq 30*64+21  -> 42901
   white map (white frame, king e1/g1, mirror 7):
     Nf6xe4: plane 4,  sq 42*64+27  -> 19099
 */
static void test_castle(void) {
    Position *p = dbase_create_position_from_epd(
        "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq -");

    int expected_w_before[] = {19099,  115282, 131468, 132232, 132236, 151884,
                               151886, 172040, 172495, 176582, 192780, 192781,
                               196882, 200965, 213194, 213196};
    int expected_b[] = {19099,  42901,  115282, 115477, 115605, 132232,
                        132236, 132431, 132443, 148804, 151689, 151884,
                        151886, 172040, 172495, 180226, 180677, 192780,
                        192781, 196882, 200965, 213194, 213196, 221378};
    int expected_w_after[] = {19099,  115282, 131468, 132232, 132236,
                              151884, 151886, 166274, 172170, 172495,
                              176582, 188548, 192780, 192781, 196882,
                              200965, 213064, 213065, 213066};

    nn_init_accumulator(p);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w_before, 16);
    assert_attack_map_eq(&p->attack_map_b, expected_b, 24);
    nn_validate_weights(p);

    int move = dbase_make_move(e1, g1, M_SCASTLE);
    assert_move_legal(p, move);
    dbase_do_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w_after, 19);
    assert_attack_map_eq(&p->attack_map_b, expected_b, 24);
    nn_validate_weights(p);

    dbase_undo_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w_before, 16);
    assert_attack_map_eq(&p->attack_map_b, expected_b, 24);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * A king move triggers a full rebuild of the side-to-move accumulator
 * (no attack contribution), so the stored attack map of that side
 * must be reset for the accumulator and the map to stay consistent.
 */
static void test_king_move(void) {
    Position *p = dbase_create_position_from_epd("4k3/8/8/8/8/8/8/4K3 w - -");

    nn_init_accumulator(p);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    nn_validate_weights(p);

    move_t move1 = dbase_make_move(e1, d2, 0);
    assert_move_legal(p, move1);
    dbase_do_move(p, move1);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    nn_validate_weights(p);

    move_t move2 = dbase_make_move(e8, d8, 0);
    assert_move_legal(p, move2);
    dbase_do_move(p, move2);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    nn_validate_weights(p);

    dbase_undo_move(p, move2);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    nn_validate_weights(p);

    dbase_undo_move(p, move1);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * A white pawn captures the black d5 pawn en passant on d6. The
 * white bishop f1 attacks the a6 pawn (e2 is empty) in both
 * positions; the feature lives in the black map in the black frame
 * (king e8, mirror 0x3f):
   Bf1xa6: plane 9, sq 58*64+23=3735 -> 40599
 * After the capture the black queen d8 attacks the pawn on d6
 * (d7 is empty); that feature lives in the white map in the white
 * frame (king e1, mirror 7):
   Qd8xd6: plane 19, sq 60*64+44=3884 -> 81708
 * The black e7 pawn also attacks d6, but pawn -> pawn attacks are not
 * part of the attack maps.
 */
static void test_en_passant(void) {
    Position *p = dbase_create_position_from_epd(
        "rnbqkbnr/1pp1pppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6");

    int expected_w_before[] = {131468, 151689, 151884, 151886, 172040,
                               172495, 176129, 176582, 192780, 192781,
                               200965, 213194, 213196, 221378};
    int expected_w_after[] = {
        81708,  131468, 151689, 151884, 151886, 172040, 172495, 176129,
        176582, 192780, 192781, 200965, 213194, 213196, 221378,
    };
    int expected_b_before[] = {40599,  131147, 131479, 151689, 151691, 151886,
                               172040, 172503, 176129, 176582, 192779, 192781,
                               192796, 200965, 213194, 213195, 221378};
    int expected_b_after[] = {40599,  131147, 131479, 151689, 151691, 151886,
                              172040, 172503, 176129, 176582, 192779, 192781,
                              200965, 213194, 213195, 221378};

    nn_init_accumulator(p);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w_before, 14);
    assert_attack_map_eq(&p->attack_map_b, expected_b_before, 17);
    nn_validate_weights(p);

    int move = dbase_make_move(e5, d6, M_ENPASSANT);
    assert_move_legal(p, move);
    dbase_do_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w_after, 15);
    assert_attack_map_eq(&p->attack_map_b, expected_b_after, 16);
    nn_validate_weights(p);

    dbase_undo_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w_before, 14);
    assert_attack_map_eq(&p->attack_map_b, expected_b_before, 17);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * In the initial position no piece attacks an enemy piece, so both
 * attack maps hold only the 18 own-piece defence features each (own
 * pieces defending own pieces, offset 28).  The first update must
 * store those maps without changing the accumulators.
 */
static void test_attack_update_initial_position(void) {
    Position *p = dbase_initial_position();

    nn_init_accumulator(p);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert(p->attack_map_w.count == 18);
    assert(p->attack_map_b.count == 18);
    assert_attack_maps_up_to_date(p);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * Both rooks attack each other and the white king defends the rook.
 * In the white frame (king a2, no mirror) the white map holds the
 * black attack Rg1xb1 (offset 0) and the white defence Ka2->b1
 * (offset 28); in the black frame (king e8, mirror 0x3f) the black
 * map holds the white attack Rb1xg1.  After the capture the maps
 * must be empty again; undo re-adds them.

   white frame:
     Rg1xb1: plane 17,       sq 6*64+1=38    -> 70017
     Ka2->b1: plane 27, +28, sq 8*64+1=513   -> 225793
   black frame:
     Rb1xg1: plane 17,       sq 62*64+57=4025 -> 73657
 */
static void test_attack_update_rook(void) {
    Position *p =
        dbase_create_position_from_epd("4k3/8/8/8/8/8/K7/1R4r1 w - -");

    int expected_w[] = {70017, 225793};
    int expected_b[] = {73657};
    int16_t acc_w[ACCUMULATOR_SIZE], acc_b[ACCUMULATOR_SIZE];

    nn_init_accumulator(p);
    nn_validate_weights(p);

    memcpy(acc_w, p->accumulator_w_q, sizeof(acc_w));
    memcpy(acc_b, p->accumulator_b_q, sizeof(acc_b));

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w, 2);
    assert_attack_map_eq(&p->attack_map_b, expected_b, 1);
    nn_validate_weights(p);

    /* The update must actually have changed the accumulators. */
    assert(memcmp(acc_w, p->accumulator_w_q, sizeof(acc_w)) != 0);
    assert(memcmp(acc_b, p->accumulator_b_q, sizeof(acc_b)) != 0);

    int move = dbase_make_move(b1, g1, M_CAPTURE);
    assert_move_legal(p, move);
    dbase_do_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, NULL, 0);
    assert_attack_map_eq(&p->attack_map_b, NULL, 0);
    nn_validate_weights(p);

    dbase_undo_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w, 2);
    assert_attack_map_eq(&p->attack_map_b, expected_b, 1);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * The white queen on d4 attacks the knight on c5, the bishop on d6
 * (rook move) and the rook on e5 (bishop move); all three are stored
 * in the black map in the black frame (king h8, mirror 0x3f).  The
 * black pieces defend each other (black map, offset 28): Bd6->c5,
 * Bd6->e5, Re5->c5.  The white map is empty: nothing of black attacks
 * queen or king.
 * Capturing the knight removes all three white attack features: from
 * c5 the queen reaches the bishop only via a bishop move and the
 * rook only via a rook move, neither of which is counted.  The black
 * bishop and rook now attack the queen on c5 (white map, white frame,
 * king b1, no mirror).

   before (black frame, mirror 0x3f):
     Qd4xc5:  plane 20,      sq 36*64+29  -> 84253
     Qd4xd6:  plane 21,      sq 36*64+20  -> 88340
     Qd4xe5:  plane 22,      sq 36*64+27  -> 92443
     Bd6->c5: plane 10, +28, sq 20*64+29  -> 156957
     Bd6->e5: plane 12, +28, sq 20*64+27  -> 165147
     Re5->c5: plane 15, +28, sq 27*64+29  -> 177885
   after Qxc5 (white frame, no mirror):
     Bd6xc5:  plane 13,      sq 43*64+34  -> 56034
     Re5xc5:  plane 18,      sq 36*64+34  -> 76066
   black map after Qxc5: only Bd6->e5 remains -> 165147
 */
static void test_attack_update_queen(void) {
    Position *p =
        dbase_create_position_from_epd("6k1/8/3b4/2n1r3/3Q4/8/8/1K6 w - -");

    int expected_b_before[] = {84253, 88340, 92443, 156957, 165147, 177885};
    int expected_w_after[] = {56034, 76066};
    int expected_b_after[] = {165147};

    nn_init_accumulator(p);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, NULL, 0);
    assert_attack_map_eq(&p->attack_map_b, expected_b_before, 6);
    nn_validate_weights(p);

    int move = dbase_make_move(d4, c5, M_CAPTURE);
    assert_move_legal(p, move);
    dbase_do_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w_after, 2);
    assert_attack_map_eq(&p->attack_map_b, expected_b_after, 1);
    nn_validate_weights(p);

    dbase_undo_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, NULL, 0);
    assert_attack_map_eq(&p->attack_map_b, expected_b_before, 6);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * The white rook on a4 attacks the black knight on a5; the feature
 * lives in the black map in the black frame (king d8 on file d:
 * rank flip 0x38, no horizontal mirror).  Moving the white king
 * Ke2-d2 crosses the white mirror boundary, but the white map is
 * empty and the black frame is unchanged, so the feature stays at
 * 63512.  Moving the black king Kd8-e8 crosses the black mirror
 * boundary and remaps the feature; each undo restores the previous
 * value.

   black frame, king d8 (mirror 0x38):
     Ra4xa5: plane 15, sq 32*64+24=2072  -> 63512
   black frame, king e8 (mirror 0x3f):
     Ra4xa5: plane 15, sq 39*64+31=2527  -> 63967
 */
static void test_attack_update_king_mirror(void) {
    Position *p = dbase_create_position_from_epd("3k4/8/8/n7/R7/8/4K3/8 w - -");

    int expected_b_d8[] = {63512};
    int expected_b_e8[] = {63967};

    nn_init_accumulator(p);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, NULL, 0);
    assert_attack_map_eq(&p->attack_map_b, expected_b_d8, 1);
    nn_validate_weights(p);

    int move1 = dbase_make_move(e2, d2, 0);
    assert_move_legal(p, move1);
    dbase_do_move(p, move1);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, NULL, 0);
    assert_attack_map_eq(&p->attack_map_b, expected_b_d8, 1);
    nn_validate_weights(p);

    int move2 = dbase_make_move(d8, e8, 0);
    assert_move_legal(p, move2);
    dbase_do_move(p, move2);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, NULL, 0);
    assert_attack_map_eq(&p->attack_map_b, expected_b_e8, 1);
    nn_validate_weights(p);

    dbase_undo_move(p, move2);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, NULL, 0);
    assert_attack_map_eq(&p->attack_map_b, expected_b_d8, 1);
    nn_validate_weights(p);

    dbase_undo_move(p, move1);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, NULL, 0);
    assert_attack_map_eq(&p->attack_map_b, expected_b_d8, 1);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * Updating already up to date attack maps must be a no-op: the
 * accumulator must not change.
 */
static void test_attack_update_idempotent(void) {
    Position *p =
        dbase_create_position_from_epd("4k3/8/8/8/8/8/K7/1R4r1 w - -");

    int16_t acc_w[ACCUMULATOR_SIZE], acc_b[ACCUMULATOR_SIZE];

    nn_init_accumulator(p);
    nn_update_attack_maps(p);
    nn_validate_weights(p);

    memcpy(acc_w, p->accumulator_w_q, sizeof(acc_w));
    memcpy(acc_b, p->accumulator_b_q, sizeof(acc_b));

    nn_update_attack_maps(p);

    assert(memcmp(acc_w, p->accumulator_w_q, sizeof(acc_w)) == 0);
    assert(memcmp(acc_b, p->accumulator_b_q, sizeof(acc_b)) == 0);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * nn_evaluate_position() must refresh the stored attack maps before
 * the accumulator is used. After a capture the stored maps are stale;
 * the evaluation must bring them up to date.
 */
static void test_evaluate_updates_attack_maps(void) {
    Position *p =
        dbase_create_position_from_epd("4k3/8/8/8/8/8/K7/1R4r1 w - -");

    /* The test needs a working hash table for nn_evaluate_position(). */
    if (L_HT_Size == 0) {
        ht_allocate();
    }

    /* assert_move_legal() may reset the stored attack maps (it
     * do/undoes king moves while testing legality); do it before the
     * maps are filled, and let nn_update_attack_maps() rebuild the
     * intended state right afterwards.
     */
    move_t move = dbase_make_move(b1, g1, M_CAPTURE);
    assert_move_legal(p, move);

    nn_init_accumulator(p);
    nn_update_attack_maps(p);
    nn_validate_weights(p);

    dbase_do_move(p, move);

    /* The stored maps are stale now; the evaluation must fix that. */
    assert(p->attack_map_w.count == 2);
    assert(p->attack_map_b.count == 1);

    nn_evaluate_position(p);

    assert_attack_map_eq(&p->attack_map_w, NULL, 0);
    assert_attack_map_eq(&p->attack_map_b, NULL, 0);
    assert_attack_maps_up_to_date(p);
    nn_validate_weights(p);

    dbase_undo_move(p, move);
    nn_update_attack_maps(p);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * The MaxPos state lives in the position (MaxPosState, see
 * running_average.h), so that parallel search threads observe the
 * statistics of their own search line. Check initialization, state
 * independence between positions, advancement on evaluation, and
 * propagation through dbase_clone_position().
 */
static void test_maxpos_state(void) {
    /* A position no other test evaluates: the score hash table must
     * not hold its key, so the evaluation below is computed fresh and
     * advances the state exactly once. */
    Position *p = dbase_create_position_from_epd("8/8/8/8/8/8/4k3/4K3 b - -");
    Position *q = dbase_initial_position();

    /* The test needs a working hash table for nn_evaluate_position(). */
    if (L_HT_Size == 0) {
        ht_allocate();
    }

    /* MaxPosInit is 2000 (see neural_net.c). */
    nn_eval_init(p);
    nn_eval_init(q);
    assert(p->maxpos_state.maxpos == 2000);
    assert(p->maxpos_state.ra_pos.ra_sum == 0);
    assert(p->maxpos_state.ra_pos.ra_index == 0);

    /* Evaluating p must not touch q's state. */
    nn_evaluate_position(p);
    assert(q->maxpos_state.maxpos == 2000);
    assert(q->maxpos_state.ra_pos.ra_sum == 0);
    assert(q->maxpos_state.ra_pos.ra_index == 0);

    /* p's state advanced by exactly one sample, and MaxPos stays
     * within the documented Pawn..Queen band. */
    assert(p->maxpos_state.ra_pos.ra_index == 1);
    assert(p->maxpos_state.ra_pos_sq.ra_index == 1);
    assert(p->maxpos_state.maxpos >= 1000 && p->maxpos_state.maxpos <= 11000);

    /* Cloning copies the state; re-initialization re-seeds it. */
    Position *c = dbase_clone_position(p);
    assert(memcmp(&c->maxpos_state, &p->maxpos_state, sizeof(MaxPosState)) ==
           0);
    nn_eval_init(c);
    assert(c->maxpos_state.maxpos == 2000);
    assert(c->maxpos_state.ra_pos.ra_sum == 0);

    dbase_free_position(p);
    dbase_free_position(q);
    dbase_free_position(c);
}

static void stress_position(const char *epd, int iterations) {
    Position *p = dbase_create_position_from_epd((char *)epd);
    nn_init_accumulator(p);
    nn_validate_weights(p);

    for (int i = 0; i < iterations; i++) {
        heap_t heap = heap_allocate();
        heap_push_section(heap);
        int count = dbase_legal_moves(p, heap);
        heap_pop_section(heap);

        if (count == 0) {
            heap_free(heap);
            continue;
        }

        move_t move = heap->data[heap->current_section->start + rand() % count];
        heap_free(heap);

        dbase_do_move(p, move);
        nn_validate_weights(p);

        nn_update_attack_maps(p);
        assert_attack_maps_up_to_date(p);
        nn_validate_weights(p);

        dbase_undo_move(p, move);
        nn_validate_weights(p);

        nn_update_attack_maps(p);
        assert_attack_maps_up_to_date(p);
        nn_validate_weights(p);
    }

    dbase_free_position(p);
}

static void test_random_moves(void) {
    stress_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -",
                    1000);
    stress_position(
        "r1bqkbnr/pppppppp/2n5/4Q3/4P3/8/PPPPPPPP/RNBQKBNR b KQkq -", 1000);
    stress_position(
        "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq -",
        2000);
    stress_position("rnbqkbnr/1pp1pppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6",
                    1000);
    stress_position("3r1k2/p3qppP/8/3p4/3r4/5QP1/P4PB1/4R1K1 w - -", 1000);
}

/*
 * Game phases: total number of non-pawn pieces (both sides), grouped
 * into buckets of three, mirroring nn_phase_index():
 *   0..2   -> phase 0
 *   3..5   -> phase 1
 *   6..8   -> phase 2
 *   9..12  -> phase 3
 */
static const char *epd_with_n_nonpawn(int n) {
    int w = (n > 8) ? 8 : n;
    int b = n - w;
    static char epd[128];
    char rank7[10] = {0};
    char rank2[10] = {0};

    for (int i = 0; i < b; i++) {
        rank7[i] = 'r';
    }

    for (int i = 0; i < w; i++) {
        rank2[i] = 'R';
    }

    /* Kings on a1/h8, rooks a7.. and a2.. (no checks, legal positions). */
    snprintf(epd, sizeof(epd), "4k3/%s%d/8/8/8/8/%s%d/K7 b - -", rank7, 8 - b,
             rank2, 8 - w);
    return epd;
}

static void test_phase_index(void) {
    static const int counts[] = {0, 3, 4, 6, 7, 9, 10, 12};
    static const int expected[] = {0, 1, 1, 2, 2, 3, 3, 3};

    for (int i = 0; i < 8; i++) {
        Position *p = dbase_create_position_from_epd(
            (char *)epd_with_n_nonpawn(counts[i]));

        assert(nn_phase_index(p) == expected[i]);
        dbase_free_position(p);
    }
}

static void test_phase_masks(void) {
    /* Non-edge buckets: exactly nn_phase_index() active. */
    static const int counts[] = {0, 1, 3, 4, 6, 7, 9, 10, 11, 12};
    for (size_t i = 0; i < 10; i++) {
        Position *p = dbase_create_position_from_epd(
            (char *)epd_with_n_nonpawn(counts[i]));
        int masks[NN_PHASE_COUNT];
        nn_phase_masks(p, masks);
        int phase = nn_phase_index(p);
        for (int k = 0; k < NN_PHASE_COUNT; k++) {
            assert(masks[k] == (k == phase));
        }
        dbase_free_position(p);
    }

    /* Bucket tops (counts 2, 5, 8): the phase itself and the higher
     * one are both active (training co-trains both heads). */
    for (int phase = 0; phase < NN_PHASE_COUNT - 1; phase++) {
        Position *p = dbase_create_position_from_epd(
            (char *)epd_with_n_nonpawn(phase * 3 + 2));
        int masks[NN_PHASE_COUNT];
        nn_phase_masks(p, masks);
        int n_active = 0;
        for (int k = 0; k < NN_PHASE_COUNT; k++) {
            assert(masks[k] == (k == phase || k == phase + 1));
            n_active += masks[k];
        }
        assert(n_active == 2);
        dbase_free_position(p);
    }
}

/*
 * The hard switch must select exactly one phase's hidden/output block:
 * perturbing one phase's output bias changes the score of positions
 * in that phase only, while positions in other phases must score
 * identically. A delta of 100 * SCALE on the output bias adds 100
 * output units (120+ final cp), which is far beyond the rounding
 * tolerance of the final two divisions.
 * The randomized weights replicate phase 0 onto the other phases,
 * but the phase index is still the discriminating selection step: p0
 * evaluates via phase 0, p3 via phase 3, and an untouched phase (1)
 * must have no effect at all.
 */
static void test_phase_selection(void) {
    int delta = 100 * NN_TEST_SCALE;

    /* Phase 0: kings only. */
    Position *p0 = dbase_create_position_from_epd("4k3/8/8/8/8/8/8/K7 b - -");
    assert(nn_phase_index(p0) == 0);

    /* Phase 3: 12 non-pawn pieces. */
    Position *p3 = dbase_initial_position();
    assert(nn_phase_index(p3) == 3);

    nn_init_accumulator(p0);
    nn_update_attack_maps(p0);
    nn_validate_weights(p0);

    nn_init_accumulator(p3);
    nn_update_attack_maps(p3);
    nn_validate_weights(p3);

    /*
     * The pure pipeline (no score hash table) is exercised directly,
     * so stale cached scores cannot mask a wrong phase selection.
     */
    int base0 = nn_compute_score(p0);
    int base3 = nn_compute_score(p3);

    /* Phase 3 is perturbed: p3 (phase 3) changes, p0 (phase 0) does not. */
    output_layer_bias_q[3][0] += delta;
    assert(nn_compute_score(p3) > base3);
    assert(nn_compute_score(p0) == base0);
    output_layer_bias_q[3][0] -= delta;

    /* Phase 0 is perturbed: p0 changes, p3 does not. */
    output_layer_bias_q[0][0] += delta;
    assert(nn_compute_score(p0) > base0);
    assert(nn_compute_score(p3) == base3);
    output_layer_bias_q[0][0] -= delta;

    /* An untouched phase (1) must have no effect on either side. */
    output_layer_bias_q[1][0] += delta;
    assert(nn_compute_score(p0) == base0);
    assert(nn_compute_score(p3) == base3);
    output_layer_bias_q[1][0] -= delta;

    dbase_free_position(p0);
    dbase_free_position(p3);
}

void test_all_neural_net(void) {
    nn_randomize_weights();

    test_randomized_attack_weights();
    test_knight_move();
    test_promotion();
    test_castle();
    test_king_move();
    test_en_passant();

    test_attack_update_initial_position();
    test_attack_update_rook();
    test_attack_update_queen();
    test_attack_update_king_mirror();
    test_attack_update_idempotent();
    test_evaluate_updates_attack_maps();
    test_maxpos_state();

    test_phase_index();
    test_phase_masks();
    test_phase_selection();

    test_random_moves();
}

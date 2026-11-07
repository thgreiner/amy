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
static void assert_attack_map_eq(IntSetSorted *actual, const int expected[],
                                 size_t n) {
    assert(intset_sorted_count(actual) == n);
    for (size_t i = 0; i < n; i++) {
        assert(actual->data[i] == expected[i]);
    }
}

/*
 * Assert that the stored attack maps equal freshly computed ones, i.e.
 * that they are up to date for the current position.
 */
static void assert_attack_maps_up_to_date(struct Position *p) {
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
static void assert_move_legal(struct Position *p, move_t move) {
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
 * stay empty in this position, but the update round must leave them
 * (correctly) unchanged.
 */
static void test_knight_move(void) {
    struct Position *p = dbase_initial_position();

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
 * move must add exactly that one white feature:
 *   Queen h8 -> Pawn g7: plane 19, sq 56*64+49=3633 -> 81457
 * (from' 56, to' 49: king on g1, mirror 7)
 */
static void test_promotion(void) {
    struct Position *p = dbase_create_position_from_epd(
        "3r1k2/p3qppP/8/3p4/3r4/5QP1/P4PB1/4R1K1 w - -");

    int expected_w_before[] = {73971, 79012, 79026};
    int expected_w_after[] = {73971, 79012, 79026, 81457};

    nn_init_accumulator(p);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w_before, 3);
    assert_attack_map_eq(&p->attack_map_b, NULL, 0);
    nn_validate_weights(p);

    int move = dbase_make_promotion(h7, h8, Queen, 0);
    assert_move_legal(p, move);
    dbase_do_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w_after, 4);
    assert_attack_map_eq(&p->attack_map_b, NULL, 0);
    nn_validate_weights(p);

    dbase_undo_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w_before, 3);
    assert_attack_map_eq(&p->attack_map_b, NULL, 0);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * Castling moves the king (full accumulator rebuild) and a rook
 * (piece update). The stored attack maps must be refreshed afterwards;
 * they are unchanged by the move itself (white king e1 -> g1 stays in
 * the same mirror frame, and the rook on f1 attacks no enemy piece).

   white: Knight f3 -> Pawn e5: plane 4,  sq 21*64+37  -> 17571
          Bishop b5 -> Knight c6: plane 10, sq 11*64+42 -> 43437
          (king on e1/g1, mirror 7)
   black: Knight f6 -> Pawn e4: plane 4,  sq 16*64+11  -> 17571
          (king on e8, mirror 0x3f)
 */
static void test_castle(void) {
    struct Position *p = dbase_create_position_from_epd(
        "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq -");

    int expected_w[] = {17571, 43437};
    int expected_b[] = {17571};

    nn_init_accumulator(p);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w, 2);
    assert_attack_map_eq(&p->attack_map_b, expected_b, 1);
    nn_validate_weights(p);

    int move = dbase_make_move(e1, g1, M_SCASTLE);
    assert_move_legal(p, move);
    dbase_do_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w, 2);
    assert_attack_map_eq(&p->attack_map_b, expected_b, 1);
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
 * A king move triggers a full rebuild of the side-to-move accumulator
 * (no attack contribution), so the stored attack map of that side
 * must be reset for the accumulator and the map to stay consistent.
 */
static void test_king_move(void) {
    struct Position *p =
        dbase_create_position_from_epd("4k3/8/8/8/8/8/8/4K3 w - -");

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
 * A white pawn captures the black d5 pawn en passant on d6. The white
 * bishop f1 attacks the a6 pawn (e2 is empty) in both positions:
   Bishop f1 -> Pawn a6: plane 9, sq 2*64+47=175 -> 37039
   (white king e1, mirror 7)
 * After the capture the black queen d8 attacks the pawn on d6
 * (d7 is empty):
   Queen d8 -> Pawn d6: plane 19, sq 4*64+20=276 -> 78100
   (black king e8, mirror 0x3f)
 * The black e7 pawn also attacks d6, but pawn -> pawn attacks are not
 * part of the attack maps.
 */
static void test_en_passant(void) {
    struct Position *p = dbase_create_position_from_epd(
        "rnbqkbnr/1pp1pppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6");

    int expected_w[] = {37039};
    int expected_b_after[] = {78100};

    nn_init_accumulator(p);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w, 1);
    assert_attack_map_eq(&p->attack_map_b, NULL, 0);
    nn_validate_weights(p);

    int move = dbase_make_move(e5, d6, M_ENPASSANT);
    assert_move_legal(p, move);
    dbase_do_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w, 1);
    assert_attack_map_eq(&p->attack_map_b, expected_b_after, 1);
    nn_validate_weights(p);

    dbase_undo_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w, 1);
    assert_attack_map_eq(&p->attack_map_b, NULL, 0);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * In the initial position both attack maps are empty, so the first
 * update must not change the accumulators and must store the empty
 * maps.
 */
static void test_attack_update_initial_position(void) {
    struct Position *p = dbase_initial_position();

    nn_init_accumulator(p);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert(p->attack_map_w.count == 0);
    assert(p->attack_map_b.count == 0);
    assert_attack_maps_up_to_date(p);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * Both rooks attack each other. The first update adds both features;
 * after the capture the maps must be empty again; undo re-adds them.

   white: Rook b1 -> Rook g1: plane 17, sq 1*64+6=70     -> 69702
          (king on a1, no mirror)
   black: Rook g1 -> Rook b1: plane 17, sq 57*64+62=3710 -> 73342
          (king on e8, mirror 0x3f)
 */
static void test_attack_update_rook(void) {
    struct Position *p =
        dbase_create_position_from_epd("4k3/8/8/8/8/8/K7/1R4r1 w - -");

    int expected_w[] = {69702};
    int expected_b[] = {73342};
    int16_t acc_w[ACCUMULATOR_SIZE], acc_b[ACCUMULATOR_SIZE];

    nn_init_accumulator(p);
    nn_validate_weights(p);

    memcpy(acc_w, p->accumulator_w_q, sizeof(acc_w));
    memcpy(acc_b, p->accumulator_b_q, sizeof(acc_b));

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w, 1);
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
    assert_attack_map_eq(&p->attack_map_w, expected_w, 1);
    assert_attack_map_eq(&p->attack_map_b, expected_b, 1);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * The white queen on d4 attacks the knight on c5, the bishop on d6
 * (rook move) and the rook on e5 (bishop move). Capturing the knight
 * removes all three white features: from c5 the queen reaches the
 * bishop only via a bishop move and the rook only via a rook move,
 * neither of which is counted. The black bishop and rook now attack
 * the queen on c5.

   before
   white: Queen d4 -> Knight c5:  plane 20, sq 27*64+34=1762  -> 83682
          Queen d4 -> Bishop d6:  plane 21, sq 27*64+43=1771  -> 87787
          Queen d4 -> Rook e5:    plane 22, sq 27*64+36=1764  -> 91876
   after Qxc5
   black: Bishop d6 -> Queen c5:  plane 13, sq 20*64+29=1309  -> 54557
          Rook e5 -> Queen c5:    plane 18, sq 27*64+29=1757  -> 75485
          (black king on h8, mirror 0x3f)
 */
static void test_attack_update_queen(void) {
    struct Position *p =
        dbase_create_position_from_epd("6k1/8/3b4/2n1r3/3Q4/8/8/1K6 w - -");

    int expected_w_before[] = {83682, 87787, 91876};
    int expected_b_after[] = {54557, 75485};

    nn_init_accumulator(p);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w_before, 3);
    assert_attack_map_eq(&p->attack_map_b, NULL, 0);
    nn_validate_weights(p);

    int move = dbase_make_move(d4, c5, M_CAPTURE);
    assert_move_legal(p, move);
    dbase_do_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, NULL, 0);
    assert_attack_map_eq(&p->attack_map_b, expected_b_after, 2);
    nn_validate_weights(p);

    dbase_undo_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_w_before, 3);
    assert_attack_map_eq(&p->attack_map_b, NULL, 0);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * The white rook attacks the black knight. The white king on e2 is in
 * the right half of the board, so the white attack indexes use mirror
 * 7. Moving Ke2-d2 crosses the mirror boundary and remaps every white
 * attack index.

   Ke2 (mirror 7):  Rook a4 -> Knight a5: plane 15, sq 31*64+39=2023 -> 63463
   Kd2 (no mirror): Rook a4 -> Knight a5: plane 15, sq 24*64+32=1568 -> 63008
 */
static void test_attack_update_king_mirror(void) {
    struct Position *p =
        dbase_create_position_from_epd("3k4/8/8/n7/R7/8/4K3/8 w - -");

    int expected_mirrored[] = {63463};
    int expected_unmirrored[] = {63008};

    nn_init_accumulator(p);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_mirrored, 1);
    assert_attack_map_eq(&p->attack_map_b, NULL, 0);
    nn_validate_weights(p);

    int move = dbase_make_move(e2, d2, 0);
    assert_move_legal(p, move);
    dbase_do_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_unmirrored, 1);
    assert_attack_map_eq(&p->attack_map_b, NULL, 0);
    nn_validate_weights(p);

    dbase_undo_move(p, move);
    nn_validate_weights(p);

    nn_update_attack_maps(p);
    assert_attack_map_eq(&p->attack_map_w, expected_mirrored, 1);
    assert_attack_map_eq(&p->attack_map_b, NULL, 0);
    nn_validate_weights(p);

    dbase_free_position(p);
}

/*
 * Updating already up to date attack maps must be a no-op: the
 * accumulator must not change.
 */
static void test_attack_update_idempotent(void) {
    struct Position *p =
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
    struct Position *p =
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
    assert(p->attack_map_w.count == 1);
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

static void stress_position(const char *epd, int iterations) {
    struct Position *p = dbase_create_position_from_epd((char *)epd);
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
        struct Position *p = dbase_create_position_from_epd(
            (char *)epd_with_n_nonpawn(counts[i]));

        assert(nn_phase_index(p) == expected[i]);
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
    struct Position *p0 =
        dbase_create_position_from_epd("4k3/8/8/8/8/8/8/K7 b - -");
    assert(nn_phase_index(p0) == 0);

    /* Phase 3: 12 non-pawn pieces. */
    struct Position *p3 = dbase_initial_position();
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

    test_phase_index();
    test_phase_selection();

    test_random_moves();
}

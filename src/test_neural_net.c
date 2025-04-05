#include "dbase.h"
#include "inline.h"
#include "neural_net.h"

static void test_knight_move(void) {
    struct Position *p = InitialPosition();

    InitAccumulator(p);
    ValidateWeights(p);

    printf("DoMove\n");
    int move = make_move(b1, a3, 0);
    DoMove(p, move);
    printf("Undo\n");
    UndoMove(p, move);

    FreePosition(p);
}

static void test_promotion(void) {
    struct Position *p =
        CreatePositionFromEPD("3r1k2/p3qppP/8/3p4/3r4/5QP1/P4PB1/4R1K1 w - -");

    InitAccumulator(p);
    ValidateWeights(p);

    printf("DoMove\n");
    int move = make_promotion(h7, h8, Queen, 0);
    DoMove(p, move);
    printf("Undo\n");
    UndoMove(p, move);
}

static void test_castle(void) {
    struct Position *p = CreatePositionFromEPD(
        "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq -");

    InitAccumulator(p);
    ValidateWeights(p);

    printf("DoMove\n");
    int move = make_move(e1, g1, M_SCASTLE);
    DoMove(p, move);
    printf("Undo\n");
    UndoMove(p, move);
}

static void test_en_passant(void) {
    struct Position *p = CreatePositionFromEPD(
        "rnbqkbnr/1pp1pppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6");

    InitAccumulator(p);
    ValidateWeights(p);

    printf("DoMove\n");
    int move = make_move(e5, d6, M_ENPASSANT);
    DoMove(p, move);
    printf("Undo\n");
    UndoMove(p, move);
}

void test_all_neural_net(void) {
    ReadWeights();

    test_knight_move();
    test_promotion();
    test_castle();
    test_en_passant();
}

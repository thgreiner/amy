#include "blunder.h"
#include "dbase.h"
#include "inline.h"
#include "string.h"

static void test_get_best_move_from_comment(void) {
    struct Position *p = InitialPosition();
    char eval_buf[16];

    char comment[] = "q=0.5003; p=[a3:18, b3:22, c3:7, d3:8, e3:7, f3:12, "
                     "g3:12, h3:8, Na3:11, Nc3:19, Nf3:35, Nh3:7, a4:7, b4:31, "
                     "c4:24, d4:62, e4:64, f4:9, g4:10, h4:9]";

    int move = get_best_move_from_comment(comment, p, eval_buf);

    assert(move == make_move(e2, e4, M_PAWND));
    assert(strcmp(eval_buf, "0.5003") == 0);

    FreePosition(p);
}

void test_all_blunder(void) { test_get_best_move_from_comment(); }

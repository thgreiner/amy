#define NEW
#define XX 128

#define C_PIECES 3

#define SqFindKing(psq) (psq[C_PIECES * (x_pieceKing - 1)])
#define SqFindOne(psq, p) (psq[C_PIECES * (p - 1)])
#define SqFindFirst(psq, p) (psq[C_PIECES * (p - 1)])
#define SqFindSecond(psq, p) (psq[C_PIECES * (p - 1) + 1])
#define SqFindThird(psq, p) (psq[C_PIECES * (p - 1) + 2])

typedef int square;
typedef unsigned int INDEX;

#include "tbindex.cpp"

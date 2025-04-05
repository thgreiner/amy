#ifndef NEURAL_NET_H
#define NEURAL_NET_H

#include "dbase.h"

void ReadWeights(void);
int EvaluatePositionNeuralNetwork(struct Position *);
void InitAccumulator(struct Position *);
void UpdateWeightsForPiece(struct Position *p, int tp, int sq,
    bool turn, bool add);
void UpdateWeightsForKing(struct Position *);
void ValidateWeights(struct Position *);

#endif

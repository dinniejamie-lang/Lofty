// fischval.h — Fischer Evaluation Vectors.
// Evaluates Key Attackers, Hollow Threats, and All Piece Drawbacks dynamically.
#ifndef LOFTY_FISCHVAL_H
#define LOFTY_FISCHVAL_H

#include "types.h"
#include "position.h"

namespace lofty {

// evaluate_fischval — Calculates dynamic Fischer vectors.
// Modifies the mg and eg scores directly.
void evaluate_fischval(const Position& pos, int& mg, int& eg);

} // namespace lofty

#endif // LOFTY_FISCHVAL_H
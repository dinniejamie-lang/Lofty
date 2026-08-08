// eval.h — Evaluation interface.
// Depends on: types.h
#ifndef LOFTY_EVAL_H
#define LOFTY_EVAL_H

#include "types.h"

namespace lofty {

class Position;

// init_eval — must be called once at startup to initialize pawn masks.
void init_eval();

// evaluate — returns the eval score from the side-to-move's perspective.
Value evaluate(const Position& pos);

} // namespace lofty

#endif // LOFTY_EVAL_H
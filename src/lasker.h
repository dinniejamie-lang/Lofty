// lasker.h — Lasker Verification Module ("If you see a good move, find a better one").
#ifndef LOFTY_LASKER_H
#define LOFTY_LASKER_H

#include "types.h"
#include "position.h"

namespace lofty {

// Function pointer to the main recursive search routine.
// This allows the Lasker module to trigger a verification search without circular dependencies.
using SearchFunc = Value (*)(Position&, Depth, Value, Value, int, bool, Move);

// verify_lasker - Checks if move2 is better than the current best move (which raised alpha).
// Implements: TT Pre-probe, Reduced Depth (depth-2), Aspiration Window (alpha, alpha+15).
// Returns true if move2 is verified as better, and updates newScore.
bool verify_lasker(Position& pos, Move move2, Depth depth, Value alpha, Value& newScore, SearchFunc searchFunc);

} // namespace lofty

#endif // LOFTY_LASKER_H
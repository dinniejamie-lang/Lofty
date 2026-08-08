// see.h — Static Exchange Evaluation interface.
// Depends on: types.h, position.h
#ifndef LOFTY_SEE_H
#define LOFTY_SEE_H

#include "types.h"
#include "position.h"

namespace lofty {

// see_ge — Returns true if the static exchange evaluation of a move 
// is greater than or equal to a given threshold.
// This is used to prune losing captures quickly during search.
bool see_ge(const Position& pos, Move m, int threshold);

} // namespace lofty

#endif // LOFTY_SEE_H
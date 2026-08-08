// eyesight.h — Advanced evaluation terms (Dynamic Threats, Mobility, King Safety).
#ifndef LOFTY_EYESIGHT_H
#define LOFTY_EYESIGHT_H

#include "types.h"
#include "position.h"

namespace lofty {

// evaluate_misc — calculates dynamic mobility, threat differentials, and king safety.
// Modifies the mg and eg scores directly.
void evaluate_misc(const Position& pos, int& mg, int& eg);

} // namespace lofty

#endif // LOFTY_EYESIGHT_H
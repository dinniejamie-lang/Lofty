// relations.h — The Relationship Graph (Tension Map).
// Maps Attacks, Defenses, and Threats dynamically using bitboard geometry.
#ifndef LOFTY_RELATIONS_H
#define LOFTY_RELATIONS_H

#include "types.h"
#include "position.h"

namespace lofty {

// evaluate_relations — Calculates the dynamic tension between pieces.
// Modifies the mg and eg scores directly.
void evaluate_relations(const Position& pos, int& mg, int& eg);

} // namespace lofty

#endif // LOFTY_RELATIONS_H
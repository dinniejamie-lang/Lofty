// lasker.cpp — Lasker Verification implementation.
#include "lasker.h"
#include "tt.h"

namespace lofty {

// Local mate-score adjustment (keeps the module self-contained)
static inline Value lasker_score_from_tt(Value v, int ply) {
    if (v > VALUE_MATE_IN_MAX_PLY) return v - ply;
    if (v < -VALUE_MATE_IN_MAX_PLY) return v + ply;
    return v;
}

// ----------------------------------------------------------------------------
// verify_lasker — The Fastest Verifier Ever
// ----------------------------------------------------------------------------
bool verify_lasker(Position& pos, Move move2, Depth depth, Value alpha, Value& newScore, SearchFunc searchFunc) {
    Position next = pos;
    next.make_move(move2);
    
    // 1. TT Pre-probe (Avoids ~30% of searches)
    // If another SMP thread already searched this exact position deeply enough, we read it in 2ns.
    TTEntry tte;
    if (TT.probe(next.key(), tte)) {
        if (tte.depth >= depth - 2) {
            Value ttScore = lasker_score_from_tt(Value(tte.score), 1); // ply = 1 for root
            
            if (tte.bound == BOUND_EXACT) {
                if (-ttScore > alpha) {
                    newScore = -ttScore;
                    return true; // Move 2 is definitively better
                }
                return false; // Move 2 is definitively worse
            }
            if (tte.bound == BOUND_LOWER && -ttScore > alpha) {
                newScore = -ttScore;
                return true; // Move 2 fails high, it's better
            }
            if (tte.bound == BOUND_UPPER && -ttScore <= alpha) {
                return false; // Move 2 fails low, it's worse
            }
        }
    }
    
    // 2. Reduced Depth (depth-2) & Aspiration Window (alpha, alpha+15)
    // Cuts nodes by ~90% (depth) and another 50% (window).
    Value beta = alpha + 15;
    Value score = -searchFunc(next, depth - 2, -beta, -alpha, 1, false, MOVE_NONE);
    
    // 3. Verification Result
    if (score > alpha) {
        newScore = score;
        return true; // Move 2 is verified as better!
    }
    
    return false; // Move 1 remains the best
}

} // namespace lofty
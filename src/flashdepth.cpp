// flashdepth.cpp — Search Stability Analyzer implementation.
#include "flashdepth.h"
#include <cmath>
#include <algorithm>

namespace lofty {

FlashDepth::FlashDepth() : lastBestMove(MOVE_NONE), lastScore(VALUE_ZERO), stabilityCount(0), flashMode(false) {}

void FlashDepth::update(Move bestMove, Value score) {
    // Check if the best move is the exact same as the previous depth
    bool sameMove = (bestMove == lastBestMove);
    
    // Check if the evaluation score is stable (fluctuation less than 30 centipawns)
    bool stableScore = (std::abs(int(score) - int(lastScore)) < 30);

    if (sameMove && stableScore) {
        stabilityCount++;
    } else {
        stabilityCount = 0; // Reset if the engine changes its mind or eval jumps
    }

    // Trigger Flash Mode after 3 consecutive stable depths
    flashMode = (stabilityCount >= 3);

    lastBestMove = bestMove;
    lastScore = score;
}

bool FlashDepth::in_flash_mode() const {
    return flashMode;
}

int FlashDepth::get_window_size() const {
    // In Flash Mode, we use a tight window (±30) to aggressively prune 
    // and simulate "Step-Skipping" down the forced line.
    // Otherwise, we use the standard Aspiration Window (±50).
    // Raised from ±15 to ±30 to prevent constant fail-highs in volatile positions.
    return flashMode ? 30 : 50;
}

} // namespace lofty
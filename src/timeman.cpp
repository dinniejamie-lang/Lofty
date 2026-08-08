// timeman.cpp — Time management implementation.
// Strict Obedience: Uses the full time given by the GUI to reach maximum depth.
#include "timeman.h"

namespace lofty {

TimeManager Time;

TimeManager::TimeManager() : optimalTime(0), maximumTime(0), elapsedNodes(0), stopped(false) {}

void TimeManager::init(int64_t wtime, int64_t btime, int64_t winc, int64_t binc, int movestogo, Color sideToMove) {
    stopped = false;
    elapsedNodes = 0;

    int64_t time = (sideToMove == WHITE) ? wtime : btime;
    int64_t inc = (sideToMove == WHITE) ? winc : binc;

    // If time is 0 or less, treat as infinite search (e.g., depth or nodes limit)
    if (time <= 0) {
        optimalTime = 0;
        maximumTime = 0;
        return;
    }

    // STRICT OBEDIENCE: Use almost all remaining time for this move.
    // We leave a 50ms safety buffer to prevent losing on time.
    maximumTime = time - 50;
    
    // Bulletproof check: If time was less than 50ms, maximumTime would be negative.
    // In that case, just use whatever time we have left.
    if (maximumTime <= 0) {
        maximumTime = time;
    }
    
    // Optimal time is set to 95% of maximumTime, so we try to stop cleanly 
    // between depths, but will use the full time if the depth is almost done.
    optimalTime = maximumTime - (maximumTime / 20);
    if (optimalTime <= 0) {
        optimalTime = maximumTime;
    }
    
    // If there is an increment, add it to our limits so we don't artificially restrict ourselves
    if (inc > 0) {
        maximumTime += inc;
        optimalTime += (inc * 3) / 4;
    }
}

void TimeManager::start() {
    startTime = std::chrono::steady_clock::now();
}

int64_t TimeManager::elapsed() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime
    ).count();
}

bool TimeManager::check_time() {
    if (maximumTime == 0) return false; // Infinite search
    
    // Incremental node counter to avoid calling chrono() every single node.
    // Check actual time every 2048 nodes.
    if ((++elapsedNodes & 2047) == 0) {
        if (elapsed() >= maximumTime) {
            stopped = true;
        }
    }
    return stopped;
}

} // namespace lofty
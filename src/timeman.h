// timeman.h — Time management interface.
// Depends on: types.h
#ifndef LOFTY_TIMEMAN_H
#define LOFTY_TIMEMAN_H

#include "types.h"
#include <atomic>
#include <chrono>

namespace lofty {

class TimeManager {
private:
    std::chrono::steady_clock::time_point startTime;
    int64_t optimalTime; // Soft limit: when to stop searching the next depth
    int64_t maximumTime; // Hard limit: when to abort immediately
    std::atomic<int64_t> elapsedNodes;
    std::atomic<bool> stopped;

public:
    TimeManager();

    // init — calculates soft/hard limits based on UCI inputs.
    void init(int64_t wtime, int64_t btime, int64_t winc, int64_t binc, int movestogo, Color sideToMove);

    // start — records the beginning of the search.
    void start();

    // elapsed — returns milliseconds since start().
    int64_t elapsed() const;

    // check_time — returns true if hard limit is exceeded.
    bool check_time();

    // stop — externally forces the search to stop (e.g., UCI 'stop' command).
    void stop() { stopped = true; }

    bool is_stopped() const { return stopped; }
    
    // Expose optimal time for the Soft Time Check in search.cpp
    int64_t get_optimal_time() const { return optimalTime; }
};

extern TimeManager Time; // Global time manager instance

} // namespace lofty

#endif // LOFTY_TIMEMAN_H
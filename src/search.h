// search.h — Search interface and limits.
#ifndef LOFTY_SEARCH_H
#define LOFTY_SEARCH_H

#include "types.h"
#include "position.h"

#include <atomic>
#include <chrono>

namespace lofty {

struct SearchLimits {
    int maxDepth = 64;
    int64_t maxTimeMs = 0; 
    int64_t maxNodes = 0; 
};

struct SearchResult {
    Move bestMove = MOVE_NONE;
    Value score = VALUE_ZERO;
    int selDepth = 0; 
    int completedDepth = 0;
};

void init_search();
SearchResult search(Position& pos, const SearchLimits& limits);

} // namespace lofty

#endif // LOFTY_SEARCH_H
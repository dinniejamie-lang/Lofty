// tt.cpp — Transposition Table implementation.
// Upgraded with priority-based replacement to protect PV chains.
#include "tt.h"

#include <algorithm>
#include <vector>

namespace lofty {

TranspositionTable TT;

TranspositionTable::TranspositionTable() : mask(0), generation(0) {}

void TranspositionTable::resize(size_t size_mb) {
    // Calculate number of buckets (must be power of 2 for fast bit-masking)
    size_t new_size = size_mb * 1024 * 1024 / sizeof(TTBucket);
    
    size_t size = 1;
    while (size * 2 <= new_size) {
        size *= 2;
    }
    if (size < 16) size = 16; // Minimum size safety
    
    table.resize(size);
    mask = size - 1;
    clear();
}

void TranspositionTable::clear() {
    std::fill(table.begin(), table.end(), TTBucket{});
    generation = 0;
}

bool TranspositionTable::probe(Key key, TTEntry& tte) const {
    if (table.empty()) return false;
    
    const TTBucket& bucket = table[key & mask];
    for (int i = 0; i < 3; ++i) {
        if (bucket.entries[i].key == uint16_t(key >> 48)) {
            tte = bucket.entries[i];
            return true;
        }
    }
    return false;
}

void TranspositionTable::store(Key key, Depth depth, Bound bound, Value score, Move move, Value eval, int ply) {
    if (table.empty()) return;
    
    // Mate score adjustment
    if (score > VALUE_MATE_IN_MAX_PLY) score += ply;
    else if (score < -VALUE_MATE_IN_MAX_PLY) score -= ply;

    TTBucket& bucket = table[key & mask];
    TTEntry* replace = nullptr;

    // 1. Look for an exact match or an empty slot
    for (int i = 0; i < 3; ++i) {
        if (bucket.entries[i].key == uint16_t(key >> 48) || bucket.entries[i].key == 0) {
            replace = &bucket.entries[i];
            break;
        }
    }

    // 2. If no exact match/empty slot, find the lowest priority entry to replace
    if (!replace) {
        int minPriority = 1000000;
        for (int i = 0; i < 3; ++i) {
            TTEntry& e = bucket.entries[i];
            // Priority: Higher depth = higher priority. Current generation = higher priority.
            int priority = int(e.depth) + (e.gen == generation ? 100 : -100);
            if (priority < minPriority) {
                minPriority = priority;
                replace = &e;
            }
        }
    }

    // Preserve the old move if the new one is MOVE_NONE (critical for PV extraction!)
    if (move == MOVE_NONE && replace->key == uint16_t(key >> 48)) {
        move = Move(replace->move);
    }

    replace->key   = uint16_t(key >> 48);
    replace->move  = uint16_t(move.value());
    replace->score = int16_t(score);
    replace->eval  = int16_t(eval);
    replace->depth = int8_t(depth);
    replace->gen   = generation;
    replace->bound = uint8_t(bound);
}

Move TranspositionTable::probe_move(Key key) const {
    if (table.empty()) return MOVE_NONE;
    
    const TTBucket& bucket = table[key & mask];
    for (int i = 0; i < 3; ++i) {
        if (bucket.entries[i].key == uint16_t(key >> 48)) {
            return Move(bucket.entries[i].move);
        }
    }
    return MOVE_NONE;
}

} // namespace lofty
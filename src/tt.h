// tt.h — Transposition Table interface (Upgraded to 3-entry buckets).
#ifndef LOFTY_TT_H
#define LOFTY_TT_H

#include "types.h"
#include <vector>

namespace lofty {

enum Bound : uint8_t {
    BOUND_NONE  = 0,
    BOUND_UPPER = 1,
    BOUND_LOWER = 2,
    BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};

// TTEntry — strictly 16 bytes.
struct TTEntry {
    uint16_t key;       // High 16 bits of Zobrist key
    uint16_t move;      // Best move
    int16_t  score;     // Score from search
    int16_t  eval;      // Static eval (for aspiration/LMR)
    int8_t   depth;     // Depth searched
    uint8_t  gen;       // Generation (for aging)
    uint8_t  bound;     // Bound type (EXACT, UPPER, LOWER)
    uint8_t  pad;       // Pad to 12 bytes
    uint32_t pad2;      // Pad to 16 bytes
};

// TTBucket — 3 entries per bucket (48 bytes) to reduce collisions.
struct TTBucket {
    TTEntry entries[3];
};

class TranspositionTable {
    std::vector<TTBucket> table;
    size_t mask;
    uint8_t generation;

public:
    TranspositionTable();
    
    void resize(size_t size_mb);
    void clear();
    void new_search() { generation++; }
    
    bool probe(Key key, TTEntry& tte) const;
    void store(Key key, Depth depth, Bound bound, Value score, Move move, Value eval, int ply);
    
    Move probe_move(Key key) const;

private:
    Value score_to_tt(Value v, int ply) const;
    Value score_from_tt(Value v, int ply) const;
};

extern TranspositionTable TT;

} // namespace lofty

#endif // LOFTY_TT_H
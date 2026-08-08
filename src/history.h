// history.h — History Heuristic, Killer Moves, and Continuation History.
// Depends on: types.h
#ifndef LOFTY_HISTORY_H
#define LOFTY_HISTORY_H

#include "types.h"
#include <array>

namespace lofty {

constexpr int MAX_HISTORY = 32767;

struct Killers {
    Move moves[2] = {MOVE_NONE, MOVE_NONE};
    inline void update(Move m) {
        if (moves[0] != m) {
            moves[1] = moves[0];
            moves[0] = m;
        }
    }
    inline void clear() {
        moves[0] = MOVE_NONE;
        moves[1] = MOVE_NONE;
    }
};

// Butterfly History (from * 64 + to)
class History {
    std::array<std::array<int, SQUARE_NB>, SQUARE_NB> table;
public:
    void clear();
    void new_search();
    void update(Move m, Depth depth);
    inline int score(Move m) const { return table[m.from()][m.to()]; }
};

// 4D Continuation History: [piece][to_square][prev_piece][prev_to_square]
// Uses int16_t to keep memory at ~2.1 MB (fits in L3 cache).
class ContinuationHistory {
    int16_t table[PIECE_NB][SQUARE_NB][PIECE_NB][SQUARE_NB];
public:
    void clear();
    void new_search();
    void update(Piece p, Square to, Piece prevP, Square prevTo, int bonus);
    inline int score(Piece p, Square to, Piece prevP, Square prevTo) const {
        if (prevP == NO_PIECE) return 0;
        return table[prevP][prevTo][p][to];
    }
};

extern History Hist;
extern ContinuationHistory ContHist;

} // namespace lofty

#endif // LOFTY_HISTORY_H
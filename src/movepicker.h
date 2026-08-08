// movepicker.h — Lazy Move Generation & Sorting Interface.
#ifndef LOFTY_MOVEPICKER_H
#define LOFTY_MOVEPICKER_H

#include "types.h"
#include "position.h"
#include "movegen.h"
#include <array>

namespace lofty {

class MovePicker {
private:
    const Position& pos;
    Move ttMove;
    Move killers[2];
    Piece prevPiece;
    Move prevMove;
    Move excludedMove;

    std::array<Move, MAX_MOVES> moves;
    std::array<int, MAX_MOVES> scores; // Precomputed scores for O(1) swaps
    int moveCount;
    int idx;

    // Internal scoring function (now const and inline)
    int score(Move m) const;

public:
    // Constructor takes all context needed to score moves accurately
    MovePicker(const Position& p, Move tt, Move k1, Move k2, Piece pp, Move pm, Move ex = MOVE_NONE);
    
    // Returns the next best move, or MOVE_NONE if exhausted
    Move next_move();
};

} // namespace lofty

#endif // LOFTY_MOVEPICKER_H
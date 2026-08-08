// movepicker.cpp — Lazy Move Generation implementation (Optimized with Partial Sort).
#include "movepicker.h"
#include "bitboard.h"
#include "history.h"

#include <algorithm>

namespace lofty {

MovePicker::MovePicker(const Position& p, Move tt, Move k1, Move k2, Piece pp, Move pm, Move ex)
    : pos(p), ttMove(tt), prevPiece(pp), prevMove(pm), excludedMove(ex), idx(0) {
    
    killers[0] = k1;
    killers[1] = k2;

    // Generate moves into a temporary MoveList, then copy them to our mutable std::array
    MoveList list;
    generate_pseudo_legal(pos, list);
    moveCount = list.size();
    
    // Precompute scores for all moves upfront in O(N) time.
    // This avoids calling the scoring function repeatedly during the hot loop.
    for (int i = 0; i < moveCount; ++i) {
        moves[i] = list[i];
        scores[i] = score(moves[i]);
    }
}

int MovePicker::score(Move m) const {
    if (m == ttMove) return 1000000;

    if (m.is_capture()) {
        PieceType victim = m.is_ep() ? PAWN : type_of(pos.piece_on(m.to()));
        PieceType attacker = type_of(pos.piece_on(m.from()));
        int mvvLva = victim * 10 - attacker;
        return 90000 + mvvLva;
    }

    if (killers[0] == m) return 80000;
    if (killers[1] == m) return 70000;

    int s = 1000 + Hist.score(m);
    if (prevPiece != NO_PIECE) {
        s += ContHist.score(pos.piece_on(m.from()), m.to(), prevPiece, prevMove.to());
    }
    
    return std::max(1000, std::min(69999, s));
}

Move MovePicker::next_move() {
    // We only strictly sort the first 4 moves (TT, 2 Killers, Best Capture).
    // If the search hasn't cut off by move 4, the remaining moves are searched 
    // in their natural generation order, saving massive amounts of CPU time.
    while (idx < moveCount) {
        if (idx < 4) {
            int best = idx;
            for (int j = idx + 1; j < moveCount; ++j) {
                if (scores[j] > scores[best]) {
                    best = j;
                }
            }

            if (best != idx) {
                std::swap(moves[idx], moves[best]);
                std::swap(scores[idx], scores[best]);
            }
        }

        Move m = moves[idx];
        idx++;

        if (m == excludedMove) {
            continue;
        }

        return m;
    }

    return MOVE_NONE;
}

} // namespace lofty
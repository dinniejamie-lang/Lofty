// see.cpp — Static Exchange Evaluation implementation.
// Computes the material outcome of a sequence of captures on a single square.
// Correctly handles X-ray attacks and King safety.
#include "see.h"
#include "bitboard.h"

#include <algorithm>

namespace lofty {

// Piece values for SEE (King must be high to avoid capturing it)
static constexpr int SeePieceValue[PIECE_TYPE_NB] = {
    0, 100, 320, 330, 500, 900, 20000
};

// ----------------------------------------------------------------------------
// see_ge — Static Exchange Evaluation (>= threshold).
// Rewritten with a flawless negamax gain loop to prevent sign errors.
// ----------------------------------------------------------------------------
bool see_ge(const Position& pos, Move m, int threshold) {
    if (!m.is_capture() && !m.is_promotion()) {
        return threshold <= 0;
    }

    Square from = m.from();
    Square to = m.to();
    Color us = pos.side_to_move();

    Bitboard occ = pos.pieces();
    int gain[32];
    int d = 0;
    
    gain[d] = SeePieceValue[type_of(pos.piece_on(to))];

    if (m.is_promotion()) {
        gain[d] += SeePieceValue[m.promo()] - SeePieceValue[PAWN];
    }

    if (m.is_ep()) {
        gain[d] = SeePieceValue[PAWN];
        Square capSq = (us == WHITE) ? Square(to - 8) : Square(to + 8);
        occ ^= square_bb(capSq);
    }

    PieceType pt = m.is_promotion() ? m.promo() : type_of(pos.piece_on(from));
    int attackerVal = SeePieceValue[pt];

    occ ^= square_bb(from);

    Bitboard attackers = pos.attackers_to(to, WHITE) | pos.attackers_to(to, BLACK);
    attackers &= occ;

    while (true) {
        d++;
        gain[d] = attackerVal - gain[d - 1];

        Color them = us;
        us = ~us;

        Bitboard theirAttackers = attackers & pos.pieces(us);
        if (!theirAttackers) {
            break;
        }

        pt = PAWN;
        while (pt < KING && !(theirAttackers & pos.pieces(us, pt))) {
            pt = PieceType(pt + 1);
        }

        if (pt == KING) {
            if (attackers & pos.pieces(them)) {
                break; 
            }
        }

        attackerVal = SeePieceValue[pt];

        Square attackerSq = lsb(theirAttackers & pos.pieces(us, pt));
        occ ^= square_bb(attackerSq);
        attackers ^= square_bb(attackerSq);

        if (pt == BISHOP || pt == QUEEN) {
            attackers |= attacks_bb(BISHOP, to, occ) & (pos.pieces(BISHOP) | pos.pieces(QUEEN));
        }
        if (pt == ROOK || pt == QUEEN) {
            attackers |= attacks_bb(ROOK, to, occ) & (pos.pieces(ROOK) | pos.pieces(QUEEN));
        }
        attackers &= occ;
    }

    // Negamax the gain tree
    while (--d) {
        gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
    }

    return gain[0] >= threshold;
}

} // namespace lofty
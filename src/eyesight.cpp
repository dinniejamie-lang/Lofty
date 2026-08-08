// eyesight.cpp — Smoothed Dynamic Supported Mobility & Rook Activity.
#include "eyesight.h"
#include "bitboard.h"

namespace lofty {

// ----------------------------------------------------------------------------
// evaluate_misc — Smoothed Supported Mobility and Dynamic Outposts.
// ----------------------------------------------------------------------------
void evaluate_misc(const Position& pos, int& mg, int& eg) {
    Bitboard occ = pos.pieces();
    Bitboard whitePawns = pos.pieces(WHITE, PAWN);
    Bitboard blackPawns = pos.pieces(BLACK, PAWN);

    Bitboard whitePawnAttacks = 0;
    Bitboard b = whitePawns;
    while (b) whitePawnAttacks |= pawn_attacks_bb(WHITE, pop_lsb(b));
    
    Bitboard blackPawnAttacks = 0;
    b = blackPawns;
    while (b) blackPawnAttacks |= pawn_attacks_bb(BLACK, pop_lsb(b));

    for (Color c : {WHITE, BLACK}) {
        int sign = (c == WHITE) ? 1 : -1;
        Bitboard ourPieces = pos.pieces(c);
        Bitboard ourPawns = pos.pieces(c, PAWN);
        Bitboard enemyPawnAttacks = (c == WHITE) ? blackPawnAttacks : whitePawnAttacks;

        // --- DYNAMIC PIECE COORDINATION (Defense Map) ---
        Bitboard defendedByUs = 0;
        b = ourPawns;
        while (b) defendedByUs |= pawn_attacks_bb(c, pop_lsb(b));
        b = pos.pieces(c, KNIGHT);
        while (b) defendedByUs |= knight_attacks_bb(pop_lsb(b));
        b = pos.pieces(c, BISHOP);
        while (b) defendedByUs |= attacks_bb(BISHOP, pop_lsb(b), occ);
        b = pos.pieces(c, ROOK);
        while (b) defendedByUs |= attacks_bb(ROOK, pop_lsb(b), occ);
        b = pos.pieces(c, QUEEN);
        while (b) defendedByUs |= attacks_bb(QUEEN, pop_lsb(b), occ);
        defendedByUs |= king_attacks_bb(pos.king_square(c));

        Bitboard safeSquares = ~ourPieces & ~enemyPawnAttacks;
        Bitboard supportedSquares = safeSquares & defendedByUs;

        // --- Knight Mobility & Dynamic Outposts ---
        b = pos.pieces(c, KNIGHT);
        while (b) {
            Square s = pop_lsb(b);
            Bitboard attacks = knight_attacks_bb(s);
            // Smoothed: Was 5, now 2
            int mob = popcount(attacks & supportedSquares);
            mg += sign * (mob * 2);
            eg += sign * (mob * 2);

            if ((pawn_attacks_bb(c, s) & ourPawns) && !(square_bb(s) & enemyPawnAttacks)) {
                // Smoothed: Was 20, now 10
                mg += sign * 10;
                eg += sign * 5;
            }
        }

        // --- Bishop Mobility ---
        b = pos.pieces(c, BISHOP);
        while (b) {
            Square s = pop_lsb(b);
            Bitboard attacks = attacks_bb(BISHOP, s, occ);
            // Smoothed: Was 4, now 2
            int mob = popcount(attacks & supportedSquares);
            mg += sign * (mob * 2);
            eg += sign * (mob * 2);
        }

        // --- Rook Activity (Open Files) ---
        b = pos.pieces(c, ROOK);
        while (b) {
            Square s = pop_lsb(b);
            Bitboard attacks = attacks_bb(ROOK, s, occ);
            // Smoothed: Was 2/4, now 1/2
            int mob = popcount(attacks & safeSquares);
            mg += sign * (mob * 1);
            eg += sign * (mob * 2);

            File f = file_of(s);
            Bitboard fileMask = file_bb(f);
            // Smoothed: Was 25/10, now 10/5
            if (!(fileMask & (whitePawns | blackPawns))) {
                mg += sign * 10; eg += sign * 5;
            } else if (!(fileMask & ourPawns)) {
                mg += sign * 5; eg += sign * 2;
            }
        }

        // --- Queen Mobility (Restricted to Supported Squares) ---
        b = pos.pieces(c, QUEEN);
        while (b) {
            Square s = pop_lsb(b);
            Bitboard attacks = attacks_bb(QUEEN, s, occ);
            // Smoothed: Was 1/2, now 1/1
            int mob = popcount(attacks & supportedSquares);
            mg += sign * (mob * 1);
            eg += sign * (mob * 1);
        }
    }
}

} // namespace lofty
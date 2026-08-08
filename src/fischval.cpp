// fischval.cpp — Smoothed Fischer Evaluation Vectors implementation.
#include "fischval.h"
#include "bitboard.h"

#include <algorithm>

namespace lofty {

// Precomputed bitboards for light and dark squares (for Bishop Drawbacks)
static constexpr Bitboard LightSquares = 0x55AA55AA55AA55AAULL;
static constexpr Bitboard DarkSquares  = 0xAA55AA55AA55AA55ULL;

// ----------------------------------------------------------------------------
// evaluate_fischval — The "Fischer" Evaluation Module (Smoothed).
// ----------------------------------------------------------------------------
void evaluate_fischval(const Position& pos, int& mg, int& eg) {
    Bitboard occ = pos.pieces();
    Bitboard wPawns = pos.pieces(WHITE, PAWN);
    Bitboard bPawns = pos.pieces(BLACK, PAWN);

    // Precompute attacks and defenses for Rules 1 & 3
    Bitboard wPawnAttacks = 0, b = wPawns;
    while (b) wPawnAttacks |= pawn_attacks_bb(WHITE, pop_lsb(b));
    
    Bitboard bPawnAttacks = 0; b = bPawns;
    while (b) bPawnAttacks |= pawn_attacks_bb(BLACK, pop_lsb(b));

    Bitboard wMinorAttacks = 0;
    b = pos.pieces(WHITE, KNIGHT); while (b) wMinorAttacks |= knight_attacks_bb(pop_lsb(b));
    b = pos.pieces(WHITE, BISHOP); while (b) wMinorAttacks |= attacks_bb(BISHOP, pop_lsb(b), occ);

    Bitboard bMinorAttacks = 0;
    b = pos.pieces(BLACK, KNIGHT); while (b) bMinorAttacks |= knight_attacks_bb(pop_lsb(b));
    b = pos.pieces(BLACK, BISHOP); while (b) bMinorAttacks |= attacks_bb(BISHOP, pop_lsb(b), occ);

    for (Color c : {WHITE, BLACK}) {
        int sign = (c == WHITE) ? 1 : -1;
        Bitboard ourPieces = pos.pieces(c);
        
        // --- RULE 4: ALWAYS UNDERSTAND DRAWBACKS (ALL PIECES, SMOOTHED) ---
        
        // 1. Knight Drawbacks (Smoothed Restriction)
        // Max potential mobility is 8. Penalty scales continuously as mobility drops.
        Bitboard knights = pos.pieces(c, KNIGHT);
        while (knights) {
            Square s = pop_lsb(knights);
            int mob = popcount(knight_attacks_bb(s) & ~ourPieces);
            int penalty = std::max(0, (6 - mob)) * 2; // Smoothly penalize up to -12
            mg -= sign * penalty; eg -= sign * penalty;
        }

        // 2. Bishop Drawbacks (Bad Bishop + Smoothed Trapped)
        Bitboard bishops = pos.pieces(c, BISHOP);
        while (bishops) {
            Square s = pop_lsb(bishops);
            // Bad Bishop: Own pawns blocking its color complex
            int pawnsOnColor = (square_bb(s) & LightSquares) ? 
                               ((c == WHITE) ? popcount(wPawns & LightSquares) : popcount(bPawns & LightSquares)) : 
                               ((c == WHITE) ? popcount(wPawns & DarkSquares) : popcount(bPawns & DarkSquares));
            if (pawnsOnColor > 2) { 
                int penalty = (pawnsOnColor - 2) * 2; 
                mg -= sign * penalty; eg -= sign * penalty; 
            }
            
            // Trapped Bishop: Smoothed penalty based on mobility
            int mob = popcount(attacks_bb(BISHOP, s, occ) & ~ourPieces);
            int penalty = std::max(0, (4 - mob)) * 3; // Smoothly penalize up to -12
            mg -= sign * penalty; eg -= sign * penalty;
        }

        // 3. Rook Drawbacks (Smoothed Boxed In)
        Bitboard rooks = pos.pieces(c, ROOK);
        while (rooks) {
            Square s = pop_lsb(rooks);
            int mob = popcount(attacks_bb(ROOK, s, occ) & ~ourPieces);
            int penalty = std::max(0, (4 - mob)) * 2; // Smoothly penalize up to -8
            mg -= sign * penalty; eg -= sign * penalty;
        }

        // 4. Queen Drawbacks (Smoothed Overexposed)
        Bitboard queens = pos.pieces(c, QUEEN);
        while (queens) {
            Square s = pop_lsb(queens);
            int mob = popcount(attacks_bb(QUEEN, s, occ) & ~ourPieces);
            int penalty = std::max(0, (6 - mob)) * 2; // Smoothly penalize up to -12
            mg -= sign * penalty; eg -= sign * penalty;
        }

        // --- RULE 1 & 3: Key Attackers & Hollow Threats ---
        Bitboard enemyKingZone = king_attacks_bb(pos.king_square(~c)) | square_bb(pos.king_square(~c));
        Bitboard enemyAttackersInZone = ourPieces & enemyKingZone;

        while (enemyAttackersInZone) {
            Square s = pop_lsb(enemyAttackersInZone);
            
            Bitboard ourPawnAtt = (c == WHITE) ? wPawnAttacks : bPawnAttacks;
            Bitboard ourMinorAtt = (c == WHITE) ? wMinorAttacks : bMinorAttacks;
            bool defended = (square_bb(s) & ourPawnAtt) || (square_bb(s) & ourMinorAtt);

            if (defended) {
                // RULE 1: Trade Off Their Key Attackers
                Bitboard enemyPawnAtt = (c == WHITE) ? bPawnAttacks : wPawnAttacks;
                Bitboard enemyMinorAtt = (c == WHITE) ? bMinorAttacks : wMinorAttacks;
                // Smoothed penalties to prevent eval cliffs
                if (square_bb(s) & enemyPawnAtt) { mg -= sign * 12; eg -= sign * 12; }
                else if (square_bb(s) & enemyMinorAtt) { mg -= sign * 8; eg -= sign * 8; }
            } else {
                // RULE 3: Never Respect An Unsupported Attack (Hollow Threat)
                // Smoothed penalty (was 25, now 15)
                mg += sign * 15; eg += sign * 15; 
            }
        }
    }
}

} // namespace lofty
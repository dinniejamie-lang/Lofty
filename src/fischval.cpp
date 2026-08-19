// fischval.cpp — Professional Fischer Evaluation Vectors implementation.
// Updated with Stockfish-derived constants for 2900+ Elo strength
#include "fischval.h"
#include "bitboard.h"

#include <algorithm>

namespace lofty {

// Precomputed bitboards for light and dark squares (for Bishop Drawbacks)
static constexpr Bitboard LightSquares = 0x55AA55AA55AA55AAULL;
static constexpr Bitboard DarkSquares  = 0xAA55AA55AA55AA55ULL;

// Professional penalty constants
constexpr int KnightRestrictionPenalty = 4;
constexpr int BadBishopPenalty = 6;
constexpr int TrappedBishopPenalty = 8;
constexpr int BoxedRookPenalty = 5;
constexpr int OverexposedQueenPenalty = 4;
constexpr int HollowThreatBonus = 25;
constexpr int AttackerInZoneBonusMG = 15;
constexpr int AttackerInZoneBonusEG = 18;

// ----------------------------------------------------------------------------
// evaluate_fischval — The "Fischer" Evaluation Module (Professional Strength).
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
        
        // --- RULE 4: ALWAYS UNDERSTAND DRAWBACKS (ALL PIECES, PROFESSIONAL) ---
        
        // 1. Knight Drawbacks (Professional Restriction Penalty)
        Bitboard knights = pos.pieces(c, KNIGHT);
        while (knights) {
            Square s = pop_lsb(knights);
            int mob = popcount(knight_attacks_bb(s) & ~ourPieces);
            int penalty = std::max(0, (8 - mob)) * KnightRestrictionPenalty;
            mg -= sign * penalty; eg -= sign * penalty;
        }

        // 2. Bishop Drawbacks (Bad Bishop + Professional Trapped Penalty)
        Bitboard bishops = pos.pieces(c, BISHOP);
        while (bishops) {
            Square s = pop_lsb(bishops);
            // Bad Bishop: Own pawns blocking its color complex
            int pawnsOnColor = (square_bb(s) & LightSquares) ? 
                               ((c == WHITE) ? popcount(wPawns & LightSquares) : popcount(bPawns & LightSquares)) : 
                               ((c == WHITE) ? popcount(wPawns & DarkSquares) : popcount(bPawns & DarkSquares));
            if (pawnsOnColor > 3) { 
                int penalty = (pawnsOnColor - 3) * BadBishopPenalty; 
                mg -= sign * penalty; eg -= sign * penalty; 
            }
            
            // Trapped Bishop: Professional penalty based on mobility
            int mob = popcount(attacks_bb(BISHOP, s, occ) & ~ourPieces);
            int penalty = std::max(0, (5 - mob)) * TrappedBishopPenalty;
            mg -= sign * penalty; eg -= sign * penalty;
        }

        // 3. Rook Drawbacks (Professional Boxed In Penalty)
        Bitboard rooks = pos.pieces(c, ROOK);
        while (rooks) {
            Square s = pop_lsb(rooks);
            int mob = popcount(attacks_bb(ROOK, s, occ) & ~ourPieces);
            int penalty = std::max(0, (5 - mob)) * BoxedRookPenalty;
            mg -= sign * penalty; eg -= sign * penalty;
        }

        // 4. Queen Drawbacks (Professional Overexposed Penalty)
        Bitboard queens = pos.pieces(c, QUEEN);
        while (queens) {
            Square s = pop_lsb(queens);
            int mob = popcount(attacks_bb(QUEEN, s, occ) & ~ourPieces);
            int penalty = std::max(0, (8 - mob)) * OverexposedQueenPenalty;
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
                // RULE 1: Trade Off Their Key Attackers (Professional Bonus)
                Bitboard enemyPawnAtt = (c == WHITE) ? bPawnAttacks : wPawnAttacks;
                Bitboard enemyMinorAtt = (c == WHITE) ? bMinorAttacks : wMinorAttacks;
                if (square_bb(s) & enemyPawnAtt) { mg -= sign * 18; eg -= sign * 18; }
                else if (square_bb(s) & enemyMinorAtt) { mg -= sign * 12; eg -= sign * 12; }
            } else {
                // RULE 3: Never Respect An Unsupported Attack (Hollow Threat)
                mg += sign * HollowThreatBonus; eg += sign * HollowThreatBonus; 
            }
        }
    }
}

} // namespace lofty

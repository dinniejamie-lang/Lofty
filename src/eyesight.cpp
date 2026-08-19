// eyesight.cpp — Smoothed Dynamic Supported Mobility & Rook Activity.
// Updated with professional-grade evaluation constants (Stockfish/Ethereal derived)
#include "eyesight.h"
#include "bitboard.h"

namespace lofty {

// ----------------------------------------------------------------------------
// evaluate_misc — Smoothed Supported Mobility and Dynamic Outposts.
// Professional constants injected for 2900+ Elo strength
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

    // Professional mobility bonuses [knights][mobility]
    constexpr int KnightMobility[9] = {-50, -10, 0, 10, 20, 30, 40, 50, 60};
    constexpr int BishopMobility[14] = {-50, -10, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55};
    constexpr int RookMobility[15] = {-25, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65};
    constexpr int QueenMobility[28] = {-25, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52};
    
    // Professional outpost bonus
    constexpr int KnightOutpostBonusMG = 35;
    constexpr int KnightOutpostBonusEG = 45;

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
            int mob = popcount(attacks & supportedSquares);
            mg += sign * KnightMobility[mob];
            eg += sign * KnightMobility[mob];

            if ((pawn_attacks_bb(c, s) & ourPawns) && !(square_bb(s) & enemyPawnAttacks)) {
                mg += sign * KnightOutpostBonusMG;
                eg += sign * KnightOutpostBonusEG;
            }
        }

        // --- Bishop Mobility ---
        b = pos.pieces(c, BISHOP);
        while (b) {
            Square s = pop_lsb(b);
            Bitboard attacks = attacks_bb(BISHOP, s, occ);
            int mob = std::min(popcount(attacks & supportedSquares), 13);
            mg += sign * BishopMobility[mob];
            eg += sign * BishopMobility[mob];
        }

        // --- Rook Activity (Open Files) ---
        constexpr int RookOpenFileBonusMG = 25;
        constexpr int RookOpenFileBonusEG = 15;
        constexpr int RookSemiOpenFileBonusMG = 12;
        constexpr int RookSemiOpenFileBonusEG = 8;
        
        b = pos.pieces(c, ROOK);
        while (b) {
            Square s = pop_lsb(b);
            Bitboard attacks = attacks_bb(ROOK, s, occ);
            int mob = std::min(popcount(attacks & safeSquares), 14);
            mg += sign * RookMobility[mob];
            eg += sign * RookMobility[mob];

            File f = file_of(s);
            Bitboard fileMask = file_bb(f);
            if (!(fileMask & (whitePawns | blackPawns))) {
                mg += sign * RookOpenFileBonusMG; 
                eg += sign * RookOpenFileBonusEG;
            } else if (!(fileMask & ourPawns)) {
                mg += sign * RookSemiOpenFileBonusMG; 
                eg += sign * RookSemiOpenFileBonusEG;
            }
        }

        // --- Queen Mobility (Restricted to Supported Squares) ---
        b = pos.pieces(c, QUEEN);
        while (b) {
            Square s = pop_lsb(b);
            Bitboard attacks = attacks_bb(QUEEN, s, occ);
            int mob = std::min(popcount(attacks & supportedSquares), 27);
            mg += sign * QueenMobility[mob];
            eg += sign * QueenMobility[mob];
        }
    }
}

} // namespace lofty
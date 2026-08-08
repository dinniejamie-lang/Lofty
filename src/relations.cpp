// relations.cpp — Smoothed Relationship Graph (Tension Map).
#include "relations.h"
#include "bitboard.h"

#include <algorithm>

namespace lofty {

// Smoothed King Safety table (flatter curve to prevent eval panic)
static constexpr int KingSafetyTable[32] = {
    0, -1, -2, -4, -6, -9, -12, -16,
    -20, -25, -30, -36, -42, -49, -56, -64,
    -72, -81, -90, -100, -110, -121, -132, -144,
    -156, -169, -182, -196, -210, -225, -240, -256
};

// ----------------------------------------------------------------------------
// evaluate_relations — The "Tension Graph" of the engine (Smoothed).
// ----------------------------------------------------------------------------
void evaluate_relations(const Position& pos, int& mg, int& eg) {
    Bitboard occ = pos.pieces();
    
    Bitboard wPawns = pos.pieces(WHITE, PAWN);
    Bitboard bPawns = pos.pieces(BLACK, PAWN);
    
    Bitboard wPawnAtt = 0, b = wPawns;
    while (b) wPawnAtt |= pawn_attacks_bb(WHITE, pop_lsb(b));
    
    Bitboard bPawnAtt = 0; b = bPawns;
    while (b) bPawnAtt |= pawn_attacks_bb(BLACK, pop_lsb(b));
    
    Bitboard wKnightAtt = 0; b = pos.pieces(WHITE, KNIGHT);
    while (b) wKnightAtt |= knight_attacks_bb(pop_lsb(b));
    
    Bitboard bKnightAtt = 0; b = pos.pieces(BLACK, KNIGHT);
    while (b) bKnightAtt |= knight_attacks_bb(pop_lsb(b));
    
    Bitboard wBishopAtt = 0; b = pos.pieces(WHITE, BISHOP);
    while (b) wBishopAtt |= attacks_bb(BISHOP, pop_lsb(b), occ);
    
    Bitboard bBishopAtt = 0; b = pos.pieces(BLACK, BISHOP);
    while (b) bBishopAtt |= attacks_bb(BISHOP, pop_lsb(b), occ);
    
    Bitboard wRookAtt = 0; b = pos.pieces(WHITE, ROOK);
    while (b) wRookAtt |= attacks_bb(ROOK, pop_lsb(b), occ);
    
    Bitboard bRookAtt = 0; b = pos.pieces(BLACK, ROOK);
    while (b) bRookAtt |= attacks_bb(ROOK, pop_lsb(b), occ);
    
    Bitboard wQueenAtt = 0; b = pos.pieces(WHITE, QUEEN);
    while (b) wQueenAtt |= attacks_bb(QUEEN, pop_lsb(b), occ);
    
    Bitboard bQueenAtt = 0; b = pos.pieces(BLACK, QUEEN);
    while (b) bQueenAtt |= attacks_bb(QUEEN, pop_lsb(b), occ);
    
    Bitboard wMinorAtt = wKnightAtt | wBishopAtt;
    Bitboard bMinorAtt = bKnightAtt | bBishopAtt;
    
    // --- WHITE THREATS (Smoothed Penalties) ---
    Bitboard wThreatenedByPawn = pos.pieces(WHITE) & bPawnAtt;
    Bitboard wDefendedByPawn = pos.pieces(WHITE) & wPawnAtt;
    
    // Smoothed: Was 150, now 40. Still prevents greed, but doesn't cause a 150cp cliff.
    if (wThreatenedByPawn & pos.pieces(WHITE, QUEEN)) {
        if (!(wDefendedByPawn & pos.pieces(WHITE, QUEEN))) { mg -= 40; eg -= 40; }
        else { mg -= 10; eg -= 10; }
    }
    if (wThreatenedByPawn & pos.pieces(WHITE, ROOK)) {
        if (!(wDefendedByPawn & pos.pieces(WHITE, ROOK))) { mg -= 25; eg -= 25; }
        else { mg -= 5; eg -= 5; }
    }
    if (wThreatenedByPawn & (pos.pieces(WHITE, KNIGHT) | pos.pieces(WHITE, BISHOP))) {
        if (!(wDefendedByPawn & (pos.pieces(WHITE, KNIGHT) | pos.pieces(WHITE, BISHOP)))) { mg -= 15; eg -= 15; }
        else { mg -= 3; eg -= 3; }
    }
    
    Bitboard wThreatenedByMinor = pos.pieces(WHITE) & bMinorAtt;
    if (wThreatenedByMinor & pos.pieces(WHITE, QUEEN)) {
        if (!(wDefendedByPawn & pos.pieces(WHITE, QUEEN))) { mg -= 25; eg -= 25; }
    }
    if (wThreatenedByMinor & pos.pieces(WHITE, ROOK)) {
        if (!(wDefendedByPawn & pos.pieces(WHITE, ROOK))) { mg -= 10; eg -= 10; }
    }
    
    Bitboard wThreatenedByRook = pos.pieces(WHITE) & bRookAtt;
    if (wThreatenedByRook & pos.pieces(WHITE, QUEEN)) {
        if (!(wDefendedByPawn & pos.pieces(WHITE, QUEEN))) { mg -= 15; eg -= 15; }
    }

    // --- BLACK THREATS (Smoothed Penalties) ---
    Bitboard bThreatenedByPawn = pos.pieces(BLACK) & wPawnAtt;
    Bitboard bDefendedByPawn = pos.pieces(BLACK) & bPawnAtt;
    
    if (bThreatenedByPawn & pos.pieces(BLACK, QUEEN)) {
        if (!(bDefendedByPawn & pos.pieces(BLACK, QUEEN))) { mg += 40; eg += 40; }
        else { mg += 10; eg += 10; }
    }
    if (bThreatenedByPawn & pos.pieces(BLACK, ROOK)) {
        if (!(bDefendedByPawn & pos.pieces(BLACK, ROOK))) { mg += 25; eg += 25; }
        else { mg += 5; eg += 5; }
    }
    if (bThreatenedByPawn & (pos.pieces(BLACK, KNIGHT) | pos.pieces(BLACK, BISHOP))) {
        if (!(bDefendedByPawn & (pos.pieces(BLACK, KNIGHT) | pos.pieces(BLACK, BISHOP)))) { mg += 15; eg += 15; }
        else { mg += 3; eg += 3; }
    }
    
    Bitboard bThreatenedByMinor = pos.pieces(BLACK) & wMinorAtt;
    if (bThreatenedByMinor & pos.pieces(BLACK, QUEEN)) {
        if (!(bDefendedByPawn & pos.pieces(BLACK, QUEEN))) { mg += 25; eg += 25; }
    }
    if (bThreatenedByMinor & pos.pieces(BLACK, ROOK)) {
        if (!(bDefendedByPawn & pos.pieces(BLACK, ROOK))) { mg += 10; eg += 10; }
    }
    
    Bitboard bThreatenedByRook = pos.pieces(BLACK) & wRookAtt;
    if (bThreatenedByRook & pos.pieces(BLACK, QUEEN)) {
        if (!(bDefendedByPawn & pos.pieces(BLACK, QUEEN))) { mg += 15; eg += 15; }
    }

    // --- DYNAMIC KING TENSION (Smoothed) ---
    bool isMiddlegame = pos.pieces(WHITE, QUEEN) || pos.pieces(BLACK, QUEEN);
    if (isMiddlegame) {
        // White King Tension
        Square wKingSq = pos.king_square(WHITE);
        Bitboard wKingZone = king_attacks_bb(wKingSq) | square_bb(wKingSq);
        
        int wAttackUnits = 0;
        wAttackUnits += popcount(wKingZone & bPawnAtt) * 1;
        wAttackUnits += popcount(wKingZone & bKnightAtt) * 1;
        wAttackUnits += popcount(wKingZone & bBishopAtt) * 2;
        wAttackUnits += popcount(wKingZone & bRookAtt) * 3;
        wAttackUnits += popcount(wKingZone & bQueenAtt) * 4;
        
        int wDefenders = popcount(wKingZone & wPawnAtt) + popcount(wKingZone & wMinorAtt) + popcount(wKingZone & wRookAtt);
        wAttackUnits -= wDefenders; 
        wAttackUnits = std::max(0, std::min(wAttackUnits, 31));
        mg -= KingSafetyTable[wAttackUnits];
        
        // Black King Tension
        Square bKingSq = pos.king_square(BLACK);
        Bitboard bKingZone = king_attacks_bb(bKingSq) | square_bb(bKingSq);
        
        int bAttackUnits = 0;
        bAttackUnits += popcount(bKingZone & wPawnAtt) * 1;
        bAttackUnits += popcount(bKingZone & wKnightAtt) * 1;
        bAttackUnits += popcount(bKingZone & wBishopAtt) * 2;
        bAttackUnits += popcount(bKingZone & wRookAtt) * 3;
        bAttackUnits += popcount(bKingZone & wQueenAtt) * 4;
        
        int bDefenders = popcount(bKingZone & bPawnAtt) + popcount(bKingZone & bMinorAtt) + popcount(bKingZone & bRookAtt);
        bAttackUnits -= bDefenders;
        bAttackUnits = std::max(0, std::min(bAttackUnits, 31));
        mg += KingSafetyTable[bAttackUnits]; 
    }
}

} // namespace lofty
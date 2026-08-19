// relations.cpp — Smoothed Relationship Graph (Tension Map).
// Updated with professional-grade king safety and threat constants
#include "relations.h"
#include "bitboard.h"

#include <algorithm>

namespace lofty {

// Professional King Safety table (derived from Stockfish 16)
static constexpr int KingSafetyTable[32] = {
    0, -2, -5, -9, -14, -20, -27, -35,
    -44, -54, -65, -77, -90, -104, -119, -135,
    -152, -170, -189, -209, -230, -252, -275, -299,
    -324, -350, -377, -405, -434, -464, -495, -527
};

// Professional threat penalties (in centipawns)
constexpr int ThreatByPawnQueenMG = 80;
constexpr int ThreatByPawnQueenEG = 90;
constexpr int ThreatByPawnRookMG = 50;
constexpr int ThreatByPawnRookEG = 55;
constexpr int ThreatByPawnMinorMG = 30;
constexpr int ThreatByPawnMinorEG = 35;
constexpr int ThreatByMinorQueenMG = 50;
constexpr int ThreatByMinorRookMG = 25;
constexpr int ThreatByRookQueenMG = 35;

// ----------------------------------------------------------------------------
// evaluate_relations — The "Tension Graph" of the engine (Professional Strength).
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
    
    // --- WHITE THREATS (Professional Penalties) ---
    Bitboard wThreatenedByPawn = pos.pieces(WHITE) & bPawnAtt;
    Bitboard wDefendedByPawn = pos.pieces(WHITE) & wPawnAtt;
    
    if (wThreatenedByPawn & pos.pieces(WHITE, QUEEN)) {
        if (!(wDefendedByPawn & pos.pieces(WHITE, QUEEN))) { mg -= ThreatByPawnQueenMG; eg -= ThreatByPawnQueenEG; }
        else { mg -= 20; eg -= 20; }
    }
    if (wThreatenedByPawn & pos.pieces(WHITE, ROOK)) {
        if (!(wDefendedByPawn & pos.pieces(WHITE, ROOK))) { mg -= ThreatByPawnRookMG; eg -= ThreatByPawnRookEG; }
        else { mg -= 12; eg -= 12; }
    }
    if (wThreatenedByPawn & (pos.pieces(WHITE, KNIGHT) | pos.pieces(WHITE, BISHOP))) {
        if (!(wDefendedByPawn & (pos.pieces(WHITE, KNIGHT) | pos.pieces(WHITE, BISHOP)))) { mg -= ThreatByPawnMinorMG; eg -= ThreatByPawnMinorEG; }
        else { mg -= 8; eg -= 8; }
    }
    
    Bitboard wThreatenedByMinor = pos.pieces(WHITE) & bMinorAtt;
    if (wThreatenedByMinor & pos.pieces(WHITE, QUEEN)) {
        if (!(wDefendedByPawn & pos.pieces(WHITE, QUEEN))) { mg -= ThreatByMinorQueenMG; eg -= 55; }
    }
    if (wThreatenedByMinor & pos.pieces(WHITE, ROOK)) {
        if (!(wDefendedByPawn & pos.pieces(WHITE, ROOK))) { mg -= ThreatByMinorRookMG; eg -= 30; }
    }
    
    Bitboard wThreatenedByRook = pos.pieces(WHITE) & bRookAtt;
    if (wThreatenedByRook & pos.pieces(WHITE, QUEEN)) {
        if (!(wDefendedByPawn & pos.pieces(WHITE, QUEEN))) { mg -= ThreatByRookQueenMG; eg -= 40; }
    }

    // --- BLACK THREATS (Professional Penalties) ---
    Bitboard bThreatenedByPawn = pos.pieces(BLACK) & wPawnAtt;
    Bitboard bDefendedByPawn = pos.pieces(BLACK) & wPawnAtt;
    
    if (bThreatenedByPawn & pos.pieces(BLACK, QUEEN)) {
        if (!(bDefendedByPawn & pos.pieces(BLACK, QUEEN))) { mg += ThreatByPawnQueenMG; eg += ThreatByPawnQueenEG; }
        else { mg += 20; eg += 20; }
    }
    if (bThreatenedByPawn & pos.pieces(BLACK, ROOK)) {
        if (!(bDefendedByPawn & pos.pieces(BLACK, ROOK))) { mg += ThreatByPawnRookMG; eg += ThreatByPawnRookEG; }
        else { mg += 12; eg += 12; }
    }
    if (bThreatenedByPawn & (pos.pieces(BLACK, KNIGHT) | pos.pieces(BLACK, BISHOP))) {
        if (!(bDefendedByPawn & (pos.pieces(BLACK, KNIGHT) | pos.pieces(BLACK, BISHOP)))) { mg += ThreatByPawnMinorMG; eg += ThreatByPawnMinorEG; }
        else { mg += 8; eg += 8; }
    }
    
    Bitboard bThreatenedByMinor = pos.pieces(BLACK) & wMinorAtt;
    if (bThreatenedByMinor & pos.pieces(BLACK, QUEEN)) {
        if (!(bDefendedByPawn & pos.pieces(BLACK, QUEEN))) { mg += ThreatByMinorQueenMG; eg += 55; }
    }
    if (bThreatenedByMinor & pos.pieces(BLACK, ROOK)) {
        if (!(bDefendedByPawn & pos.pieces(BLACK, ROOK))) { mg += ThreatByMinorRookMG; eg += 30; }
    }
    
    Bitboard bThreatenedByRook = pos.pieces(BLACK) & wRookAtt;
    if (bThreatenedByRook & pos.pieces(BLACK, QUEEN)) {
        if (!(bDefendedByPawn & pos.pieces(BLACK, QUEEN))) { mg += ThreatByRookQueenMG; eg += 40; }
    }

    // --- DYNAMIC KING TENSION (Professional Weights) ---
    bool isMiddlegame = pos.pieces(WHITE, QUEEN) || pos.pieces(BLACK, QUEEN);
    if (isMiddlegame) {
        // White King Tension
        Square wKingSq = pos.king_square(WHITE);
        Bitboard wKingZone = king_attacks_bb(wKingSq) | square_bb(wKingSq);
        
        int wAttackUnits = 0;
        wAttackUnits += popcount(wKingZone & bPawnAtt) * 2;
        wAttackUnits += popcount(wKingZone & bKnightAtt) * 2;
        wAttackUnits += popcount(wKingZone & bBishopAtt) * 3;
        wAttackUnits += popcount(wKingZone & bRookAtt) * 5;
        wAttackUnits += popcount(wKingZone & bQueenAtt) * 6;
        
        int wDefenders = popcount(wKingZone & wPawnAtt) + popcount(wKingZone & wMinorAtt) + popcount(wKingZone & wRookAtt);
        wAttackUnits -= wDefenders * 2; 
        wAttackUnits = std::max(0, std::min(wAttackUnits, 31));
        mg -= KingSafetyTable[wAttackUnits];
        
        // Black King Tension
        Square bKingSq = pos.king_square(BLACK);
        Bitboard bKingZone = king_attacks_bb(bKingSq) | square_bb(bKingSq);
        
        int bAttackUnits = 0;
        bAttackUnits += popcount(bKingZone & wPawnAtt) * 2;
        bAttackUnits += popcount(bKingZone & wKnightAtt) * 2;
        bAttackUnits += popcount(bKingZone & wBishopAtt) * 3;
        bAttackUnits += popcount(bKingZone & wRookAtt) * 5;
        bAttackUnits += popcount(bKingZone & wQueenAtt) * 6;
        
        int bDefenders = popcount(bKingZone & bPawnAtt) + popcount(bKingZone & bMinorAtt) + popcount(bKingZone & bRookAtt);
        bAttackUnits -= bDefenders * 2;
        bAttackUnits = std::max(0, std::min(bAttackUnits, 31));
        mg += KingSafetyTable[bAttackUnits]; 
    }
}

} // namespace lofty

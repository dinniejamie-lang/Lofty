// eval.cpp — Tapered evaluation with Pawn Hash, Dynamic Bishop Pair, Eyesight, Relations, & Fischval.
#include "eval.h"
#include "position.h"
#include "bitboard.h"
#include "eyesight.h"
#include "relations.h"
#include "fischval.h"
#include "simd.h"

#include <array>
#include <algorithm>
#include <cstring>

namespace lofty {

struct PawnEntry {
    Key key;
    int mg;
    int eg;
};

static constexpr int PawnHashSize = 16384; 
static std::array<PawnEntry, PawnHashSize> PawnHashTable;

static Bitboard IsolatedMask[64];
static Bitboard PassedMask[2][64];

void init_eval() {
    for (int sq = 0; sq < 64; ++sq) {
        File f = file_of(Square(sq));
        Rank r = rank_of(Square(sq));
        
        Bitboard adjFiles = 0;
        if (f > FILE_A) adjFiles |= FILE_BBS[f - 1];
        if (f < FILE_H) adjFiles |= FILE_BBS[f + 1];
        
        IsolatedMask[sq] = adjFiles;
        
        Bitboard whiteInFront = 0;
        for (int rr = r + 1; rr <= RANK_8; ++rr) whiteInFront |= RANK_BBS[rr];
        PassedMask[WHITE][sq] = (adjFiles | FILE_BBS[f]) & whiteInFront;
        
        Bitboard blackInFront = 0;
        for (int rr = r - 1; rr >= RANK_1; --rr) blackInFront |= RANK_BBS[rr];
        PassedMask[BLACK][sq] = (adjFiles | FILE_BBS[f]) & blackInFront;
    }
    
    std::memset(PawnHashTable.data(), 0, sizeof(PawnEntry) * PawnHashSize);
}

static void evaluate_pawns(const Position& pos, PawnEntry& entry) {
    int mg = 0;
    int eg = 0;

#if LOFTY_AVX2 && defined(__AVX2__)
    // SIMD-accelerated pawn evaluation for AVX2-capable CPUs
    // Process pawns in batches of 4 using AVX2 instructions
    
    for (Color c : {WHITE, BLACK}) {
        Bitboard pawns = pos.pieces(c, PAWN);
        Bitboard ourPawns = pawns;
        Bitboard theirPawns = pos.pieces(~c, PAWN);
        
        // Collect up to 4 pawns for SIMD processing
        simd::PawnEvalState state;
        state.count = 0;
        
        while (pawns && state.count < 4) {
            Square s = pop_lsb(pawns);
            File f = file_of(s);
            
            state.our_pawns[state.count] = square_bb(s);
            state.their_pawns[state.count] = theirPawns;
            
            // Pre-compute isolation mask for this pawn
            Bitboard adjFiles = 0;
            if (f > FILE_A) adjFiles |= FILE_BBS[f - 1];
            if (f < FILE_H) adjFiles |= FILE_BBS[f + 1];
            state.isolated_masks[state.count] = adjFiles;
            
            // Pre-compute passed pawn mask
            Bitboard inFront = 0;
            Rank r = rank_of(s);
            if (c == WHITE) {
                for (int rr = int(r) + 1; rr <= RANK_8; ++rr) 
                    inFront |= RANK_BBS[rr];
            } else {
                for (int rr = int(r) - 1; rr >= RANK_1; --rr) 
                    inFront |= RANK_BBS[rr];
            }
            state.passed_masks[state.count] = (adjFiles | FILE_BBS[f]) & inFront;
            
            state.count++;
        }
        
        // Process 4 pawns in parallel if we have enough
        if (state.count == 4) {
            int batch_mg = 0, batch_eg = 0;
            simd::evaluate_pawns_simd(state, batch_mg, batch_eg);
            mg += batch_mg;
            eg += batch_eg;
            
            // Continue with remaining pawns using scalar code
            while (pawns) {
                Square s = pop_lsb(pawns);
                File f = file_of(s);
                Rank r = rank_of(s);
                
                Bitboard adjOurPawns = ourPawns & FILE_BBS[f];
                if (adjOurPawns & (c == WHITE ? (square_bb(s) >> 8) : (square_bb(s) << 8))) {
                    mg -= 10;
                    eg -= 20;
                }
                
                Bitboard adjFiles = 0;
                if (f > FILE_A) adjFiles |= FILE_BBS[f - 1];
                if (f < FILE_H) adjFiles |= FILE_BBS[f + 1];
                
                if (!(ourPawns & adjFiles)) {
                    mg -= 15;
                    eg -= 15;
                }
                
                Bitboard theirInFront = theirPawns & ((adjFiles | FILE_BBS[f]) & 
                    (c == WHITE ? (~RANK_BBS[RANK_1] << (int(r)*8)) : (~RANK_BBS[RANK_8] >> (64-(int(r)*8)))));
                
                bool passed = true;
                for (int checkSq = 0; checkSq < 64; checkSq++) {
                    if (theirInFront & square_bb(Square(checkSq))) {
                        passed = false;
                        break;
                    }
                }
                
                if (passed && !(theirPawns & PassedMask[c][s])) {
                    int rank = (c == WHITE) ? int(r) : (7 - int(r));
                    int bonus = rank * rank * 5;
                    mg += bonus;
                    eg += bonus + rank * 10;
                }
            }
        } else {
            // Fallback to scalar for fewer than 4 pawns
            pawns = pos.pieces(c, PAWN);
            while (pawns) {
                Square s = pop_lsb(pawns);
                File f = file_of(s);
                Rank r = rank_of(s);
                
                if (ourPawns & file_bb(f) & (c == WHITE ? (square_bb(s) >> 8) : (square_bb(s) << 8))) {
                    mg -= 10;
                    eg -= 20;
                }
                
                if (!(ourPawns & IsolatedMask[s])) {
                    mg -= 15;
                    eg -= 15;
                }
                
                if (!(theirPawns & PassedMask[c][s])) {
                    int rank = (c == WHITE) ? int(r) : (7 - int(r));
                    int bonus = rank * rank * 5;
                    mg += bonus;
                    eg += bonus + rank * 10;
                }
            }
        }
    }
#else
    // Scalar fallback for non-AVX2 CPUs
    for (Color c : {WHITE, BLACK}) {
        Bitboard pawns = pos.pieces(c, PAWN);
        Bitboard ourPawns = pawns;
        Bitboard theirPawns = pos.pieces(~c, PAWN);
        
        while (pawns) {
            Square s = pop_lsb(pawns);
            File f = file_of(s);
            Rank r = rank_of(s);
            
            if (ourPawns & file_bb(f) & (c == WHITE ? (square_bb(s) >> 8) : (square_bb(s) << 8))) {
                mg -= 10;
                eg -= 20;
            }
            
            if (!(ourPawns & IsolatedMask[s])) {
                mg -= 15;
                eg -= 15;
            }
            
            if (!(theirPawns & PassedMask[c][s])) {
                int rank = (c == WHITE) ? int(r) : (7 - int(r));
                int bonus = rank * rank * 5;
                mg += bonus;
                eg += bonus + rank * 10;
            }
        }
    }
#endif
    
    entry.mg = mg;
    entry.eg = eg;
}

Value evaluate(const Position& pos) {
    int mg = pos.psqt_mg();
    int eg = pos.psqt_eg();
    
    // 1. Pawn Structure (Pawn Hash Table)
    Key pawnKey = 0;
    Bitboard wp = pos.pieces(WHITE, PAWN);
    while (wp) pawnKey ^= Zobrist::psq[W_PAWN][pop_lsb(wp)];
    Bitboard bp = pos.pieces(BLACK, PAWN);
    while (bp) pawnKey ^= Zobrist::psq[B_PAWN][pop_lsb(bp)];
    
    size_t idx = pawnKey & (PawnHashSize - 1);
    PawnEntry& pe = PawnHashTable[idx];
    
    if (pe.key != pawnKey) {
        pe.key = pawnKey;
        evaluate_pawns(pos, pe);
    }
    mg += pe.mg;
    eg += pe.eg;
    
    // 2. Dynamic Bishop Pair
    Bitboard wPawns = pos.pieces(WHITE, PAWN);
    Bitboard bPawns = pos.pieces(BLACK, PAWN);
    
    bool bPawnsBothSides = (bPawns & (FILE_BBS[FILE_A] | FILE_BBS[FILE_B] | FILE_BBS[FILE_C] | FILE_BBS[FILE_D])) && 
                           (bPawns & (FILE_BBS[FILE_E] | FILE_BBS[FILE_F] | FILE_BBS[FILE_G] | FILE_BBS[FILE_H]));
    if (popcount(pos.pieces(WHITE, BISHOP)) >= 2) {
        mg += bPawnsBothSides ? 40 : 20; 
        eg += bPawnsBothSides ? 60 : 30;
    }
    
    bool wPawnsBothSides = (wPawns & (FILE_BBS[FILE_A] | FILE_BBS[FILE_B] | FILE_BBS[FILE_C] | FILE_BBS[FILE_D])) && 
                           (wPawns & (FILE_BBS[FILE_E] | FILE_BBS[FILE_F] | FILE_BBS[FILE_G] | FILE_BBS[FILE_H]));
    if (popcount(pos.pieces(BLACK, BISHOP)) >= 2) {
        mg -= wPawnsBothSides ? 40 : 20; 
        eg -= wPawnsBothSides ? 60 : 30;
    }
    
    // 3. Eyesight: Supported Mobility & Open Files
    evaluate_misc(pos, mg, eg);
    
    // 4. Relations: Threats, Pressure, and Geometric King Safety
    evaluate_relations(pos, mg, eg);
    
    // 5. Fischval: Fischer Vectors (Key Attackers, Hollow Threats, All Piece Drawbacks)
    evaluate_fischval(pos, mg, eg);
    
    // 6. Tempo Bonus
    mg += 10;
    eg += 5;
    
    // 7. Tapered Evaluation
    int phase = 24;
    phase -= popcount(pos.pieces(KNIGHT)) * 1;
    phase -= popcount(pos.pieces(BISHOP)) * 1;
    phase -= popcount(pos.pieces(ROOK))   * 2;
    phase -= popcount(pos.pieces(QUEEN))  * 4;
    phase = std::max(0, phase);
    
    int score = (mg * phase + eg * (24 - phase)) / 24;
    
    return (pos.side_to_move() == WHITE) ? Value(score) : Value(-score);
}

} // namespace lofty
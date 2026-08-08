// movegen.cpp — pseudo-legal generation + legality test.
#include "movegen.h"
#include "position.h"
#include "bitboard.h"

#include <cassert>

namespace lofty {

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------
static inline void add_move(MoveList& list, Square from, Square to, MoveFlag flag) {
    list.add(Move(from, to, flag));
}

// ----------------------------------------------------------------------------
// Pawn move generation.
// ----------------------------------------------------------------------------
static void generate_pawn_moves(const Position& pos, MoveList& list, Color us) {
    Color them = ~us;
    Bitboard pawns   = pos.pieces(us, PAWN);
    Bitboard empty   = ~pos.pieces();
    Bitboard enemies = pos.pieces(them);
    Bitboard notA = ~FILE_BBS[FILE_A];
    Bitboard notH = ~FILE_BBS[FILE_H];

    Bitboard promoRank, rank3or6;
    int forward; // push offset: +8 for white, -8 for black

    if (us == WHITE) {
        promoRank = RANK_BBS[RANK_8];
        rank3or6  = RANK_BBS[RANK_3];
        forward   = 8;
    } else {
        promoRank = RANK_BBS[RANK_1];
        rank3or6  = RANK_BBS[RANK_6];
        forward   = -8;
    }

    // --- Single pushes ---
    Bitboard singlePush = (us == WHITE) ? ((pawns << 8) & empty)
                                        : ((pawns >> 8) & empty);
    Bitboard pushPromo = singlePush & promoRank;
    Bitboard pushQuiet = singlePush & ~promoRank;

    // --- Double pushes ---
    Bitboard doublePush = (us == WHITE) ? ((pushQuiet & rank3or6) << 8) & empty
                                        : ((pushQuiet & rank3or6) >> 8) & empty;

    // --- Captures ---
    Bitboard capLeft, capRight; // left=A-file side, right=H-file side
    int leftShift, rightShift;
    if (us == WHITE) {
        capLeft   = ((pawns & notA) << 7) & enemies;  // NW (+7)
        capRight  = ((pawns & notH) << 9) & enemies;  // NE (+9)
        leftShift  = 7;
        rightShift = 9;
    } else {
        capLeft   = ((pawns & notA) >> 9) & enemies;  // SW (-9)
        capRight  = ((pawns & notH) >> 7) & enemies;  // SE (-7)
        leftShift  = -9;
        rightShift = -7;
    }

    Bitboard capLeftPromo  = capLeft  & promoRank;
    Bitboard capLeftQuiet  = capLeft  & ~promoRank;
    Bitboard capRightPromo = capRight & promoRank;
    Bitboard capRightQuiet = capRight & ~promoRank;

    // --- Emit: quiet pushes ---
    for (Bitboard b = pushQuiet; b; ) {
        Square to = pop_lsb(b);
        add_move(list, Square(int(to) - forward), to, FLAG_QUIET);
    }
    // --- Emit: promotion pushes (4 pieces each) ---
    for (Bitboard b = pushPromo; b; ) {
        Square to = pop_lsb(b);
        Square from = Square(int(to) - forward);
        for (MoveFlag f : {FLAG_QUEEN_PROMO, FLAG_ROOK_PROMO, FLAG_BISHOP_PROMO, FLAG_KNIGHT_PROMO})
            add_move(list, from, to, f);
    }
    // --- Emit: double pushes ---
    for (Bitboard b = doublePush; b; ) {
        Square to = pop_lsb(b);
        add_move(list, Square(int(to) - 2 * forward), to, FLAG_DOUBLE_PUSH);
    }
    // --- Emit: quiet captures (left + right) ---
    for (Bitboard b = capLeftQuiet; b; ) {
        Square to = pop_lsb(b);
        add_move(list, Square(int(to) - leftShift), to, FLAG_CAPTURE);
    }
    for (Bitboard b = capRightQuiet; b; ) {
        Square to = pop_lsb(b);
        add_move(list, Square(int(to) - rightShift), to, FLAG_CAPTURE);
    }
    // --- Emit: promotion captures (left + right, 4 pieces each) ---
    for (Bitboard b = capLeftPromo; b; ) {
        Square to = pop_lsb(b);
        Square from = Square(int(to) - leftShift);
        for (MoveFlag f : {FLAG_QUEEN_PROMO_CAP, FLAG_ROOK_PROMO_CAP, FLAG_BISHOP_PROMO_CAP, FLAG_KNIGHT_PROMO_CAP})
            add_move(list, from, to, f);
    }
    for (Bitboard b = capRightPromo; b; ) {
        Square to = pop_lsb(b);
        Square from = Square(int(to) - rightShift);
        for (MoveFlag f : {FLAG_QUEEN_PROMO_CAP, FLAG_ROOK_PROMO_CAP, FLAG_BISHOP_PROMO_CAP, FLAG_KNIGHT_PROMO_CAP})
            add_move(list, from, to, f);
    }

    // --- En passant ---
    Square ep = pos.ep_square();
    if (ep != SQ_NONE) {
        Bitboard epAttackers = pawn_attacks_bb(them, ep) & pawns;
        while (epAttackers) {
            Square from = pop_lsb(epAttackers);
            add_move(list, from, ep, FLAG_EP_CAPTURE);
        }
    }
}

// ----------------------------------------------------------------------------
// Castling generation.
// ----------------------------------------------------------------------------
static void generate_castling(const Position& pos, MoveList& list, Color us) {
    Color them = ~us;
    CastleRights rights = pos.castling_rights();
    Bitboard occ = pos.pieces();

    if (us == WHITE) {
        if (pos.king_square(WHITE) != SQ_E1) return;

        if ((rights & WHITE_OO)
            && !(occ & (square_bb(SQ_F1) | square_bb(SQ_G1)))
            && !pos.attackers_to(SQ_E1, them)
            && !pos.attackers_to(SQ_F1, them)
            && !pos.attackers_to(SQ_G1, them))
        {
            add_move(list, SQ_E1, SQ_G1, FLAG_KING_CASTLE);
        }
        if ((rights & WHITE_OOO)
            && !(occ & (square_bb(SQ_B1) | square_bb(SQ_C1) | square_bb(SQ_D1)))
            && !pos.attackers_to(SQ_E1, them)
            && !pos.attackers_to(SQ_D1, them)
            && !pos.attackers_to(SQ_C1, them))
        {
            add_move(list, SQ_E1, SQ_C1, FLAG_QUEEN_CASTLE);
        }
    } else {
        if (pos.king_square(BLACK) != SQ_E8) return;

        if ((rights & BLACK_OO)
            && !(occ & (square_bb(SQ_F8) | square_bb(SQ_G8)))
            && !pos.attackers_to(SQ_E8, them)
            && !pos.attackers_to(SQ_F8, them)
            && !pos.attackers_to(SQ_G8, them))
        {
            add_move(list, SQ_E8, SQ_G8, FLAG_KING_CASTLE);
        }
        if ((rights & BLACK_OOO)
            && !(occ & (square_bb(SQ_B8) | square_bb(SQ_C8) | square_bb(SQ_D8)))
            && !pos.attackers_to(SQ_E8, them)
            && !pos.attackers_to(SQ_D8, them)
            && !pos.attackers_to(SQ_C8, them))
        {
            add_move(list, SQ_E8, SQ_C8, FLAG_QUEEN_CASTLE);
        }
    }
}

// ----------------------------------------------------------------------------
// generate_pseudo_legal — all piece moves + castling.
// ----------------------------------------------------------------------------
int generate_pseudo_legal(const Position& pos, MoveList& list) {
    list.clear();

    Color us = pos.side_to_move();
    Color them = ~us;
    Bitboard occ = pos.pieces();
    Bitboard usPieces = pos.pieces(us);
    Bitboard themPieces = pos.pieces(them);

    // --- Pawns ---
    generate_pawn_moves(pos, list, us);

    // --- Knights ---
    for (Bitboard b = pos.pieces(us, KNIGHT); b; ) {
        Square from = pop_lsb(b);
        Bitboard targets = knight_attacks_bb(from) & ~usPieces;
        while (targets) {
            Square to = pop_lsb(targets);
            MoveFlag f = (themPieces & square_bb(to)) ? FLAG_CAPTURE : FLAG_QUIET;
            add_move(list, from, to, f);
        }
    }

    // --- Bishops ---
    for (Bitboard b = pos.pieces(us, BISHOP); b; ) {
        Square from = pop_lsb(b);
        Bitboard targets = attacks_bb(BISHOP, from, occ) & ~usPieces;
        while (targets) {
            Square to = pop_lsb(targets);
            MoveFlag f = (themPieces & square_bb(to)) ? FLAG_CAPTURE : FLAG_QUIET;
            add_move(list, from, to, f);
        }
    }

    // --- Rooks ---
    for (Bitboard b = pos.pieces(us, ROOK); b; ) {
        Square from = pop_lsb(b);
        Bitboard targets = attacks_bb(ROOK, from, occ) & ~usPieces;
        while (targets) {
            Square to = pop_lsb(targets);
            MoveFlag f = (themPieces & square_bb(to)) ? FLAG_CAPTURE : FLAG_QUIET;
            add_move(list, from, to, f);
        }
    }

    // --- Queens ---
    for (Bitboard b = pos.pieces(us, QUEEN); b; ) {
        Square from = pop_lsb(b);
        Bitboard targets = attacks_bb(QUEEN, from, occ) & ~usPieces;
        while (targets) {
            Square to = pop_lsb(targets);
            MoveFlag f = (themPieces & square_bb(to)) ? FLAG_CAPTURE : FLAG_QUIET;
            add_move(list, from, to, f);
        }
    }

    // --- King (non-castling) ---
    {
        Square from = pos.king_square(us);
        Bitboard targets = king_attacks_bb(from) & ~usPieces;
        while (targets) {
            Square to = pop_lsb(targets);
            MoveFlag f = (themPieces & square_bb(to)) ? FLAG_CAPTURE : FLAG_QUIET;
            add_move(list, from, to, f);
        }
    }

    // --- Castling ---
    generate_castling(pos, list, us);

    return list.size();
}

// ----------------------------------------------------------------------------
// Position::is_legal — test a pseudo-legal move by copy-make + in_check.
// ----------------------------------------------------------------------------
bool Position::is_legal(Move m) const {
    assert(m != MOVE_NONE);
    Color us = side_;

    Position copy = *this;  // copy-make: ~168 bytes, 3 cache lines
    copy.make_move(m);
    return !copy.in_check(us);
}

// ----------------------------------------------------------------------------
// generate_legal — pseudo-legal moves filtered to strictly legal.
// ----------------------------------------------------------------------------
int generate_legal(const Position& pos, MoveList& list) {
    generate_pseudo_legal(pos, list);

    int write = 0;
    int total = list.size();
    for (int i = 0; i < total; ++i) {
        if (pos.is_legal(list[i])) {
            list[write++] = list[i];
        }
    }
    list.set_size(write);
    return write;
}

} // namespace lofty
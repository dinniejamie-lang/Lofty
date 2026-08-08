// position.h — board state, FEN interface, make_move, Zobrist declarations.
// Now includes Incremental Evaluation (PSQT) state.
#ifndef LOFTY_POSITION_H
#define LOFTY_POSITION_H

#include "types.h"
#include "bitboard.h"

#include <string>
#include <cstdint>
#include <array>

namespace lofty {

// ----------------------------------------------------------------------------
// Zobrist keys
// ----------------------------------------------------------------------------
namespace Zobrist {
    extern Key psq[PIECE_NB][SQUARE_NB];
    extern Key side;
    extern Key castling[16];
    extern Key enpassant[SQUARE_NB];
    void init();
}

// ----------------------------------------------------------------------------
// Evaluation Constants (Moved here for incremental updates)
// ----------------------------------------------------------------------------
constexpr int PawnValue   = 100;
constexpr int KnightValue = 320;
constexpr int BishopValue = 330;
constexpr int RookValue   = 500;
constexpr int QueenValue  = 900;

static constexpr int MaterialTable[PIECE_TYPE_NB] = {
    0, PawnValue, KnightValue, BishopValue, RookValue, QueenValue, 0
};

// Piece-Square Tables (from White's perspective, a1=0, h8=63)
using PST = std::array<int, 64>;

constexpr PST pawnMg = {
    0, 0, 0, 0, 0, 0, 0, 0,
    -35, -1, -20, -23, -15, 24, 38, -22,
    -26, -4, -4, -10, 3, 20, 23, -5,
    -27, -2, 5, 12, 17, 6, 20, -2,
    -14, 13, 6, 21, -3, 17, 23, 5,
    -6, 8, 7, 15, 6, 21, 18, 2,
    -15, 1, 4, 15, 7, 22, 18, 2,
    0, 0, 0, 0, 0, 0, 0, 0
};

constexpr PST pawnEg = {
    0, 0, 0, 0, 0, 0, 0, 0,
    13, 8, 8, 10, 13, 0, 2, -7,
    4, 7, -6, 1, 0, -5, -1, -8,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, 0, 0, 0, 0, -2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

constexpr PST knightMg = {
    -105, -21, -58, -33, -17, -28, -19, -23,
    -29, -53, -12, -3, -1, 18, -42, -9,
    -23, -9, 12, 10, 19, 17, 25, -16,
    -13, 4, 16, 13, 28, 19, 21, -8,
    -9, 17, 19, 53, 37, 69, 18, 22,
    -47, 60, 37, 65, 84, 129, 73, 16,
    -73, -41, 72, 36, 23, 62, 7, -17,
    -175, -80, -72, -15, -24, -22, -42, -51
};

constexpr PST knightEg = {
    -53, -34, -21, -11, -28, -14, -23, -46,
    -15, -14, -10, -2, 1, -2, -11, -16,
    -7, 7, 9, 8, 8, 10, 7, -6,
    -3, -2, 3, 5, 1, 0, -1, -6,
    -4, 0, 2, 4, 0, 0, 2, -5,
    3, 2, 6, 6, 5, 5, 3, 1,
    53, 37, 22, 8, -3, 4, 10, 10,
    39, 26, 27, -6, -8, -8, -3, -3
};

constexpr PST bishopMg = {
    -37, -1, -5, -3, -3, -5, -1, -37,
    -16, -13, -1, 2, 2, -1, -13, -16,
    -16, 0, 3, 5, 5, 3, 0, -16,
    -17, -2, -1, 0, 0, -1, -2, -17,
    -17, -2, -1, 0, 0, -1, -2, -17,
    -16, 0, 3, 5, 5, 3, 0, -16,
    -16, -13, -1, 2, 2, -1, -13, -16,
    -37, -1, -5, -3, -3, -5, -1, -37
};

constexpr PST bishopEg = {
    -14, -1, -8, -2, -2, -8, -1, -14,
    -10, 0, 1, 3, 3, 1, 0, -10,
    -2, 1, 3, 6, 6, 3, 1, -2,
    1, 2, 5, 7, 7, 5, 2, 1,
    1, 2, 5, 7, 7, 5, 2, 1,
    -2, 1, 3, 6, 6, 3, 1, -2,
    -10, 0, 1, 3, 3, 1, 0, -10,
    -14, -1, -8, -2, -2, -8, -1, -14
};

constexpr PST rookMg = {
    0, 0, 0, 0, 0, 0, 0, 0,
    3, 5, 8, 11, 11, 8, 5, 3,
    0, 0, 0, 0, 0, 0, 0, 0,
    -4, 0, 1, 3, 3, 1, 0, -4,
    -4, 0, 1, 3, 3, 1, 0, -4,
    -4, 0, 1, 3, 3, 1, 0, -4,
    -4, 0, 1, 3, 3, 1, 0, -4,
    0, 0, 0, 0, 0, 0, 0, 0
};

constexpr PST rookEg = {
    0, 0, 0, 0, 0, 0, 0, 0,
    -12, -10, -9, -9, -9, -9, -10, -12,
    -7, -4, -3, -2, -2, -3, -4, -7,
    -2, 0, 1, 1, 1, 1, 0, -2,
    -2, 0, 1, 1, 1, 1, 0, -2,
    -2, 0, 1, 1, 1, 1, 0, -2,
    -2, 0, 1, 1, 1, 1, 0, -2,
    0, 0, 0, 0, 0, 0, 0, 0
};

constexpr PST queenMg = {
    -33, -3, -7, -3, -3, -7, -3, -33,
    -3, 3, 3, 3, 3, 3, 3, -3,
    -3, 3, 6, 6, 6, 6, 3, -3,
    -3, 3, 6, 8, 8, 6, 3, -3,
    -3, 3, 6, 8, 8, 6, 3, -3,
    -3, 3, 6, 6, 6, 6, 3, -3,
    -3, 3, 3, 3, 3, 3, 3, -3,
    -33, -3, -7, -3, -3, -7, -3, -33
};

constexpr PST queenEg = {
    -22, -7, -5, -4, -4, -5, -7, -22,
    -7, 0, 1, 2, 2, 1, 0, -7,
    -5, 1, 3, 4, 4, 3, 1, -5,
    -4, 2, 4, 5, 5, 4, 2, -4,
    -4, 2, 4, 5, 5, 4, 2, -4,
    -5, 1, 3, 4, 4, 3, 1, -5,
    -7, 0, 1, 2, 2, 1, 0, -7,
    -22, -7, -5, -4, -4, -5, -7, -22
};

constexpr PST kingMg = {
    7, 8, 8, 8, 8, 8, 8, 7,
    7, 8, 8, 8, 8, 8, 8, 7,
    7, 8, 8, 8, 8, 8, 8, 7,
    -7, -4, 2, 5, 5, 2, -4, -7,
    -13, -9, 2, 8, 8, 2, -9, -13,
    -15, -11, -1, 6, 6, -1, -11, -15,
    -25, -17, -5, 1, 1, -5, -17, -25,
    -40, -25, -12, -3, -3, -12, -25, -40
};

constexpr PST kingEg = {
    22, 30, 34, 34, 34, 34, 30, 22,
    27, 32, 37, 37, 37, 37, 32, 27,
    23, 28, 33, 34, 34, 33, 28, 23,
    20, 24, 28, 30, 30, 28, 24, 20,
    17, 21, 24, 26, 26, 24, 21, 17,
    14, 17, 20, 21, 21, 20, 17, 14,
    11, 13, 15, 16, 16, 15, 13, 11,
    8, 10, 12, 13, 13, 12, 10, 8
};

// Array of PSTs for easy indexing by PieceType
static constexpr PST PstMg[PIECE_TYPE_NB] = { {}, pawnMg, knightMg, bishopMg, rookMg, queenMg, kingMg };
static constexpr PST PstEg[PIECE_TYPE_NB] = { {}, pawnEg, knightEg, bishopEg, rookEg, queenEg, kingEg };

// ----------------------------------------------------------------------------
// Position — complete state of a chess position.
// ----------------------------------------------------------------------------
class Position {
    Bitboard byColor[COLOR_NB];
    Bitboard byType[PIECE_TYPE_NB];
    int8_t   board[SQUARE_NB];

    Color        side_;
    CastleRights castling_;
    Square       epSquare_;
    int          halfmoveClock_;
    int          fullmoveNumber_;
    int          gamePly_;
    Key          key_;
    
    // Incremental Evaluation State (MG / EG)
    int32_t psqt[2]; 

public:
    Position() { clear(); }

    void clear();
    bool set_fen(const std::string& fen);
    std::string fen() const;

    Bitboard pieces()                        const { return byType[0]; }
    Bitboard pieces(Color c)                 const { return byColor[c]; }
    Bitboard pieces(PieceType pt)            const { return byType[pt]; }
    Bitboard pieces(Color c, PieceType pt)   const { return byColor[c] & byType[pt]; }

    Piece piece_on(Square s)            const { return Piece(board[int(s)]); }
    bool  empty(Square s)               const { return board[int(s)] == NO_PIECE; }
    Square king_square(Color c)         const { return lsb(pieces(c, KING)); }

    Color        side_to_move()    const { return side_; }
    CastleRights castling_rights() const { return castling_; }
    Square       ep_square()       const { return epSquare_; }
    int          halfmove_clock()  const { return halfmoveClock_; }
    int          fullmove_number() const { return fullmoveNumber_; }
    int          game_ply()        const { return gamePly_; }
    Key          key()             const { return key_; }
    
    // Incremental Eval Accessors
    int psqt_mg() const { return psqt[0]; }
    int psqt_eg() const { return psqt[1]; }

    void make_move(Move m);
    void do_null_move();

    Bitboard attackers_to(Square s, Color by) const;
    Bitboard checkers() const;
    bool     in_check(Color c) const;
    bool     in_check() const { return in_check(side_); }

    bool is_legal(Move m) const;

private:
    void put_piece(Piece p, Square s);
    void remove_piece(Square s);
    void move_piece(Square from, Square to);

    void xor_key(Key k) { key_ ^= k; }
};

} // namespace lofty

#endif // LOFTY_POSITION_H
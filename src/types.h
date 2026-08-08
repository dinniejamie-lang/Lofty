// types.h — fundamental types and constants for Lofty (corrected ordering)
#ifndef LOFTY_TYPES_H
#define LOFTY_TYPES_H

#include <cstdint>

namespace lofty {

// ----------------------------------------------------------------------------
// Bitboard
// ----------------------------------------------------------------------------
using Bitboard = uint64_t;
inline constexpr int SQUARE_NB = 64;

// ----------------------------------------------------------------------------
// Square: a1 = 0, b1 = 1, ..., h1 = 7, a2 = 8, ..., h8 = 63
// ----------------------------------------------------------------------------
enum Square : int {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE = 64
};

inline constexpr Square& operator++(Square& s)        { s = Square(int(s) + 1); return s; }
inline constexpr Square  operator++(Square& s, int)   { Square r = s; ++s; return r; }
inline constexpr Square  operator+ (Square s, int i)  { return Square(int(s) + i); }
inline constexpr Square  operator- (Square s, int i)  { return Square(int(s) - i); }

// ----------------------------------------------------------------------------
// File / Rank
// ----------------------------------------------------------------------------
enum File : int { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };
enum Rank : int { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };

inline constexpr File    file_of(Square s)      { return File(int(s) & 7); }
inline constexpr Rank    rank_of(Square s)      { return Rank(int(s) >> 3); }
inline constexpr Square  make_square(File f, Rank r) { return Square((int(r) << 3) + int(f)); }

// ----------------------------------------------------------------------------
// Direction
// ----------------------------------------------------------------------------
enum Direction : int {
    NORTH = 8, SOUTH = -8, EAST = 1, WEST = -1,
    NORTH_EAST = 9, NORTH_WEST = 7, SOUTH_EAST = -7, SOUTH_WEST = -9
};

// ----------------------------------------------------------------------------
// Color
// ----------------------------------------------------------------------------
enum Color : int { WHITE = 0, BLACK = 1, COLOR_NB = 2 };
inline constexpr Color operator~(Color c) { return Color(int(c) ^ 1); }

// ----------------------------------------------------------------------------
// PieceType
// ----------------------------------------------------------------------------
enum PieceType : int {
    NO_PIECE_TYPE = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3,
    ROOK = 4, QUEEN = 5, KING = 6, PIECE_TYPE_NB = 7
};

// ----------------------------------------------------------------------------
// Piece: bit 3 = color, bits 0-2 = type
// ----------------------------------------------------------------------------
enum Piece : int {
    NO_PIECE = 0,
    W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = 9, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
};

inline constexpr Piece     make_piece(Color c, PieceType pt) { return Piece((int(c) << 3) + int(pt)); }
inline constexpr PieceType type_of(Piece p)                  { return PieceType(int(p) & 7); }
inline constexpr Color     color_of(Piece p)                 { return Color(int(p) >> 3); }

// ----------------------------------------------------------------------------
// Castling rights
// ----------------------------------------------------------------------------
enum CastleRights : int {
    NO_CASTLING  = 0,
    WHITE_OO     = 1,
    WHITE_OOO    = 2,
    BLACK_OO     = 4,
    BLACK_OOO    = 8,
    ANY_CASTLING = 15
};

inline constexpr CastleRights operator| (CastleRights a, CastleRights b) { return CastleRights(int(a) | int(b)); }
inline constexpr CastleRights operator& (CastleRights a, CastleRights b) { return CastleRights(int(a) & int(b)); }
inline constexpr CastleRights operator^ (CastleRights a, CastleRights b) { return CastleRights(int(a) ^ int(b)); }
inline constexpr CastleRights operator~ (CastleRights a)                 { return CastleRights(int(a) ^ ANY_CASTLING); }
inline constexpr CastleRights& operator|=(CastleRights& a, CastleRights b){ a = a | b; return a; }
inline constexpr CastleRights& operator&=(CastleRights& a, CastleRights b){ a = a & b; return a; }

// ----------------------------------------------------------------------------
// Value / Score
// ----------------------------------------------------------------------------
using Value = int32_t;
inline constexpr Value VALUE_ZERO             = 0;
inline constexpr Value VALUE_DRAW             = 0;
inline constexpr Value VALUE_MATE             = 32000;
inline constexpr Value VALUE_INFINITE         = 32001;
inline constexpr Value VALUE_MATE_IN_MAX_PLY  = VALUE_MATE - 256;

inline constexpr int  abs_val(int x)          { return x < 0 ? -x : x; }
inline constexpr bool is_mate_score(Value v)  { return abs_val(int(v)) >= VALUE_MATE_IN_MAX_PLY; }
inline constexpr int  mate_in(Value v)        { return (v > 0) ? (VALUE_MATE - v) : -(VALUE_MATE + v); }

// ----------------------------------------------------------------------------
// Depth / Key
// ----------------------------------------------------------------------------
using Depth = int;
inline constexpr Depth DEPTH_ZERO = 0;
inline constexpr Depth DEPTH_MAX  = 255;
using Key = uint64_t;

// ----------------------------------------------------------------------------
// Move flags
// ----------------------------------------------------------------------------
enum MoveFlag : int {
    FLAG_QUIET            = 0,
    FLAG_DOUBLE_PUSH      = 1,
    FLAG_KING_CASTLE      = 2,
    FLAG_QUEEN_CASTLE     = 3,
    FLAG_CAPTURE          = 4,
    FLAG_EP_CAPTURE       = 5,
    FLAG_KNIGHT_PROMO     = 8,
    FLAG_BISHOP_PROMO     = 9,
    FLAG_ROOK_PROMO       = 10,
    FLAG_QUEEN_PROMO      = 11,
    FLAG_KNIGHT_PROMO_CAP = 12,
    FLAG_BISHOP_PROMO_CAP = 13,
    FLAG_ROOK_PROMO_CAP   = 14,
    FLAG_QUEEN_PROMO_CAP  = 15
};

// ----------------------------------------------------------------------------
// Free-function flag predicates — declared BEFORE Move so the Move class
// methods can call them (required for MSVC, which does not defer lookup).
// ----------------------------------------------------------------------------
inline constexpr bool is_capture(MoveFlag f) {
    return f == FLAG_CAPTURE || f == FLAG_EP_CAPTURE
        || f == FLAG_KNIGHT_PROMO_CAP || f == FLAG_BISHOP_PROMO_CAP
        || f == FLAG_ROOK_PROMO_CAP   || f == FLAG_QUEEN_PROMO_CAP;
}
inline constexpr bool is_promotion(MoveFlag f) {
    return f >= FLAG_KNIGHT_PROMO && f <= FLAG_QUEEN_PROMO_CAP;
}
inline constexpr bool is_castle(MoveFlag f) {
    return f == FLAG_KING_CASTLE || f == FLAG_QUEEN_CASTLE;
}
inline constexpr bool is_ep(MoveFlag f) { return f == FLAG_EP_CAPTURE; }

inline constexpr PieceType promo_type(MoveFlag f) {
    switch (f) {
        case FLAG_KNIGHT_PROMO:     case FLAG_KNIGHT_PROMO_CAP: return KNIGHT;
        case FLAG_BISHOP_PROMO:     case FLAG_BISHOP_PROMO_CAP: return BISHOP;
        case FLAG_ROOK_PROMO:       case FLAG_ROOK_PROMO_CAP:   return ROOK;
        case FLAG_QUEEN_PROMO:      case FLAG_QUEEN_PROMO_CAP:  return QUEEN;
        default:                                                return NO_PIECE_TYPE;
    }
}

// ----------------------------------------------------------------------------
// Move encoding (16 bits): from:6 | to:6 | flag:4
// ----------------------------------------------------------------------------
class Move {
    uint16_t v_;
public:
    constexpr Move() : v_(0) {}
    constexpr explicit Move(uint16_t v) : v_(v) {}
    constexpr Move(Square from, Square to, MoveFlag flag)
        : v_(uint16_t(int(from) | (int(to) << 6) | (int(flag) << 12))) {}

    constexpr uint16_t  value() const { return v_; }
    constexpr Square    from()  const { return Square(v_ & 0x3F); }
    constexpr Square    to()    const { return Square((v_ >> 6) & 0x3F); }
    constexpr MoveFlag  flag()  const { return MoveFlag((v_ >> 12) & 0xF); }

    constexpr bool      is_capture()   const { return lofty::is_capture(flag()); }
    constexpr bool      is_promotion() const { return lofty::is_promotion(flag()); }
    constexpr bool      is_castle()    const { return lofty::is_castle(flag()); }
    constexpr bool      is_ep()        const { return lofty::is_ep(flag()); }
    constexpr PieceType promo()        const { return lofty::promo_type(flag()); }
};

inline constexpr bool operator==(Move a, Move b) { return a.value() == b.value(); }
inline constexpr bool operator!=(Move a, Move b) { return a.value() != b.value(); }
inline constexpr Move MOVE_NONE = Move(uint16_t(0));

} // namespace lofty

#endif // LOFTY_TYPES_H
// bitboard.h — bitboard utilities, leaper attack tables (constexpr),
//              and slider attack declarations (magic / PEXT).
// Depends only on types.h.
#ifndef LOFTY_BITBOARD_H
#define LOFTY_BITBOARD_H

#include "types.h"

#include <array>
#include <bit>
#include <cassert>

// Auto-detect BMI2 for the PEXT fast path. (Slower on AMD Zen1/Zen2, but
// we still enable it there for now; refine later with a runtime check.)
#if defined(__BMI2__) && (defined(__x86_64__) || defined(_M_X64))
  #include <immintrin.h>
  #define LOFTY_USE_PEXT 1
#else
  #define LOFTY_USE_PEXT 0
#endif

namespace lofty {

// ----------------------------------------------------------------------------
// Single-square, file, rank bitboards
// ----------------------------------------------------------------------------
namespace detail {
    inline constexpr std::array<Bitboard, 64> make_square_bbs() {
        std::array<Bitboard, 64> a{};
        for (int i = 0; i < 64; ++i) a[i] = Bitboard(1) << i;
        return a;
    }
    inline constexpr std::array<Bitboard, 8> make_file_bbs() {
        std::array<Bitboard, 8> a{};
        for (int f = 0; f < 8; ++f) {
            Bitboard b = 0;
            for (int r = 0; r < 8; ++r) b |= Bitboard(1) << (r * 8 + f);
            a[f] = b;
        }
        return a;
    }
    inline constexpr std::array<Bitboard, 8> make_rank_bbs() {
        std::array<Bitboard, 8> a{};
        for (int r = 0; r < 8; ++r) a[r] = Bitboard(0xFF) << (r * 8);
        return a;
    }
}

inline constexpr std::array<Bitboard, 64> SQUARE_BBS = detail::make_square_bbs();
inline constexpr std::array<Bitboard, 8>  FILE_BBS   = detail::make_file_bbs();
inline constexpr std::array<Bitboard, 8>  RANK_BBS   = detail::make_rank_bbs();

inline constexpr Bitboard square_bb(Square s) { return SQUARE_BBS[int(s)]; }
inline constexpr Bitboard file_bb(File f)     { return FILE_BBS[int(f)]; }
inline constexpr Bitboard rank_bb(Rank r)     { return RANK_BBS[int(r)]; }

// ----------------------------------------------------------------------------
// Popcount / LSB / MSB / pop
//   lsb/msb require b != 0.
// ----------------------------------------------------------------------------
inline int popcount(Bitboard b) { return std::popcount(b); }
inline Square lsb(Bitboard b)    { assert(b); return Square(std::countr_zero(b)); }
inline Square msb(Bitboard b)    { assert(b); return Square(63 - std::countl_zero(b)); }
inline Square pop_lsb(Bitboard& b) { Square s = lsb(b); b &= b - 1; return s; }

// more than one bit set?
inline bool more_than_one(Bitboard b) { return b != 0 && (b & (b - 1)) != 0; }
inline bool exactly_one(Bitboard b)   { return b != 0 && (b & (b - 1)) == 0; }

// ----------------------------------------------------------------------------
// Leaper attacks (knight / king / pawn) — constexpr, zero runtime init cost
// ----------------------------------------------------------------------------
namespace detail {
    inline constexpr Bitboard knight_attacks_impl(Square s) {
        Bitboard b = 0;
        int r = int(s) / 8, f = int(s) % 8;
        const int dr[8] = { 2, 2, -2, -2, 1, 1, -1, -1 };
        const int df[8] = { 1, -1, 1, -1, 2, -2, 2, -2 };
        for (int i = 0; i < 8; ++i) {
            int nr = r + dr[i], nf = f + df[i];
            if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8)
                b |= Bitboard(1) << (nr * 8 + nf);
        }
        return b;
    }
    inline constexpr Bitboard king_attacks_impl(Square s) {
        Bitboard b = 0;
        int r = int(s) / 8, f = int(s) % 8;
        for (int dr = -1; dr <= 1; ++dr)
            for (int df = -1; df <= 1; ++df)
                if ((dr || df) && r + dr >= 0 && r + dr < 8 && f + df >= 0 && f + df < 8)
                    b |= Bitboard(1) << ((r + dr) * 8 + (f + df));
        return b;
    }
    inline constexpr Bitboard pawn_attacks_impl(Color c, Square s) {
        Bitboard b = 0;
        int r = int(s) / 8, f = int(s) % 8;
        int dr = (c == WHITE) ? 1 : -1;
        for (int df : { -1, 1 }) {
            int nr = r + dr, nf = f + df;
            if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8)
                b |= Bitboard(1) << (nr * 8 + nf);
        }
        return b;
    }

    inline constexpr std::array<Bitboard, 64> make_knight_attacks() {
        std::array<Bitboard, 64> a{};
        for (int i = 0; i < 64; ++i) a[i] = knight_attacks_impl(Square(i));
        return a;
    }
    inline constexpr std::array<Bitboard, 64> make_king_attacks() {
        std::array<Bitboard, 64> a{};
        for (int i = 0; i < 64; ++i) a[i] = king_attacks_impl(Square(i));
        return a;
    }
    inline constexpr std::array<std::array<Bitboard, 64>, 2> make_pawn_attacks() {
        std::array<std::array<Bitboard, 64>, 2> a{};
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 64; ++i)
                a[c][i] = pawn_attacks_impl(Color(c), Square(i));
        return a;
    }
}

inline constexpr std::array<Bitboard, 64>              KNIGHT_ATTACKS = detail::make_knight_attacks();
inline constexpr std::array<Bitboard, 64>              KING_ATTACKS   = detail::make_king_attacks();
inline constexpr std::array<std::array<Bitboard,64>,2> PAWN_ATTACKS   = detail::make_pawn_attacks();

inline Bitboard knight_attacks_bb(Square s)         { return KNIGHT_ATTACKS[int(s)]; }
inline Bitboard king_attacks_bb(Square s)           { return KING_ATTACKS[int(s)]; }
inline Bitboard pawn_attacks_bb(Color c, Square s)  { return PAWN_ATTACKS[int(c)][int(s)]; }

// ----------------------------------------------------------------------------
// Lines and "between" squares — used for pinned-piece and check detection
// ----------------------------------------------------------------------------
namespace detail {
    inline constexpr Bitboard diagonal_bb(Square s) {
        Bitboard b = 0;
        int d = int(s) / 8 - int(s) % 8; // rank - file
        for (int r = 0; r < 8; ++r) {
            int f = r - d;
            if (f >= 0 && f < 8) b |= Bitboard(1) << (r * 8 + f);
        }
        return b;
    }
    inline constexpr Bitboard anti_diagonal_bb(Square s) {
        Bitboard b = 0;
        int d = int(s) / 8 + int(s) % 8; // rank + file
        for (int r = 0; r < 8; ++r) {
            int f = d - r;
            if (f >= 0 && f < 8) b |= Bitboard(1) << (r * 8 + f);
        }
        return b;
    }
    // full line through a and b (rank/file/diagonal), or 0 if not aligned
    inline constexpr Bitboard line_impl(Square a, Square b) {
        int ra = int(a) / 8, fa = int(a) % 8, rb = int(b) / 8, fb = int(b) % 8;
        if (fa == fb)                 return FILE_BBS[fa];          // same file
        if (ra == rb)                 return RANK_BBS[ra];          // same rank
        if (ra - fa == rb - fb)       return diagonal_bb(a);        // main diagonal
        if (ra + fa == rb + fb)       return anti_diagonal_bb(a);   // anti-diagonal
        return 0;
    }
    // squares strictly between a and b on their common line (empty if not aligned)
    inline constexpr Bitboard between_impl(Square a, Square b) {
        int ra = int(a) / 8, fa = int(a) % 8, rb = int(b) / 8, fb = int(b) % 8;
        if (!(fa == fb || ra == rb || ra - fa == rb - fb || ra + fa == rb + fb))
            return 0; // not aligned
        int dr = (rb > ra) - (rb < ra);
        int df = (fb > fa) - (fb < fa);
        Bitboard b_bb = 0;
        int r = ra + dr, f = fa + df;
        while (r != rb || f != fb) {
            b_bb |= Bitboard(1) << (r * 8 + f);
            r += dr; f += df;
        }
        return b_bb;
    }

    inline constexpr std::array<std::array<Bitboard, 64>, 64> make_line_bbs() {
        std::array<std::array<Bitboard, 64>, 64> a{};
        for (int i = 0; i < 64; ++i)
            for (int j = 0; j < 64; ++j)
                a[i][j] = line_impl(Square(i), Square(j));
        return a;
    }
    inline constexpr std::array<std::array<Bitboard, 64>, 64> make_between_bbs() {
        std::array<std::array<Bitboard, 64>, 64> a{};
        for (int i = 0; i < 64; ++i)
            for (int j = 0; j < 64; ++j)
                a[i][j] = between_impl(Square(i), Square(j));
        return a;
    }
}

inline constexpr std::array<std::array<Bitboard, 64>, 64> LINE_BBS    = detail::make_line_bbs();
inline constexpr std::array<std::array<Bitboard, 64>, 64> BETWEEN_BBS = detail::make_between_bbs();

inline Bitboard line_bb(Square a, Square b)    { return LINE_BBS[int(a)][int(b)]; }
inline Bitboard between_bb(Square a, Square b) { return BETWEEN_BBS[int(a)][int(b)]; }
inline bool     aligned(Square a, Square b, Square c) { return (line_bb(a, b) & square_bb(c)) != 0; }

// ----------------------------------------------------------------------------
// Slider attacks via Magic Bitboards (PEXT fast path if BMI2).
// Tables are filled at startup by init_bitboards() in bitboard.cpp.
// ----------------------------------------------------------------------------
struct Magic {
    Bitboard  mask;     // relevant occupancy squares for this slider+square
    Bitboard  magic;    // magic multiplier (unused when LOFTY_USE_PEXT)
    Bitboard* attacks;  // attack table, size = 2^popcount(mask)
    unsigned  shift;    // right-shift for magic index (unused when LOFTY_USE_PEXT)
};

extern Magic RookMagics[SQUARE_NB];
extern Magic BishopMagics[SQUARE_NB];

// Call once at program start (before any move generation).
void init_bitboards();

// Attack set for a piece type on `s` given occupancy `occ`.
// For pawns use pawn_attacks_bb(color, square) — color is required.
inline Bitboard attacks_bb(PieceType pt, Square s, Bitboard occ) {
    switch (pt) {
        case ROOK: {
            const Magic& m = RookMagics[int(s)];
            occ &= m.mask;
#if LOFTY_USE_PEXT
            return m.attacks[unsigned(_pext_u64(occ, m.mask))];
#else
            return m.attacks[unsigned((occ * m.magic) >> m.shift)];
#endif
        }
        case BISHOP: {
            const Magic& m = BishopMagics[int(s)];
            occ &= m.mask;
#if LOFTY_USE_PEXT
            return m.attacks[unsigned(_pext_u64(occ, m.mask))];
#else
            return m.attacks[unsigned((occ * m.magic) >> m.shift)];
#endif
        }
        case QUEEN:   return attacks_bb(ROOK, s, occ) | attacks_bb(BISHOP, s, occ);
        case KNIGHT:  return knight_attacks_bb(s);
        case KING:    return king_attacks_bb(s);
        default:      return 0;
    }
}

} // namespace lofty

#endif // LOFTY_BITBOARD_H
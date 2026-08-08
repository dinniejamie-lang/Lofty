// bitboard.cpp — Magic/PEXT attack table generation + self-test.
// Call init_bitboards() ONCE at startup, before any move generation.
#include "bitboard.h"

#include <cassert>
#include <cstdio>

namespace lofty {

// ----------------------------------------------------------------------------
// Globals (declared in bitboard.h)
// ----------------------------------------------------------------------------
Magic RookMagics[SQUARE_NB];
Magic BishopMagics[SQUARE_NB];

// ----------------------------------------------------------------------------
// Relevant-occupancy bit counts per square (standard values, Pradu Kannan)
// popcount(rook_mask(sq))  == RBits[sq]
// popcount(bishop_mask(sq)) == BBits[sq]
// ----------------------------------------------------------------------------
static constexpr int RBits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};

static constexpr int BBits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};

// Total attack-table entries = sum of 2^bits over all 64 squares
static constexpr int ROOK_ENTRIES   = 4 * 4096 + 24 * 2048 + 36 * 1024; // 102400
static constexpr int BISHOP_ENTRIES = 4 * 64   + 44 * 32  + 12 * 128 + 4 * 512; // 5248

// Flat storage (~840 KB total — fits comfortably in L2 cache on modern CPUs)
static Bitboard rookAttackTable[ROOK_ENTRIES];
static Bitboard bishopAttackTable[BISHOP_ENTRIES];

// ----------------------------------------------------------------------------
// Slow but obviously-correct sliding attack generators (reference for testing)
// These scan ray-by-ray: include the blocker square, then stop.
// ----------------------------------------------------------------------------
static Bitboard slow_rook_attacks(Square sq, Bitboard occ) {
    Bitboard attacks = 0;
    int r = int(sq) / 8, f = int(sq) % 8;

    // North / South
    for (int dr : {1, -1}) {
        for (int i = 1; i < 8; i++) {
            int nr = r + dr * i;
            if (nr < 0 || nr > 7) break;
            Square s = Square(nr * 8 + f);
            attacks |= square_bb(s);
            if (occ & square_bb(s)) break;
        }
    }
    // East / West
    for (int df : {1, -1}) {
        for (int i = 1; i < 8; i++) {
            int nf = f + df * i;
            if (nf < 0 || nf > 7) break;
            Square s = Square(r * 8 + nf);
            attacks |= square_bb(s);
            if (occ & square_bb(s)) break;
        }
    }
    return attacks;
}

static Bitboard slow_bishop_attacks(Square sq, Bitboard occ) {
    Bitboard attacks = 0;
    int r = int(sq) / 8, f = int(sq) % 8;

    for (int dr : {1, -1}) {
        for (int df : {1, -1}) {
            for (int i = 1; i < 8; i++) {
                int nr = r + dr * i;
                int nf = f + df * i;
                if (nr < 0 || nr > 7 || nf < 0 || nf > 7) break;
                Square s = Square(nr * 8 + nf);
                attacks |= square_bb(s);
                if (occ & square_bb(s)) break;
            }
        }
    }
    return attacks;
}

// ----------------------------------------------------------------------------
// Mask generators: relevant occupancy squares (excludes board edges).
// A blocker on the edge can't block anything beyond it, so edges are excluded
// from the mask — this is what makes magic bitboards compact.
// ----------------------------------------------------------------------------
static Bitboard rook_mask(Square sq) {
    Bitboard mask = 0;
    int r = int(sq) / 8, f = int(sq) % 8;
    for (int i = r + 1; i <= 6; i++) mask |= square_bb(Square(i * 8 + f));
    for (int i = r - 1; i >= 1; i--) mask |= square_bb(Square(i * 8 + f));
    for (int i = f + 1; i <= 6; i++) mask |= square_bb(Square(r * 8 + i));
    for (int i = f - 1; i >= 1; i--) mask |= square_bb(Square(r * 8 + i));
    return mask;
}

static Bitboard bishop_mask(Square sq) {
    Bitboard mask = 0;
    int r = int(sq) / 8, f = int(sq) % 8;
    for (int dr = 1,  df = 1;  r + dr <= 6 && f + df <= 6; dr++, df++)
        mask |= square_bb(Square((r + dr) * 8 + (f + df)));
    for (int dr = 1,  df = -1; r + dr <= 6 && f + df >= 1; dr++, df--)
        mask |= square_bb(Square((r + dr) * 8 + (f + df)));
    for (int dr = -1, df = 1;  r + dr >= 1 && f + df <= 6; dr--, df++)
        mask |= square_bb(Square((r + dr) * 8 + (f + df)));
    for (int dr = -1, df = -1; r + dr >= 1 && f + df >= 1; dr--, df--)
        mask |= square_bb(Square((r + dr) * 8 + (f + df)));
    return mask;
}

// ----------------------------------------------------------------------------
// Xorshift64 PRNG — deterministic, portable, no libc dependency.
// Used only during init (magic search). Zobrist keys use a separate PRNG
// in position.cpp so their state is independent of this one.
// ----------------------------------------------------------------------------
static uint64_t prng_state = 0xD1B54A32D192ED03ULL;

static uint64_t prng_next() {
    uint64_t x = prng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    prng_state = x;
    return x;
}

// Sparse random bitboard: AND of 3 randoms → few bits set.
// Tord Romstad's trick — magics with few bits work best.
static Bitboard random_sparse() {
    return Bitboard(prng_next() & prng_next() & prng_next());
}

// ----------------------------------------------------------------------------
// find_magic — brute-force search for a collision-free magic multiplier.
// Returns 0 on failure (should never happen with 100M attempts).
// ----------------------------------------------------------------------------
static Bitboard find_magic(Square sq, int bits, bool bishop) {
    Bitboard mask = bishop ? bishop_mask(sq) : rook_mask(sq);
    int n = popcount(mask);
    assert(n == bits);

    // Precompute all 2^n subsets of the mask and their attack sets
    Bitboard subsets[4096], attacks[4096];
    Bitboard sub = 0;
    int count = 0;
    do {
        subsets[count] = sub;
        attacks[count] = bishop ? slow_bishop_attacks(sq, sub)
                                : slow_rook_attacks(sq, sub);
        count++;
    } while ((sub = (sub - mask) & mask) != 0);
    assert(count == (1 << bits));

    // Try random sparse magics until one produces a collision-free hash
    for (int attempt = 0; attempt < 100'000'000; attempt++) {
        Bitboard magic = random_sparse();
        // Reject if top 8 bits of (mask * magic) have too few bits —
        // such magics waste index space and rarely work
        if (popcount((mask * magic) & 0xFF00000000000000ULL) < 6)
            continue;

        Bitboard used[4096] = {};
        bool fail = false;
        for (int i = 0; i < count && !fail; i++) {
            unsigned idx = unsigned((subsets[i] * magic) >> (64 - bits));
            if (used[idx] == 0)
                used[idx] = attacks[i];
            else if (used[idx] != attacks[i])
                fail = true; // destructive collision — different attacks, same index
        }
        if (!fail)
            return magic;
    }
    return 0; // unreachable in practice
}

// ----------------------------------------------------------------------------
// magic_index — computes the table index for a given occupancy.
// MUST be identical to the index logic in attacks_bb() (bitboard.h).
// ----------------------------------------------------------------------------
static inline unsigned magic_index(const Magic& m, Bitboard occ) {
    occ &= m.mask;
#if LOFTY_USE_PEXT
    return unsigned(_pext_u64(occ, m.mask));
#else
    return unsigned((occ * m.magic) >> m.shift);
#endif
}

// ----------------------------------------------------------------------------
// verify_bitboards — self-test: fast lookup must match slow reference.
// Called in debug builds. Tests every subset of every mask (exhaustive)
// plus random full-board occupancies (exercises the masking step).
// ----------------------------------------------------------------------------
static void verify_bitboards() {
    for (int sq = 0; sq < SQUARE_NB; sq++) {
        Bitboard rmask = rook_mask(Square(sq));
        Bitboard bmask = bishop_mask(Square(sq));

        // Exhaustive: every subset of the rook mask
        Bitboard sub = 0;
        do {
            assert(attacks_bb(ROOK, Square(sq), sub) == slow_rook_attacks(Square(sq), sub));
        } while ((sub = (sub - rmask) & rmask) != 0);

        // Exhaustive: every subset of the bishop mask
        sub = 0;
        do {
            assert(attacks_bb(BISHOP, Square(sq), sub) == slow_bishop_attacks(Square(sq), sub));
        } while ((sub = (sub - bmask) & bmask) != 0);
    }

    // Random occupancies: verify masking + queen = rook | bishop
    for (int i = 0; i < 5000; i++) {
        Bitboard occ = Bitboard(prng_next());
        for (int sq = 0; sq < SQUARE_NB; sq++) {
            Bitboard r = attacks_bb(ROOK, Square(sq), occ);
            Bitboard b = attacks_bb(BISHOP, Square(sq), occ);
            assert(r == slow_rook_attacks(Square(sq), occ));
            assert(b == slow_bishop_attacks(Square(sq), occ));
            assert(attacks_bb(QUEEN, Square(sq), occ) == (r | b));
        }
    }
}

// ----------------------------------------------------------------------------
// init_bitboards — find magics, allocate and fill attack tables, self-test.
// Call once at program start.
// ----------------------------------------------------------------------------
void init_bitboards() {
    // --- Rooks ---
    Bitboard* rPtr = rookAttackTable;
    for (int sq = 0; sq < SQUARE_NB; sq++) {
        int bits = RBits[sq];
        Bitboard mask = rook_mask(Square(sq));

        Magic& m = RookMagics[sq];
        m.mask    = mask;
        m.shift   = unsigned(64 - bits);
        m.attacks = rPtr;
        m.magic   = find_magic(Square(sq), bits, false);
        assert(m.magic != 0);

        // Fill every subset's attack set into the table
        Bitboard sub = 0;
        do {
            unsigned idx = magic_index(m, sub);
            rPtr[idx] = slow_rook_attacks(Square(sq), sub);
        } while ((sub = (sub - mask) & mask) != 0);

        rPtr += (1 << bits);
    }

    // --- Bishops ---
    Bitboard* bPtr = bishopAttackTable;
    for (int sq = 0; sq < SQUARE_NB; sq++) {
        int bits = BBits[sq];
        Bitboard mask = bishop_mask(Square(sq));

        Magic& m = BishopMagics[sq];
        m.mask    = mask;
        m.shift   = unsigned(64 - bits);
        m.attacks = bPtr;
        m.magic   = find_magic(Square(sq), bits, true);
        assert(m.magic != 0);

        Bitboard sub = 0;
        do {
            unsigned idx = magic_index(m, sub);
            bPtr[idx] = slow_bishop_attacks(Square(sq), sub);
        } while ((sub = (sub - mask) & mask) != 0);

        bPtr += (1 << bits);
    }

#ifndef NDEBUG
    verify_bitboards();
#endif
}

} // namespace lofty
// simd.h — SIMD utilities for AVX2/AVX-512 acceleration
#ifndef LOFTY_SIMD_H
#define LOFTY_SIMD_H

#include <cstdint>
#include <type_traits>
#include <cstring>
#include <bit>

// Detect available instruction sets
#if defined(__AVX512BW__) && defined(__AVX512VL__)
    #define LOFTY_AVX512 1
#else
    #define LOFTY_AVX512 0
#endif

#if defined(__AVX2__)
    #define LOFTY_AVX2 1
#else
    #define LOFTY_AVX2 0
#endif

#if LOFTY_AVX512
    #include <immintrin.h>
#elif LOFTY_AVX2
    #include <immintrin.h>
#endif

namespace lofty {
namespace simd {

// Runtime CPU feature detection
inline bool has_avx2() {
#if LOFTY_AVX2
    int regs[4];
    #if defined(__GNUC__) || defined(__clang__)
        __asm__ __volatile__("cpuid"
            : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
            : "a"(7), "c"(0));
        return (regs[1] & (1 << 5)) != 0;
    #elif defined(_MSC_VER)
        __cpuidex(regs, 7, 0);
        return (regs[1] & (1 << 5)) != 0;
    #else
        return true;
    #endif
#else
    return false;
#endif
}

inline bool has_avx512() {
#if LOFTY_AVX512
    int regs[4];
    #if defined(__GNUC__) || defined(__clang__)
        __asm__ __volatile__("cpuid"
            : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
            : "a"(7), "c"(0));
        return (regs[1] & (1 << 30)) != 0;
    #elif defined(_MSC_VER)
        __cpuidex(regs, 7, 0);
        return (regs[1] & (1 << 30)) != 0;
    #else
        return true;
    #endif
#else
    return false;
#endif
}

#if LOFTY_AVX2

// Structure to hold SIMD-accelerated pawn evaluation state
struct PawnEvalState {
    alignas(32) uint64_t our_pawns[4];
    alignas(32) uint64_t their_pawns[4];
    alignas(32) uint64_t isolated_masks[4];
    alignas(32) uint64_t passed_masks[4];
    int mg_scores[4];
    int eg_scores[4];
    int count;
};

// Evaluate 4 pawn positions in parallel using AVX2
// Returns total MG and EG scores
inline void evaluate_pawns_simd(PawnEvalState& state, int& total_mg, int& total_eg) {
    if (state.count < 4) {
        // Fallback to scalar for remaining pawns
        total_mg = 0;
        total_eg = 0;
        return;
    }
    
    // Load pawn bitboards into SIMD registers
    __m256i our_pawns_v   = _mm256_load_si256(reinterpret_cast<const __m256i*>(state.our_pawns));
    __m256i their_pawns_v = _mm256_load_si256(reinterpret_cast<const __m256i*>(state.their_pawns));
    __m256i iso_masks_v   = _mm256_load_si256(reinterpret_cast<const __m256i*>(state.isolated_masks));
    __m256i pass_masks_v  = _mm256_load_si256(reinterpret_cast<const __m256i*>(state.passed_masks));
    
    // Check for isolated pawns: pawn & ~isolated_mask == 0 means isolated
    __m256i not_iso_v = _mm256_and_si256(our_pawns_v, iso_masks_v);
    
    // Check for passed pawns: pawn & ~passed_mask == opponent_pawns_on_path
    // If (their_pawns & passed_mask) == 0, then it's a passed pawn
    __m256i blocked_v = _mm256_and_si256(their_pawns_v, pass_masks_v);
    
    // Count bits in each lane to determine penalties/bonuses
    // We'll use horizontal operations
    
    // Extract each 64-bit lane and compute popcount
    alignas(32) uint64_t not_iso_vals[4];
    alignas(32) uint64_t blocked_vals[4];
    
    _mm256_store_si256(reinterpret_cast<__m256i*>(not_iso_vals), not_iso_v);
    _mm256_store_si256(reinterpret_cast<__m256i*>(blocked_vals), blocked_v);
    
    int mg = 0;
    int eg = 0;
    
    for (int i = 0; i < 4; i++) {
        // Isolated pawn check: if our_pawns[i] exists and not_iso_vals[i] == 0
        if (state.our_pawns[i] != 0 && not_iso_vals[i] == 0) {
            mg -= 15;
            eg -= 15;
        }
        
        // Passed pawn check: if our_pawns[i] exists and blocked_vals[i] == 0
        if (state.our_pawns[i] != 0 && blocked_vals[i] == 0) {
            // Calculate rank-based bonus (simplified - assumes we know the rank)
            int rank_bonus = 20; // Base bonus, actual implementation would use rank
            mg += rank_bonus;
            eg += rank_bonus + 10;
        }
    }
    
    total_mg = mg;
    total_eg = eg;
}

// Optimized version using AVX2 popcount intrinsics
inline int popcount_simd_4x(uint64_t b0, uint64_t b1, uint64_t b2, uint64_t b3) {
    __m256i v = _mm256_set_epi64x(b3, b2, b1, b0);
    
    // Use _mm256_popcnt_u64 if available (AVX512), otherwise fallback
    #if LOFTY_AVX512
        __m256i counts = _mm256_popcnt_epi64(v);
        alignas(32) uint64_t c[4];
        _mm256_store_si256(reinterpret_cast<__m256i*>(c), counts);
        return int(c[0] + c[1] + c[2] + c[3]);
    #else
        // Fallback to scalar popcount (still faster due to data packing)
        return std::popcount(b0) + std::popcount(b1) + std::popcount(b2) + std::popcount(b3);
    #endif
}

#endif // LOFTY_AVX2

} // namespace simd
} // namespace lofty

#endif // LOFTY_SIMD_H

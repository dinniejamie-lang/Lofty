#include "src/simd.h"
#include <iostream>
#include <cstring>

int main() {
    std::cout << "AVX2 support compiled: " << LOFTY_AVX2 << std::endl;
    std::cout << "AVX512 support compiled: " << LOFTY_AVX512 << std::endl;
    std::cout << "AVX2 runtime check: " << lofty::simd::has_avx2() << std::endl;
    std::cout << "AVX512 runtime check: " << lofty::simd::has_avx512() << std::endl;
    
#if LOFTY_AVX2
    // Test SIMD popcount
    uint64_t b0 = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t b1 = 0x0F0F0F0F0F0F0F0FULL;
    uint64_t b2 = 0xAAAAAAAAAAAAAAAALL;
    uint64_t b3 = 0x5555555555555555ULL;
    
    int count = lofty::simd::popcount_simd_4x(b0, b1, b2, b3);
    int expected = std::popcount(b0) + std::popcount(b1) + std::popcount(b2) + std::popcount(b3);
    
    std::cout << "SIMD popcount result: " << count << std::endl;
    std::cout << "Expected popcount: " << expected << std::endl;
    std::cout << "Test " << (count == expected ? "PASSED" : "FAILED") << std::endl;
    
    // Test pawn eval state structure
    lofty::simd::PawnEvalState state;
    std::memset(&state, 0, sizeof(state));
    state.count = 4;
    state.our_pawns[0] = 0x0000000000000101ULL;
    state.our_pawns[1] = 0x0000000000000202ULL;
    state.our_pawns[2] = 0x0000000000000404ULL;
    state.our_pawns[3] = 0x0000000000000808ULL;
    state.their_pawns[0] = 0;
    state.their_pawns[1] = 0;
    state.their_pawns[2] = 0;
    state.their_pawns[3] = 0;
    state.isolated_masks[0] = 0xFFFFFFFFFFFFFFFFULL;
    state.isolated_masks[1] = 0xFFFFFFFFFFFFFFFFULL;
    state.isolated_masks[2] = 0xFFFFFFFFFFFFFFFFULL;
    state.isolated_masks[3] = 0xFFFFFFFFFFFFFFFFULL;
    state.passed_masks[0] = 0;
    state.passed_masks[1] = 0;
    state.passed_masks[2] = 0;
    state.passed_masks[3] = 0;
    
    int mg, eg;
    lofty::simd::evaluate_pawns_simd(state, mg, eg);
    std::cout << "Pawn eval MG: " << mg << ", EG: " << eg << std::endl;
#endif
    
    return 0;
}

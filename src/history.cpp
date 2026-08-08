// history.cpp — History Heuristic implementation.
#include "history.h"
#include <cstring>

namespace lofty {

History Hist;
ContinuationHistory ContHist;

void History::clear() {
    for (auto& row : table) row.fill(0);
}

void History::new_search() {
    for (auto& row : table) {
        for (auto& val : row) val >>= 1; // Decay by half
    }
}

void History::update(Move m, Depth depth) {
    int bonus = int(depth) * int(depth);
    int& current = table[m.from()][m.to()];
    current += bonus;
    if (current > MAX_HISTORY) current = MAX_HISTORY;
}

// --- ContinuationHistory ---
void ContinuationHistory::clear() {
    std::memset(table, 0, sizeof(table));
}

void ContinuationHistory::new_search() {
    // Decay all values by half.
    for (int a = 0; a < PIECE_NB; ++a)
        for (int b = 0; b < SQUARE_NB; ++b)
            for (int c = 0; c < PIECE_NB; ++c)
                for (int d = 0; d < SQUARE_NB; ++d)
                    table[a][b][c][d] >>= 1;
}

void ContinuationHistory::update(Piece p, Square to, Piece prevP, Square prevTo, int bonus) {
    if (prevP == NO_PIECE) return;
    
    // Promote to int to prevent overflow during addition/clamping
    int v = table[prevP][prevTo][p][to] + bonus;
    if (v > 32767) v = 32767;
    if (v < -32768) v = -32768;
    table[prevP][prevTo][p][to] = static_cast<int16_t>(v);
}

} // namespace lofty
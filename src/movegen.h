// movegen.h — move list type + pseudo-legal / legal move generation interface.
// Depends on: types.h (Position is forward-declared).
#ifndef LOFTY_MOVEGEN_H
#define LOFTY_MOVEGEN_H

#include "types.h"
#include <array>

namespace lofty {

class Position;

// Maximum moves in any chess position is 218 (proven bound). 256 gives slack
// and aligns to a clean power of two for stack-friendly allocation.
inline constexpr int MAX_MOVES = 256;

// MoveList — fixed-capacity stack array. No heap allocation anywhere.
class MoveList {
    std::array<Move, MAX_MOVES> moves_;
    int size_;
public:
    MoveList() : size_(0) {}

    void clear() { size_ = 0; }
    void add(Move m) { moves_[size_++] = m; }
    void set_size(int n) { size_ = n; }

    int  size()  const { return size_; }
    bool empty() const { return size_ == 0; }

    Move  operator[](int i) const { return moves_[i]; }
    Move& operator[](int i)       { return moves_[i]; }

    const Move* begin() const { return moves_.data(); }
    const Move* end()   const { return moves_.data() + size_; }
};

// generate_pseudo_legal — all moves legal except for leaving own king in check.
int generate_pseudo_legal(const Position& pos, MoveList& list);

// generate_legal — pseudo-legal moves filtered by is_legal().
int generate_legal(const Position& pos, MoveList& list);

} // namespace lofty

#endif // LOFTY_MOVEGEN_H
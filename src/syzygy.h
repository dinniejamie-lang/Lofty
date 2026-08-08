// syzygy.h — Syzygy Tablebase Prober Interface
#ifndef LOFTY_SYZYGY_H
#define LOFTY_SYZYGY_H

#include <string>

namespace lofty {

class Position;

enum TBScore {
    TB_FAILED = -1,
    TB_DRAW = 0,
    TB_WIN = 1,
    TB_LOSS = 2,
    TB_BLESSED_LOSS = 3,
    TB_CURSED_WIN = 4
};

void init_tb(const std::string& paths);
void free_tb();
int tb_max_cardinality();
TBScore probe_wdl(const Position& pos);
int probe_dtz(const Position& pos);

} // namespace lofty

#endif // LOFTY_SYZYGY_H
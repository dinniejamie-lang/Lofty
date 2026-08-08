// syzygy.cpp — Stub implementation (Tablebases disabled for now)
#include "syzygy.h"

namespace lofty {

void init_tb(const std::string& paths) { (void)paths; }
void free_tb() {}
int tb_max_cardinality() { return 0; }
TBScore probe_wdl(const Position&) { return TB_FAILED; }
int probe_dtz(const Position&) { return 0; }

} // namespace lofty
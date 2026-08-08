// uci.h — Universal Chess Interface loop and parsing.
// Depends on: types.h, position.h, search.h
#ifndef LOFTY_UCI_H
#define LOFTY_UCI_H

#include "types.h"
#include "position.h"
#include "search.h"

namespace lofty {

// uci_loop — main blocking loop that reads std::cin and dispatches commands.
void uci_loop();

} // namespace lofty

#endif // LOFTY_UCI_H
// flashdepth.h — Search Stability Analyzer & Flash Depth Trigger.
#ifndef LOFTY_FLASHDEPTH_H
#define LOFTY_FLASHDEPTH_H

#include "types.h"

namespace lofty {

class FlashDepth {
private:
    Move lastBestMove;
    Value lastScore;
    int stabilityCount;
    bool flashMode;

public:
    FlashDepth();

    // Called at the end of each Iterative Deepening depth by Thread 0
    void update(Move bestMove, Value score);

    // Returns true if the search has been stable for multiple depths
    bool in_flash_mode() const;

    // Returns the recommended aspiration window size based on stability
    int get_window_size() const;
};

} // namespace lofty

#endif // LOFTY_FLASHDEPTH_H
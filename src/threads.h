// threads.h — Thread Pool and Lazy SMP interface.
#ifndef LOFTY_THREADS_H
#define LOFTY_THREADS_H

#include "types.h"
#include "position.h"
#include "search.h"

#include <vector>
#include <memory>
#include <thread>
#include <atomic>

namespace lofty {

class Thread {
public:
    std::thread nativeThread;
    size_t id = 0;
    SearchResult result;

    Thread(size_t n) : id(n) {}
    
    // The main search loop for this specific thread
    void search(Position pos, SearchLimits limits);
};

class ThreadPool {
    std::vector<std::unique_ptr<Thread>> threads;
    std::atomic<bool> stopFlag{false};

public:
    ThreadPool() = default;

    // set — configures the number of active threads.
    void set(size_t n);

    // start — launches all threads to begin searching the position.
    void start(Position pos, SearchLimits limits);

    // wait — blocks the main thread until all search threads finish.
    void wait();

    // stop — signals all threads to abort immediately.
    void stop() { stopFlag = true; }

    // best_result — collects votes from all threads and returns the best move.
    SearchResult best_result() const;

    size_t size() const { return threads.size(); }
};

extern ThreadPool Threads; // Global thread pool instance

} // namespace lofty

#endif // LOFTY_THREADS_H
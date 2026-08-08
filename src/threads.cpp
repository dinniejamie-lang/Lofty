// threads.cpp — Thread Pool implementation (Lazy SMP).
#include "threads.h"
#include "timeman.h"

namespace lofty {

ThreadPool Threads;

// Declare the thread_local ID from search.cpp so we can set it here
extern thread_local size_t ThreadID;

void Thread::search(Position pos, SearchLimits limits) {
    ThreadID = id; // Assign the thread ID so search.cpp knows who is running
    result = lofty::search(pos, limits);
}

void ThreadPool::set(size_t n) {
    // Wait for any existing threads to finish before resizing
    wait();
    
    threads.clear();
    if (n == 0) n = 1; // Always have at least 1 thread
    
    for (size_t i = 0; i < n; ++i) {
        threads.emplace_back(std::make_unique<Thread>(i));
    }
}

void ThreadPool::start(Position pos, SearchLimits limits) {
    stopFlag = false;
    
    if (threads.empty()) {
        set(1);
    }

    // LAZY SMP: Launch ALL threads in the pool!
    for (size_t i = 0; i < threads.size(); ++i) {
        // Pass pos and limits by value so the thread owns its own copy
        threads[i]->nativeThread = std::thread(&Thread::search, threads[i].get(), pos, limits);
    }
}

void ThreadPool::wait() {
    for (auto& t : threads) {
        if (t->nativeThread.joinable()) {
            t->nativeThread.join();
        }
    }
}

SearchResult ThreadPool::best_result() const {
    if (threads.empty()) return SearchResult{};

    // DEPTH-PRIORITIZED VOTING:
    // We must find the thread that reached the highest completedDepth.
    // We completely ignore threads that stopped at a shallower depth, 
    // even if their score is higher, to prevent shallow-eval greed.
    
    SearchResult best = threads[0]->result;
    
    for (size_t i = 1; i < threads.size(); ++i) {
        SearchResult current = threads[i]->result;
        
        if (current.completedDepth > best.completedDepth) {
            best = current; // This thread searched deeper, it wins.
        } 
        else if (current.completedDepth == best.completedDepth) {
            // Tiebreaker: If depths are equal, pick the better score.
            if (current.score > best.score) {
                best = current;
            }
        }
    }
    
    return best;
}

} // namespace lofty
// main.cpp — Entry point, initialization, and perft driver.
#include "bitboard.h"
#include "position.h"
#include "movegen.h"
#include "eval.h"
#include "uci.h"
#include "tt.h"
#include "history.h"
#include "timeman.h"
#include "ucioption.h"
#include "threads.h"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>

namespace lofty {

// ----------------------------------------------------------------------------
// Perft Driver — fast copy-make perft for verifying move generation.
// ----------------------------------------------------------------------------
uint64_t perft(Position pos, int depth) {
    if (depth == 0) return 1;

    MoveList list;
    generate_pseudo_legal(pos, list);
    
    Color us = pos.side_to_move();
    uint64_t nodes = 0;

    for (int i = 0; i < list.size(); ++i) {
        Position next = pos; // copy-make
        next.make_move(list[i]);
        
        // If the move leaves our king in check, it's illegal. Skip it.
        if (next.in_check(us)) continue;
        
        nodes += (depth == 1) ? 1 : perft(next, depth - 1);
    }
    
    return nodes;
}

// ----------------------------------------------------------------------------
// run_perft_tests — validates move generation against the 6 standard positions.
// ----------------------------------------------------------------------------
void run_perft_tests() {
    struct Test {
        std::string fen;
        std::vector<uint64_t> expected; // depths 1, 2, 3, 4, 5
    };

    std::vector<Test> tests = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", {20, 400, 8902, 197281, 4865609}},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", {48, 2039, 97862, 4085603, 193690690}},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", {14, 191, 2812, 43238, 674624}},
        {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", {6, 264, 9467, 422333, 15833292}},
        {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", {44, 1486, 62379, 2103487, 89941194}},
        {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", {46, 2079, 89890, 3894594, 164075551}}
    };

    std::cout << "Running perft tests...\n";
    std::cout << "====================================================\n";
    
    for (size_t i = 0; i < tests.size(); ++i) {
        Position pos;
        if (!pos.set_fen(tests[i].fen)) {
            std::cout << "Test " << (i + 1) << ": FEN PARSE ERROR!\n";
            continue;
        }
        
        std::cout << "Test " << (i + 1) << ": " << tests[i].fen << "\n";
        bool allPassed = true;
        
        for (size_t d = 0; d < tests[i].expected.size(); ++d) {
            int depth = int(d + 1);
            auto start = std::chrono::high_resolution_clock::now();
            uint64_t nodes = perft(pos, depth);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            
            bool passed = (nodes == tests[i].expected[d]);
            if (!passed) allPassed = false;
            
            std::cout << "  Depth " << depth << ": " << nodes 
                      << " (Expected: " << tests[i].expected[d] << ") "
                      << (passed ? "[PASS]" : "[FAIL]") 
                      << " Time: " << elapsed.count() << "s\n";
        }
        std::cout << (allPassed ? ">>> TEST PASSED <<<" : ">>> TEST FAILED <<<") << "\n";
        std::cout << "----------------------------------------------------\n";
    }
}

} // namespace lofty

// ----------------------------------------------------------------------------
// main — initializes all engine subsystems and starts the UCI loop.
// ----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    // Speed up I/O
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // Initialize all engine subsystems
    lofty::init_bitboards();
    lofty::Zobrist::init();
    lofty::init_eval();
    lofty::init_search();
    
    // Initialize UCI Options (this automatically allocates the default 64MB TT)
    lofty::init_options(lofty::Options);
    lofty::Hist.clear();

    // Initialize the Thread Pool with 1 thread by default
    lofty::Threads.set(1);

    // If run with "perft" argument, execute tests and exit
    if (argc > 1 && std::string(argv[1]) == "perft") {
        lofty::run_perft_tests();
        return 0;
    }

    // Otherwise, start the UCI loop
    lofty::uci_loop();

    return 0;
}
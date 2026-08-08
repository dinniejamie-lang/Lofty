// uci.cpp — UCI protocol implementation.
// Uses a watcher thread to manage the ThreadPool and output bestmove.
#include "uci.h"
#include "ucioption.h"
#include "timeman.h"
#include "movegen.h"
#include "bitboard.h"
#include "search.h"
#include "tt.h"
#include "history.h"
#include "threads.h"

#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace lofty {

static std::string move_to_uci(Move m) {
    if (m == MOVE_NONE) return "0000";
    std::string s;
    s += char('a' + file_of(m.from()));
    s += char('1' + rank_of(m.from()));
    s += char('a' + file_of(m.to()));
    s += char('1' + rank_of(m.to()));
    if (m.is_promotion()) {
        switch (m.promo()) {
            case QUEEN:  s += 'q'; break;
            case ROOK:   s += 'r'; break;
            case BISHOP: s += 'b'; break;
            case KNIGHT: s += 'n'; break;
            default: break;
        }
    }
    return s;
}

static Move parse_move(const Position& pos, const std::string& str) {
    if (str.length() < 4) return MOVE_NONE;
    
    File ff = File(str[0] - 'a');
    Rank fr = Rank(str[1] - '1');
    File tf = File(str[2] - 'a');
    Rank tr = Rank(str[3] - '1');
    
    if (ff < FILE_A || ff > FILE_H || fr < RANK_1 || fr > RANK_8 ||
        tf < FILE_A || tf > FILE_H || tr < RANK_1 || tr > RANK_8) {
        return MOVE_NONE;
    }

    Square from = make_square(ff, fr);
    Square to   = make_square(tf, tr);

    MoveList list;
    generate_pseudo_legal(pos, list);
    
    for (int i = 0; i < list.size(); ++i) {
        Move m = list[i];
        if (m.from() == from && m.to() == to) {
            if (m.is_promotion()) {
                if (str.length() == 5) {
                    char promo = str[4];
                    PieceType pt = NO_PIECE_TYPE;
                    if (promo == 'q') pt = QUEEN;
                    else if (promo == 'r') pt = ROOK;
                    else if (promo == 'b') pt = BISHOP;
                    else if (promo == 'n') pt = KNIGHT;
                    
                    if (pt != NO_PIECE_TYPE && m.promo() == pt) {
                        return m;
                    }
                }
            } else {
                return m;
            }
        }
    }
    return MOVE_NONE;
}

static std::thread searchThread;

static void handle_position(Position& pos, std::istringstream& iss) {
    if (searchThread.joinable()) {
        Time.stop();
        searchThread.join();
    }

    std::string token;
    std::string fen = "";
    
    iss >> token;
    if (token == "startpos") {
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        iss >> token; 
    } else if (token == "fen") {
        while (iss >> token && token != "moves") {
            fen += token + " ";
        }
    }

    if (!fen.empty()) {
        pos.set_fen(fen);
    }

    while (iss >> token) {
        Move m = parse_move(pos, token);
        if (m != MOVE_NONE) {
            pos.make_move(m);
        }
    }
}

static void handle_go(Position& pos, std::istringstream& iss) {
    if (searchThread.joinable()) {
        Time.stop();
        searchThread.join();
    }

    SearchLimits limits;
    std::string token;
    
    int64_t wtime = 0, btime = 0, winc = 0, binc = 0, movestogo = 0;
    
    while (iss >> token) {
        if (token == "wtime") iss >> wtime;
        else if (token == "btime") iss >> btime;
        else if (token == "winc") iss >> winc;
        else if (token == "binc") iss >> binc;
        else if (token == "movestogo") iss >> movestogo;
        else if (token == "depth") iss >> limits.maxDepth;
        else if (token == "nodes") iss >> limits.maxNodes;
        else if (token == "movetime") { iss >> limits.maxTimeMs; limits.maxDepth = 64; }
        else if (token == "infinite") { limits.maxTimeMs = 0; limits.maxDepth = 64; limits.maxNodes = 0; }
    }

    if (limits.maxTimeMs > 0) {
        Time.init(limits.maxTimeMs, limits.maxTimeMs, 0, 0, 1, pos.side_to_move());
    } else {
        Time.init(wtime, btime, winc, binc, movestogo, pos.side_to_move());
    }
    
    Time.start();

    searchThread = std::thread([&pos, limits]() {
        Threads.start(pos, limits);
        Threads.wait();
        
        SearchResult res = Threads.best_result();
        std::cout << "bestmove " << move_to_uci(res.bestMove) << std::endl;
    });
}

void uci_loop() {
    Position pos;
    pos.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "uci") {
            // FIXED: Updated name and author for Lofty 1.0
            std::cout << "id name Lofty 1.0\n";
            std::cout << "id author Nutty-Games (Very cool guy)\n";
            Options.print();
            std::cout << "uciok" << std::endl;
        } 
        else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        } 
        else if (token == "ucinewgame") {
            if (searchThread.joinable()) {
                Time.stop();
                searchThread.join();
            }
            TT.clear();
            Hist.clear();
            pos.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        } 
        else if (token == "position") {
            handle_position(pos, iss);
        } 
        else if (token == "go") {
            handle_go(pos, iss);
        } 
        else if (token == "setoption") {
            std::string nameToken, name, valueToken, value;
            iss >> nameToken;
            while (iss >> nameToken && nameToken != "value") {
                if (!name.empty()) name += " ";
                name += nameToken;
            }
            while (iss >> valueToken) {
                if (!value.empty()) value += " ";
                value += valueToken;
            }
            Options.set(name, value);
            
            if (name == "Threads") {
                Threads.set(Options.get("Threads").as_int());
            }
        }
        else if (token == "stop") {
            Time.stop();
            if (searchThread.joinable()) searchThread.join();
        } 
        else if (token == "quit") {
            Time.stop();
            if (searchThread.joinable()) searchThread.join();
            break;
        }
    }
}

} // namespace lofty

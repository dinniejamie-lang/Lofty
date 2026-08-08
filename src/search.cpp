// search.cpp — Hyper-Aggressive PVS with ProbCut, Hyper-LMR, Node Limits, & Depth Tracking.
#include "search.h"
#include "movegen.h"
#include "movepicker.h"
#include "eval.h"
#include "bitboard.h"
#include "tt.h"
#include "history.h"
#include "timeman.h"
#include "see.h"
#include "syzygy.h"
#include "ucioption.h"
#include "flashdepth.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <atomic>

namespace lofty {

constexpr int MAX_PLY = 128;

struct SearchStack {
    Killers killers;
    int staticEval;
    Move currentMove;
    Piece currentPiece;
};

static thread_local SearchStack searchStacks[MAX_PLY];
thread_local size_t ThreadID = 0;

static std::atomic<bool> stopSearch(false);
static std::atomic<int64_t> nodeCount(0);
static std::atomic<int> maxSelDepth(0);
static std::atomic<int64_t> maxNodes(0);

static int lmrTable[64][64];

void init_search() {
    for (int d = 0; d < 64; ++d) {
        for (int m = 0; m < 64; ++m) {
            lmrTable[d][m] = int(1.5 * std::log(std::max(d, 1)) * std::log(std::max(m, 1)));
        }
    }
}

static inline Value score_to_tt(Value v, int ply) {
    if (v > VALUE_MATE_IN_MAX_PLY) return v + ply;
    if (v < -VALUE_MATE_IN_MAX_PLY) return v - ply;
    return v;
}

static inline Value score_from_tt(Value v, int ply) {
    if (v > VALUE_MATE_IN_MAX_PLY) return v - ply;
    if (v < -VALUE_MATE_IN_MAX_PLY) return v + ply;
    return v;
}

static Value qsearch(Position& pos, Value alpha, Value beta, int ply, SearchStack* ss, const SearchStack* prevSs) {
    if (stopSearch.load(std::memory_order_relaxed)) return VALUE_ZERO;
    nodeCount.fetch_add(1, std::memory_order_relaxed);

    int currentSel = maxSelDepth.load(std::memory_order_relaxed);
    while (ply > currentSel) {
        if (maxSelDepth.compare_exchange_weak(currentSel, ply, std::memory_order_relaxed)) break;
    }

    if (ply >= MAX_PLY) return evaluate(pos);
    if (pos.halfmove_clock() >= 100) return VALUE_DRAW;

    bool inChk = pos.in_check();
    Value stand_pat = VALUE_ZERO;

    if (!inChk) {
        stand_pat = evaluate(pos);
        if (stand_pat >= beta) return stand_pat;
        if (stand_pat > alpha) alpha = stand_pat;
    } else {
        alpha = -VALUE_INFINITE;
    }

    Move ttMove = TT.probe_move(pos.key());
    MovePicker mp(pos, ttMove, MOVE_NONE, MOVE_NONE, 
                  (prevSs ? prevSs->currentPiece : NO_PIECE), 
                  (prevSs ? prevSs->currentMove : MOVE_NONE));

    int legalMoves = 0;
    Move m;
    while ((m = mp.next_move()) != MOVE_NONE) {
        if (!inChk && !m.is_capture()) continue;
        if (!inChk && !see_ge(pos, m, -100)) continue;

        if (!inChk) {
            PieceType victim = m.is_ep() ? PAWN : type_of(pos.piece_on(m.to()));
            if (stand_pat + 200 + victim * 100 < alpha) continue;
        }

        Color us = pos.side_to_move();
        Position next = pos;
        next.make_move(m);
        if (next.in_check(us)) continue;

        legalMoves++;
        ss->currentMove = m;
        ss->currentPiece = pos.piece_on(m.from());
        
        Value score = -qsearch(next, -beta, -alpha, ply + 1, ss + 1, ss);

        if (score >= beta) return score;
        if (score > alpha) alpha = score;
    }

    if (inChk && legalMoves == 0) return -VALUE_MATE + ply;
    return alpha;
}

static Value search(Position& pos, Depth depth, Value alpha, Value beta, int ply, bool pvNode, Move excludedMove = MOVE_NONE) {
    if (stopSearch.load(std::memory_order_relaxed)) return VALUE_ZERO;
    
    if ((nodeCount.load(std::memory_order_relaxed) & 2047) == 0) {
        if (Time.check_time()) stopSearch.store(true, std::memory_order_relaxed);
        int64_t limit = maxNodes.load(std::memory_order_relaxed);
        if (limit > 0 && nodeCount.load(std::memory_order_relaxed) >= limit) {
            stopSearch.store(true, std::memory_order_relaxed);
        }
    }
    nodeCount.fetch_add(1, std::memory_order_relaxed);

    int currentSel = maxSelDepth.load(std::memory_order_relaxed);
    while (ply > currentSel) {
        if (maxSelDepth.compare_exchange_weak(currentSel, ply, std::memory_order_relaxed)) break;
    }

    if (ply >= MAX_PLY) return evaluate(pos);

    bool inChk = pos.in_check();
    if (inChk) depth++;

    if (depth <= 0) return qsearch(pos, alpha, beta, ply, &searchStacks[ply], ply > 0 ? &searchStacks[ply-1] : nullptr);

    if (pos.halfmove_clock() >= 100 && !inChk) return VALUE_DRAW;

    if (ply > 0 && excludedMove == MOVE_NONE) {
        int tbLimit = Options.get("SyzygyProbeLimit").as_int();
        if (tbLimit > 0 && popcount(pos.pieces()) <= tbLimit && popcount(pos.pieces()) <= tb_max_cardinality()) {
            TBScore wdl = probe_wdl(pos);
            if (wdl != TB_FAILED) {
                if (wdl == TB_WIN) return VALUE_MATE - 1000 - ply;
                if (wdl == TB_LOSS) return -VALUE_MATE + 1000 + ply;
                return VALUE_DRAW;
            }
        }
    }

    SearchStack* ss = &searchStacks[ply];
    const SearchStack* prevSs = (ply > 0) ? &searchStacks[ply-1] : nullptr;
    ss->currentMove = MOVE_NONE;
    ss->currentPiece = NO_PIECE;

    TTEntry tte;
    bool ttHit = TT.probe(pos.key(), tte);
    Move ttMove = ttHit ? Move(tte.move) : MOVE_NONE;
    Value ttScore = ttHit ? score_from_tt(Value(tte.score), ply) : VALUE_ZERO;

    if (excludedMove == MOVE_NONE && ttHit && !pvNode && Depth(tte.depth) >= depth) {
        if (tte.bound == BOUND_EXACT) return ttScore;
        if (tte.bound == BOUND_LOWER && ttScore >= beta) return ttScore;
        if (tte.bound == BOUND_UPPER && ttScore <= alpha) return ttScore;
    }

    ss->staticEval = inChk ? VALUE_ZERO : evaluate(pos);

    // --- PROBCUT ---
    if (!pvNode && !inChk && depth >= 5 && std::abs(int(beta)) < VALUE_MATE_IN_MAX_PLY) {
        Value rBeta = std::min(beta + 150, VALUE_INFINITE - 1);
        if (ss->staticEval >= rBeta) {
            Value s = -search(pos, depth - 4, -rBeta, -rBeta + 1, ply + 1, false);
            if (s >= rBeta) return s;
        }
    }

    if (!pvNode && !inChk && depth <= 3 && ss->staticEval + 150 * depth < alpha) {
        if (depth == 1) return qsearch(pos, alpha, beta, ply, ss, prevSs);
        Value s = qsearch(pos, alpha, beta, ply, ss, prevSs);
        if (s < alpha) return s;
    }

    if (!pvNode && !inChk && depth <= 5 && ss->staticEval - 100 * depth >= beta) {
        return ss->staticEval;
    }

    bool hasNonPawnMaterial = (pos.pieces(pos.side_to_move()) ^ pos.pieces(pos.side_to_move(), PAWN) ^ pos.pieces(pos.side_to_move(), KING)) != 0;
    if (!pvNode && !inChk && depth >= 3 && ss->staticEval >= beta && hasNonPawnMaterial && excludedMove == MOVE_NONE) {
        Position nullPos = pos;
        nullPos.do_null_move();
        int R = 3 + depth / 4 + (ss->staticEval - beta) / 200;
        Value nullScore = -search(nullPos, depth - 1 - R, -beta, -beta + 1, ply + 1, false);
        if (nullScore >= beta) return nullScore > VALUE_MATE_IN_MAX_PLY ? beta : nullScore;
    }

    bool singular = false;
    bool pruneNonSingular = false;
    if (excludedMove == MOVE_NONE && ttMove != MOVE_NONE && depth >= 8 && tte.depth >= depth - 3 && tte.bound != BOUND_UPPER) {
        Value rBeta = ttScore - depth;
        Value s = -search(pos, depth / 2 - 1, -rBeta - 1, -rBeta, ply + 1, false, ttMove);
        if (s < rBeta) {
            singular = true;
            if (s < rBeta - (2 * depth)) {
                pruneNonSingular = true;
            }
        }
    }

    MovePicker mp(pos, ttMove, ss->killers.moves[0], ss->killers.moves[1], 
                  (prevSs ? prevSs->currentPiece : NO_PIECE), 
                  (prevSs ? prevSs->currentMove : MOVE_NONE), excludedMove);

    Value bestScore = -VALUE_INFINITE;
    Move bestMove = MOVE_NONE;
    int legalMoves = 0;
    Value oldAlpha = alpha;

    Move m;
    while ((m = mp.next_move()) != MOVE_NONE) {
        Color us = pos.side_to_move();
        Position next = pos;
        next.make_move(m);
        if (next.in_check(us)) continue;

        legalMoves++;
        bool isCapture = m.is_capture();
        bool isPromotion = m.is_promotion();
        bool isQuiet = !isCapture && !isPromotion;
        bool givesCheck = next.in_check(~us);

        if (pruneNonSingular && isQuiet && m != ttMove) {
            continue; 
        }

        if (!pvNode && !inChk && depth == 1 && isQuiet && legalMoves > 1) {
            if (ss->staticEval + 200 <= alpha) continue;
        }

        if (!pvNode && !inChk && depth <= 3 && isQuiet && legalMoves > (3 + depth * 3)) {
            continue;
        }

        if (!pvNode && !inChk && depth <= 5 && !see_ge(pos, m, -50 * depth)) {
            continue;
        }

        int extension = 0;
        if (ply < MAX_PLY - 32) {
            if (givesCheck) extension = 1;                                  
            else if (isPromotion) extension = 1;                            
            if (m == ttMove && singular) extension = std::max(extension, 1); 
        }

        Value score = -VALUE_INFINITE;
        bool fullDepthSearch = false;

        ss->currentMove = m;
        ss->currentPiece = pos.piece_on(m.from());

        if (depth >= 3 && legalMoves > 3 && isQuiet) {
            int r = lmrTable[std::min(int(depth), 63)][std::min(legalMoves, 63)];
            
            if (pvNode) r--;
            if (givesCheck) r--;
            if (ss->killers.moves[0] != m && ss->killers.moves[1] != m) r += 2;
            
            if (prevSs != nullptr && prevSs->currentPiece != NO_PIECE) {
                int chScore = ContHist.score(ss->currentPiece, m.to(), prevSs->currentPiece, prevSs->currentMove.to());
                r -= chScore / 8192;
            }
            
            r = std::max(0, std::min(r, int(depth) - 2));
            score = -search(next, depth - 1 - r + extension, -alpha - 1, -alpha, ply + 1, false);
            if (score > alpha) fullDepthSearch = true;
        } else {
            fullDepthSearch = !pvNode || legalMoves > 1;
        }

        if (fullDepthSearch) {
            score = -search(next, depth - 1 + extension, -alpha - 1, -alpha, ply + 1, false);
        }

        if (pvNode && (legalMoves == 1 || score > alpha)) {
            score = -search(next, depth - 1 + extension, -beta, -alpha, ply + 1, true);
        }

        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
            if (score > alpha) alpha = score;
        }

        if (alpha >= beta) {
            if (isQuiet) {
                ss->killers.update(m);
                Hist.update(m, depth);
                if (prevSs != nullptr && prevSs->currentPiece != NO_PIECE) {
                    ContHist.update(ss->currentPiece, m.to(), prevSs->currentPiece, prevSs->currentMove.to(), int(depth) * int(depth));
                }
            }
            break;
        }
    }

    if (legalMoves == 0) {
        if (inChk) return -VALUE_MATE + ply;
        return VALUE_DRAW;
    }

    if (excludedMove == MOVE_NONE) {
        Bound bound = bestScore >= beta ? BOUND_LOWER : (alpha > oldAlpha ? BOUND_EXACT : BOUND_UPPER);
        TT.store(pos.key(), depth, bound, bestScore, bestMove, ss->staticEval, ply);
    }

    return bestScore;
}

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

static std::string extract_pv(Position pos, Move rootMove, int maxDepth) {
    std::string pv;
    if (rootMove == MOVE_NONE) return "";
    
    Move m = rootMove;
    int ply = 0;
    
    while (ply < maxDepth) {
        MoveList list;
        generate_pseudo_legal(pos, list);
        bool found = false;
        for (int i = 0; i < list.size(); ++i) {
            if (list[i] == m) { found = true; break; }
        }
        if (!found) break;
        
        Color us = pos.side_to_move();
        Position next = pos;
        next.make_move(m);
        if (next.in_check(us)) break;
        
        if (!pv.empty()) pv += " ";
        pv += move_to_uci(m);
        
        pos = next;
        ply++;
        
        TTEntry tte;
        if (!TT.probe(pos.key(), tte) || tte.move == 0) break;
        m = Move(tte.move);
        
        if (std::abs(int(tte.score)) > VALUE_MATE - 1000) break; 
    }
    return pv;
}

SearchResult search(Position& pos, const SearchLimits& limits) {
    SearchResult result;
    FlashDepth flashAnalyzer;
    
    for (int i = 0; i < MAX_PLY; ++i) {
        searchStacks[i] = SearchStack{};
    }
    
    if (ThreadID == 0) {
        Hist.new_search();
        ContHist.new_search();
        TT.new_search();
        nodeCount.store(0, std::memory_order_relaxed);
        maxSelDepth.store(0, std::memory_order_relaxed);
        maxNodes.store(limits.maxNodes, std::memory_order_relaxed);
    }

    stopSearch.store(false, std::memory_order_relaxed);

    int tbLimit = Options.get("SyzygyProbeLimit").as_int();
    if (tbLimit > 0 && popcount(pos.pieces()) <= tbLimit && popcount(pos.pieces()) <= tb_max_cardinality()) {
        MoveList rootList;
        generate_pseudo_legal(pos, rootList);
        
        Value tbBestScore = -VALUE_INFINITE;
        Move tbBestMove = MOVE_NONE;
        bool tbProbed = false;

        for (int i = 0; i < rootList.size(); ++i) {
            Move m = rootList[i];
            Color us = pos.side_to_move();
            Position next = pos;
            next.make_move(m);
            if (next.in_check(us)) continue;

            TBScore wdl = probe_wdl(next);
            if (wdl == TB_FAILED) continue;

            tbProbed = true;
            Value score;
            if (wdl == TB_LOSS) { 
                int dtz = std::abs(probe_dtz(next));
                score = VALUE_MATE - 1000 - dtz;
            } else if (wdl == TB_WIN) { 
                int dtz = std::abs(probe_dtz(next));
                score = -VALUE_MATE + 1000 + dtz;
            } else {
                score = VALUE_DRAW;
            }

            if (score > tbBestScore) {
                tbBestScore = score;
                tbBestMove = m;
            }
        }

        if (tbProbed) {
            result.score = tbBestScore;
            result.bestMove = tbBestMove;
            if (ThreadID == 0) {
                std::cout << "info depth 0 score cp " << result.score << " pv " << move_to_uci(result.bestMove) << std::endl;
            }
            return result;
        }
    }

    MoveList pseudoRootList;
    generate_pseudo_legal(pos, pseudoRootList);
    MoveList legalRootMoves;
    for (int i = 0; i < pseudoRootList.size(); ++i) {
        Move m = pseudoRootList[i];
        Color us = pos.side_to_move();
        Position next = pos;
        next.make_move(m);
        if (!next.in_check(us)) {
            legalRootMoves.add(m);
        }
    }

    if (legalRootMoves.size() == 0) {
        if (pos.in_check()) result.score = -VALUE_MATE;
        else result.score = VALUE_DRAW;
        return result;
    }

    for (int depth = 1; depth <= limits.maxDepth; ++depth) {
        if (ThreadID == 0 && depth > 1) {
            int64_t optTime = Time.get_optimal_time();
            if (optTime > 0 && Time.elapsed() >= optTime) {
                stopSearch.store(true, std::memory_order_relaxed);
                Time.stop();
            }
            int64_t limit = maxNodes.load(std::memory_order_relaxed);
            if (limit > 0 && nodeCount.load(std::memory_order_relaxed) >= limit) {
                stopSearch.store(true, std::memory_order_relaxed);
                Time.stop();
            }
        }
        
        if (stopSearch.load(std::memory_order_relaxed)) break;

        Value delta = flashAnalyzer.get_window_size();
        Value alpha = -VALUE_INFINITE;
        Value beta = VALUE_INFINITE;

        if (depth > 4) {
            alpha = std::max(result.score - delta, -VALUE_INFINITE);
            beta  = std::min(result.score + delta, VALUE_INFINITE);
        }

        while (true) {
            Value score = -VALUE_INFINITE;
            Move bestMove = result.bestMove;

            Move ttMoveForThread = result.bestMove;
            if (ThreadID != 0 && legalRootMoves.size() > 1) {
                Move smpMove = legalRootMoves[ThreadID % legalRootMoves.size()];
                if (smpMove != result.bestMove) {
                    ttMoveForThread = smpMove;
                }
            }

            MovePicker mp(pos, ttMoveForThread, MOVE_NONE, MOVE_NONE, NO_PIECE, MOVE_NONE);
            Move m;
            int legalMoves = 0;

            while ((m = mp.next_move()) != MOVE_NONE) {
                Color us = pos.side_to_move();
                Position next = pos;
                next.make_move(m);
                if (next.in_check(us)) continue;

                legalMoves++;
                Value currentScore;

                if (legalMoves == 1) {
                    currentScore = -search(next, depth - 1, -beta, -alpha, 1, true);
                } else {
                    currentScore = -search(next, depth - 1, -alpha - 1, -alpha, 1, false);
                    if (currentScore > alpha && currentScore < beta) {
                        currentScore = -search(next, depth - 1, -beta, -alpha, 1, true);
                    }
                }

                if (stopSearch.load(std::memory_order_relaxed)) break;

                if (currentScore > score) {
                    score = currentScore;
                    bestMove = m;
                    if (score > alpha) alpha = score;
                }

                if (alpha >= beta) break;
            }

            if (stopSearch.load(std::memory_order_relaxed)) {
                if (result.bestMove == MOVE_NONE && bestMove != MOVE_NONE) {
                    result.bestMove = bestMove;
                }
                break;
            }

            if (score <= alpha - delta) { 
                alpha = std::max(alpha - delta, -VALUE_INFINITE);
                delta *= 2;
                continue;
            } 
            if (score >= beta) { 
                beta = std::min(beta + delta, VALUE_INFINITE);
                delta *= 2; 
                result.score = score;
                result.bestMove = bestMove;
                continue;
            }

            result.score = score;
            result.bestMove = bestMove;
            break;
        }

        if (stopSearch.load(std::memory_order_relaxed)) break;

        // NEW: Record the successfully completed depth for SMP voting
        result.completedDepth = depth;

        flashAnalyzer.update(result.bestMove, result.score);

        if (ThreadID == 0) {
            auto elapsed = Time.elapsed();
            std::string pv = extract_pv(pos, result.bestMove, depth);
            int selDepth = maxSelDepth.load(std::memory_order_relaxed);
            
            std::cout << "info depth " << depth
                      << " seldepth " << selDepth
                      << " score cp " << result.score
                      << " nodes " << nodeCount.load(std::memory_order_relaxed)
                      << " time " << elapsed
                      << " pv " << pv << std::endl;
        }
    }

    result.selDepth = maxSelDepth.load(std::memory_order_relaxed);
    return result;
}

} // namespace lofty
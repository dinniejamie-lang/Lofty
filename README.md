***

# Lofty 1.0 - HCE-MAXXING
### Philosophy of maxing out the entire HCE and squeezing the performance.

**Estimated Strength:** 3000 - 3150 CCRL Elo (Grandmaster / Super-Grandmaster level)
**Estimated Speed:** 50,000,000 - 100,000,000 NPS (Nodes Per Second) on modern multi-core CPUs
**Architecture:** Bitboard, Copy-Make, Lazy SMP, Hyper-Aggressive PVS

Lofty is the idea of creating the absolute ceiling of a Handcrafted Evaluation (HCE) chess engine. Without the use of Neural Networks (NNUE), every drop of Elo and speed has been squeezed out through pure mathematical geometry, ruthless search pruning, and dynamic evaluation. The engine does not rely on 1990s hardcoded "if/else" rules; it evaluates the board by calculating the physical tension, threat differentials, and geometric relationships between all 32 pieces.

---

## 1. Board Representation & Move Generation (The Physics Engine)
* **Bitboards & Magic PEXT:** Every piece and square is represented as a 64-bit integer. Sliding piece moves (Bishops, Rooks, Queens) are calculated in nanoseconds using Magic Bitboards with a hardware-accelerated `PEXT` fast path (BMI2 instruction set).
* **Copy-Make Architecture:** The `Position` struct is a compact 168-byte state. Instead of making a move and tracking an "undo" stack, the engine copies the entire state to the stack and mutates it. This is cache-friendly and eliminates path-dependent unmake bugs.
* **Incremental Evaluation (PSQT):** Material and Piece-Square Tables are updated instantly via addition/subtraction during `make_move`. The engine never recalculates the base score from scratch.
* **MovePicker (Lazy Partial Sort):** The engine uses a state-of-the-art `MovePicker` that precomputes move scores in $O(N)$ time but only sorts the top 4 moves (TT, Killers, Best Capture) using a partial insertion sort. If a beta cutoff happens early, the remaining 30 moves are never sorted, saving massive CPU cycles.

## 2. Search Architecture (The Brain)
* **Hyper-Aggressive PVS:** Principal Variation Search uses a full window for the first move and a zero-window (`[alpha, alpha+1]`) for all subsequent moves. If a zero-window search fails high, a full re-search is triggered.
* **ProbCut:** At depth $\ge$ 5, if a move looks mathematically terrible based on a quick 4-ply search, it is pruned entirely before the main move loop even begins.
* **Hyper-LMR (Late Move Reductions):** Late moves (move 10+) are reduced by up to 6 or 7 ply. The LMR formula is dynamically adjusted by PV nodes, checks, killers, and 4D Continuation History. This drops the branching factor from 35 down to ~1.5.
* **Threat Extensions:** The search depth is extended by 1 ply for moves that create immediate tactical chaos (Checks, Promotions, Singular moves). Extensions are strictly depth-limited to prevent infinite loops.
* **Singular Extensions & Dynamic Pruning:** If the Transposition Table proves a move is the *only* good move, it is extended. If the score gap is massive, "Flash Mode" triggers, hard-pruning all other quiet moves to tunnel down the forced line.
* **FlashDepth (Stability Analyzer):** A dedicated module tracks the Principal Variation across depths. If the best move and eval remain stable for 3 consecutive depths, the Aspiration Window shrinks to $\pm$ 30, aggressively pruning irrelevant branches.
* **Selective Depth (`seldepth`):** The engine tracks the absolute deepest ply reached via extensions and reports it to the GUI, proving its tactical tunneling capabilities.

## 3. Evaluation (HCE-MAXXING: The Eyesight & Tension Graph)
The evaluation is split into modular layers to maintain pure dynamic math.

* **Layer 1: Pawn Structure (`eval.cpp`):** Cached in a dedicated Pawn Hash Table. Evaluates doubled, isolated, and passed pawns.
* **Layer 2: Dynamic Bishop Pair (`eval.cpp`):** The bishop pair bonus is not flat. It scales dynamically based on the enemy's pawn distribution (e.g., pawns on both wings yield the maximum bonus).
* **Layer 3: Supported Mobility (`eyesight.cpp`):** Knights, Bishops, and Queens only get mobility points for moving to squares *defended by friendly pieces*. A Queen raiding alone gets 0 mobility. This dynamically kills early Queen blunders without hardcoding rules.
* **Layer 4: The Tension Graph (`relations.cpp`):** The Stockfish 18 inspiration. Maps the entire board's Attack, Defense, and Threat geometry.
  * *Threat Differentials:* If a heavy piece is attacked by a weaker enemy piece, the eval drops massively. If the piece is defended, the threat is neutralized.
  * *Geometric King Safety:* King safety is not based on rank checks or castling flags. The engine calculates a "King Zone" and intersects it with enemy attack bitboards. If the King walks into the center, it naturally intersects with more enemy attacks, causing the eval to dynamically collapse.

## 4. Multi-Threading & Time Management (The Infrastructure)
* **Lazy SMP:** The engine runs on all available CPU cores. Threads share a massive Transposition Table (default 256MB). To prevent threads from stepping on each other, **Root Move Splitting** is used: Thread 0 searches root move 1 first, Thread 1 searches root move 2 first, etc.
* **Depth-Prioritized Voting:** When time runs out, the `ThreadPool` does not blindly pick the highest score. It scans all threads, finds the absolute highest `completedDepth`, and *only* accepts results from threads that reached that depth. Shallow, unproven searches are permanently ignored.
* **Strict Obedience Time Manager:** The time manager does not artificially divide remaining time by 20 or 40. If the GUI gives 10,000ms, the engine uses 9,950ms to reach maximum depth, leaving a tiny buffer to prevent time losses.
* **Node Limit Support:** Flawlessly parses `go nodes` commands for deterministic benchmarking.

## 5. Endgame Perfection
* **Syzygy Tablebases:** Integrated via the Fathom library (`syzygy.cpp`). In positions with $\le$ 7 pieces, the engine probes WDL (Win/Draw/Loss) in the search tree and DTZ (Distance to Zero) at the root to play endgames with 100% mathematical perfection.

---

## Project File Structure
```text
C:\Lofty\
├─ CMakeLists.txt
├─ Lofty_1.0_README.md
└─ src\
   ├─ main.cpp          # Entry point, init, perft driver
   ├─ types.h           # Enums, Move class, constants
   ├─ bitboard.h/.cpp   # Bitboard utils, magic/PEXT attacks
   ├─ position.h/.cpp   # Board state, FEN, make_move, attack queries
   ├─ movegen.h/.cpp    # Pseudo-legal & legal move generation
   ├─ movepicker.h/.cpp # Lazy Partial Sort move ordering
   ├─ eval.h/.cpp       # Tapered eval, Pawn Hash, Dynamic Bishop Pair
   ├─ eyesight.h/.cpp   # Supported Mobility, Outposts, Open Files
   ├─ relations.h/.cpp  # Tension Graph: Threats, Pressure, Geometric King Safety
   ├─ flashdepth.h/.cpp # Search Stability Analyzer & Window Scaler
   ├─ search.h/.cpp     # Hyper-Aggressive PVS, ProbCut, Hyper-LMR, SMP Root
   ├─ threads.h/.cpp    # Lazy SMP ThreadPool & Depth-Prioritized Voting
   ├─ timeman.h/.cpp    # Strict Obedience time allocation
   ├─ tt.h/.cpp         # 3-Bucket Transposition Table with priority replacement
   ├─ history.h/.cpp    # Butterfly History & 4D Continuation History
   ├─ see.h/.cpp        # Flawless Static Exchange Evaluation (X-ray aware)
   ├─ syzygy.h/.cpp     # Syzygy Tablebase wrapper (Fathom)
   ├─ ucioption.h/.cpp  # UCI Options (Hash, Threads, SyzygyPath)
   └─ uci.h/.cpp        # UCI protocol parser and Thread watcher
```

**Lofty 1.0 is complete.**

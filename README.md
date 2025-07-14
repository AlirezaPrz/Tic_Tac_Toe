# Tic_Tac_Toe

## Overview  
This bot plays on a 10×10 board aiming to get five in a row (horizontal, vertical, or diagonal). It uses:

1. **Immediate tactics**  
   - **Win scan**: checks every empty cell—if placing there yields five in a row, play it.  
   - **Block scan**: checks opponent’s potential wins and blocks them.

2. **Strategic search**  
   - **Iterative deepening minimax** with **alpha‑beta pruning** (depth ≤ 15).  
   - **Move generation** limited to empty cells within a 2‑cell “neighborhood” of existing pieces (or center on an empty board) to reduce branching.  
   - **Time control**: aborts deeper search via a `TimeOutException` after 4.8 s, falling back to the best completed depth.  

3. **Heuristic evaluation**  
   - Scores board patterns (open/closed runs of length 1–4) for both players.  
   - Treats a confirmed win (+∞) or loss (–∞) as terminal.

---

## Files in this package

- `bot`  
  The compiled single‐file executable for Ubuntu 24.04.

- `bot.cpp`  
  Full C++17 source code implementing the strategy above.

- `json.hpp`  
  nlohmann/json single‑header library for parsing `state.json`.

- `README.md`  
  This file.

---

## Build Instructions

Ensure you have a C++17 compiler installed. In bot directory, run:

```bash
g++ -std=c++17 -O2 -o bot bot.cpp
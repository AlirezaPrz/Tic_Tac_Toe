#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <stdexcept>
#include "json.hpp"

#pragma GCC optimize("Ofast")

using json = nlohmann::json;
static const int BOARD_SIZE = 10;
static char boardArr[BOARD_SIZE][BOARD_SIZE];
static char myPlayer, oppPlayer;
static const int INF = 1000000000;  // large value for win/loss
// Heuristic weights for patterns
static const int OPEN_FOUR   = 100000;
static const int CLOSED_FOUR = 10000;
static const int OPEN_THREE  = 5000;
static const int CLOSED_THREE= 1000;
static const int OPEN_TWO    = 500;
static const int CLOSED_TWO  = 100;
static const int ONE_PIECE   = 10;

static std::chrono::steady_clock::time_point startTime;
static std::chrono::milliseconds timeLimit(4800); // 4.8 seconds limit (safe margin)

struct TimeOutException : public std::exception {};

// Return list of empty positions on the board
std::vector<std::pair<int,int>> get_valid_moves(const json& board) {
    std::vector<std::pair<int,int>> moves;
    int n = board.size();
    for(int i=0;i<n;++i){
        for(int j=0;j<(int)board[i].size();++j){
            if(board[i][j].get<std::string>().empty()){
                moves.emplace_back(i,j);
            }
        }
    }
    return moves;
}

// Check if placing `symbol` at (r,c) yields five in a row (win condition)
bool checkFiveInRow(int r, int c, char symbol) {
    int directions[4][2] = {{0,1},{1,0},{1,1},{1,-1}};
    for (auto& d : directions) {
        int dr = d[0], dc = d[1];
        int count = 1;
        // check backward
        int rr = r - dr, cc = c - dc;
        while (rr >= 0 && rr < BOARD_SIZE && cc >= 0 && cc < BOARD_SIZE 
               && boardArr[rr][cc] == symbol) {
            count++;
            rr -= dr; cc -= dc;
        }
        // check forward
        rr = r + dr; cc = c + dc;
        while (rr >= 0 && rr < BOARD_SIZE && cc >= 0 && cc < BOARD_SIZE 
               && boardArr[rr][cc] == symbol) {
            count++;
            rr += dr; cc += dc;
        }
        if (count >= 5) return true;
    }
    return false;
}

// Static evaluation of the current board from the perspective of `myPlayer`
int evaluateBoard() {
    int score = 0;
    // Check all lines (horizontal, vertical, diagonals) for patterns
    // Horizontal lines
    for (int i = 0; i < BOARD_SIZE; i++) {
        int j = 0;
        while (j < BOARD_SIZE) {
            if (boardArr[i][j] != '.') {
                char sym = boardArr[i][j];
                // start of a contiguous sequence of sym
                if (j == 0 || boardArr[i][j-1] != sym) {
                    int k = j;
                    while (k < BOARD_SIZE && boardArr[i][k] == sym) {
                        k++;
                    }
                    int length = k - j;
                    bool leftOpen = (j-1 >= 0 && boardArr[i][j-1] == '.');
                    bool rightOpen = (k < BOARD_SIZE && boardArr[i][k] == '.');
                    if (length >= 5) {
                        // Already a five-in-a-row on board
                        return (sym == myPlayer ? INF : -INF);
                    }
                    if (leftOpen || rightOpen) {  // at least one end is open
                        if (sym == myPlayer) {
                            if (leftOpen && rightOpen) {  // open sequence
                                switch(length) {
                                    case 4: score += OPEN_FOUR; break;
                                    case 3: score += OPEN_THREE; break;
                                    case 2: score += OPEN_TWO; break;
                                    case 1: score += ONE_PIECE; break;
                                }
                            } else {  // closed sequence (one end blocked)
                                switch(length) {
                                    case 4: score += CLOSED_FOUR; break;
                                    case 3: score += CLOSED_THREE; break;
                                    case 2: score += CLOSED_TWO; break;
                                    case 1: score += 1; break;
                                }
                            }
                        } else {  // opponent's sequence
                            if (leftOpen && rightOpen) {
                                switch(length) {
                                    case 4: score -= OPEN_FOUR; break;
                                    case 3: score -= OPEN_THREE; break;
                                    case 2: score -= OPEN_TWO; break;
                                    case 1: score -= ONE_PIECE; break;
                                }
                            } else {
                                switch(length) {
                                    case 4: score -= CLOSED_FOUR; break;
                                    case 3: score -= CLOSED_THREE; break;
                                    case 2: score -= CLOSED_TWO; break;
                                    case 1: score -= 1; break;
                                }
                            }
                        }
                    }
                    j = k;
                    continue;
                }
            }
            j++;
        }
    }
    // Vertical lines
    for (int j = 0; j < BOARD_SIZE; j++) {
        int i = 0;
        while (i < BOARD_SIZE) {
            if (boardArr[i][j] != '.') {
                char sym = boardArr[i][j];
                if (i == 0 || boardArr[i-1][j] != sym) {
                    int k = i;
                    while (k < BOARD_SIZE && boardArr[k][j] == sym) {
                        k++;
                    }
                    int length = k - i;
                    bool leftOpen = (i-1 >= 0 && boardArr[i-1][j] == '.');
                    bool rightOpen = (k < BOARD_SIZE && boardArr[k][j] == '.');
                    if (length >= 5) {
                        return (sym == myPlayer ? INF : -INF);
                    }
                    if (leftOpen || rightOpen) {
                        if (sym == myPlayer) {
                            if (leftOpen && rightOpen) {
                                switch(length) {
                                    case 4: score += OPEN_FOUR; break;
                                    case 3: score += OPEN_THREE; break;
                                    case 2: score += OPEN_TWO; break;
                                    case 1: score += ONE_PIECE; break;
                                }
                            } else {
                                switch(length) {
                                    case 4: score += CLOSED_FOUR; break;
                                    case 3: score += CLOSED_THREE; break;
                                    case 2: score += CLOSED_TWO; break;
                                    case 1: score += 1; break;
                                }
                            }
                        } else {
                            if (leftOpen && rightOpen) {
                                switch(length) {
                                    case 4: score -= OPEN_FOUR; break;
                                    case 3: score -= OPEN_THREE; break;
                                    case 2: score -= OPEN_TWO; break;
                                    case 1: score -= ONE_PIECE; break;
                                }
                            } else {
                                switch(length) {
                                    case 4: score -= CLOSED_FOUR; break;
                                    case 3: score -= CLOSED_THREE; break;
                                    case 2: score -= CLOSED_TWO; break;
                                    case 1: score -= 1; break;
                                }
                            }
                        }
                    }
                    i = k;
                    continue;
                }
            }
            i++;
        }
    }
    // Diagonal (down-right) lines
    for (int startRow = 0; startRow < BOARD_SIZE; startRow++) {
        // diagonals starting at each cell of first column and first row
        int i = startRow, j = 0;
        // (Also handle separately starting from first row below)
        while (i < BOARD_SIZE && j < BOARD_SIZE) {
            if (boardArr[i][j] != '.') {
                char sym = boardArr[i][j];
                if (i == startRow || j == 0 || boardArr[i-1][j-1] != sym) {
                    int ii = i, jj = j;
                    while (ii < BOARD_SIZE && jj < BOARD_SIZE && boardArr[ii][jj] == sym) {
                        ii++; jj++;
                    }
                    int length = ii - i;
                    bool leftOpen = (i-1 >= 0 && j-1 >= 0 && boardArr[i-1][j-1] == '.');
                    bool rightOpen = (ii < BOARD_SIZE && jj < BOARD_SIZE && boardArr[ii][jj] == '.');
                    if (length >= 5) {
                        return (sym == myPlayer ? INF : -INF);
                    }
                    if (leftOpen || rightOpen) {
                        if (sym == myPlayer) {
                            if (leftOpen && rightOpen) {
                                switch(length) {
                                    case 4: score += OPEN_FOUR; break;
                                    case 3: score += OPEN_THREE; break;
                                    case 2: score += OPEN_TWO; break;
                                    case 1: score += ONE_PIECE; break;
                                }
                            } else {
                                switch(length) {
                                    case 4: score += CLOSED_FOUR; break;
                                    case 3: score += CLOSED_THREE; break;
                                    case 2: score += CLOSED_TWO; break;
                                    case 1: score += 1; break;
                                }
                            }
                        } else {
                            if (leftOpen && rightOpen) {
                                switch(length) {
                                    case 4: score -= OPEN_FOUR; break;
                                    case 3: score -= OPEN_THREE; break;
                                    case 2: score -= OPEN_TWO; break;
                                    case 1: score -= ONE_PIECE; break;
                                }
                            } else {
                                switch(length) {
                                    case 4: score -= CLOSED_FOUR; break;
                                    case 3: score -= CLOSED_THREE; break;
                                    case 2: score -= CLOSED_TWO; break;
                                    case 1: score -= 1; break;
                                }
                            }
                        }
                    }
                    i = ii; j = jj;
                    continue;
                }
            }
            i++; j++;
        }
    }
    for (int startCol = 1; startCol < BOARD_SIZE; ++startCol) {
        // diagonals starting at each cell of first row (except [0,0] which was covered)
        int i = 0, j = startCol;
        while (i < BOARD_SIZE && j < BOARD_SIZE) {
            if (boardArr[i][j] != '.') {
                char sym = boardArr[i][j];
                if (i == 0 || j == startCol || boardArr[i-1][j-1] != sym) {
                    int ii = i, jj = j;
                    while (ii < BOARD_SIZE && jj < BOARD_SIZE && boardArr[ii][jj] == sym) {
                        ii++; jj++;
                    }
                    int length = ii - i;
                    bool leftOpen = (i-1 >= 0 && j-1 >= 0 && boardArr[i-1][j-1] == '.');
                    bool rightOpen = (ii < BOARD_SIZE && jj < BOARD_SIZE && boardArr[ii][jj] == '.');
                    if (length >= 5) {
                        return (sym == myPlayer ? INF : -INF);
                    }
                    if (leftOpen || rightOpen) {
                        if (sym == myPlayer) {
                            if (leftOpen && rightOpen) {
                                switch(length) {
                                    case 4: score += OPEN_FOUR; break;
                                    case 3: score += OPEN_THREE; break;
                                    case 2: score += OPEN_TWO; break;
                                    case 1: score += ONE_PIECE; break;
                                }
                            } else {
                                switch(length) {
                                    case 4: score += CLOSED_FOUR; break;
                                    case 3: score += CLOSED_THREE; break;
                                    case 2: score += CLOSED_TWO; break;
                                    case 1: score += 1; break;
                                }
                            }
                        } else {
                            if (leftOpen && rightOpen) {
                                switch(length) {
                                    case 4: score -= OPEN_FOUR; break;
                                    case 3: score -= OPEN_THREE; break;
                                    case 2: score -= OPEN_TWO; break;
                                    case 1: score -= ONE_PIECE; break;
                                }
                            } else {
                                switch(length) {
                                    case 4: score -= CLOSED_FOUR; break;
                                    case 3: score -= CLOSED_THREE; break;
                                    case 2: score -= CLOSED_TWO; break;
                                    case 1: score -= 1; break;
                                }
                            }
                        }
                    }
                    i = ii; j = jj;
                    continue;
                }
            }
            i++; j++;
        }
    }
    // Anti-diagonal (down-left) lines
    for (int startRow = 0; startRow < BOARD_SIZE; ++startRow) {
        int i = startRow, j = BOARD_SIZE - 1;
        while (i < BOARD_SIZE && j >= 0) {
            if (boardArr[i][j] != '.') {
                char sym = boardArr[i][j];
                if (i == startRow || j == BOARD_SIZE-1 || boardArr[i-1][j+1] != sym) {
                    int ii = i, jj = j;
                    while (ii < BOARD_SIZE && jj >= 0 && boardArr[ii][jj] == sym) {
                        ii++; jj--;
                    }
                    int length = ii - i;
                    bool leftOpen = (i-1 >= 0 && j+1 < BOARD_SIZE && boardArr[i-1][j+1] == '.');
                    bool rightOpen = (ii < BOARD_SIZE && jj >= 0 && boardArr[ii][jj] == '.');
                    if (length >= 5) {
                        return (sym == myPlayer ? INF : -INF);
                    }
                    if (leftOpen || rightOpen) {
                        if (sym == myPlayer) {
                            if (leftOpen && rightOpen) {
                                switch(length) {
                                    case 4: score += OPEN_FOUR; break;
                                    case 3: score += OPEN_THREE; break;
                                    case 2: score += OPEN_TWO; break;
                                    case 1: score += ONE_PIECE; break;
                                }
                            } else {
                                switch(length) {
                                    case 4: score += CLOSED_FOUR; break;
                                    case 3: score += CLOSED_THREE; break;
                                    case 2: score += CLOSED_TWO; break;
                                    case 1: score += 1; break;
                                }
                            }
                        } else {
                            if (leftOpen && rightOpen) {
                                switch(length) {
                                    case 4: score -= OPEN_FOUR; break;
                                    case 3: score -= OPEN_THREE; break;
                                    case 2: score -= OPEN_TWO; break;
                                    case 1: score -= ONE_PIECE; break;
                                }
                            } else {
                                switch(length) {
                                    case 4: score -= CLOSED_FOUR; break;
                                    case 3: score -= CLOSED_THREE; break;
                                    case 2: score -= CLOSED_TWO; break;
                                    case 1: score -= 1; break;
                                }
                            }
                        }
                    }
                    i = ii; j = jj;
                    continue;
                }
            }
            i++; j--;
        }
    }
    for (int startCol = BOARD_SIZE - 2; startCol >= 0; --startCol) {
        // start at last column and each row from 1 to end for remaining anti-diagonals
        int i = 0, j = startCol;
        while (i < BOARD_SIZE && j >= 0) {
            if (boardArr[i][j] != '.') {
                char sym = boardArr[i][j];
                if (i == 0 || j == startCol || boardArr[i-1][j+1] != sym) {
                    int ii = i, jj = j;
                    while (ii < BOARD_SIZE && jj >= 0 && boardArr[ii][jj] == sym) {
                        ii++; jj--;
                    }
                    int length = ii - i;
                    bool leftOpen = (i-1 >= 0 && j+1 < BOARD_SIZE && boardArr[i-1][j+1] == '.');
                    bool rightOpen = (ii < BOARD_SIZE && jj >= 0 && boardArr[ii][jj] == '.');
                    if (length >= 5) {
                        return (sym == myPlayer ? INF : -INF);
                    }
                    if (leftOpen || rightOpen) {
                        if (sym == myPlayer) {
                            if (leftOpen && rightOpen) {
                                switch(length) {
                                    case 4: score += OPEN_FOUR; break;
                                    case 3: score += OPEN_THREE; break;
                                    case 2: score += OPEN_TWO; break;
                                    case 1: score += ONE_PIECE; break;
                                }
                            } else {
                                switch(length) {
                                    case 4: score += CLOSED_FOUR; break;
                                    case 3: score += CLOSED_THREE; break;
                                    case 2: score += CLOSED_TWO; break;
                                    case 1: score += 1; break;
                                }
                            }
                        } else {
                            if (leftOpen && rightOpen) {
                                switch(length) {
                                    case 4: score -= OPEN_FOUR; break;
                                    case 3: score -= OPEN_THREE; break;
                                    case 2: score -= OPEN_TWO; break;
                                    case 1: score -= ONE_PIECE; break;
                                }
                            } else {
                                switch(length) {
                                    case 4: score -= CLOSED_FOUR; break;
                                    case 3: score -= CLOSED_THREE; break;
                                    case 2: score -= CLOSED_TWO; break;
                                    case 1: score -= 1; break;
                                }
                            }
                        }
                    }
                    i = ii; j = jj;
                    continue;
                }
            }
            i++; j--;
        }
    }
    return score;
}

// Minimax search with alpha-beta pruning. Returns best score for current player.
int searchMinimax(int depth, bool maximizingPlayer, int alpha, int beta) {
    if (std::chrono::steady_clock::now() - startTime >= timeLimit) {
        throw TimeOutException();
    }
    if (depth == 0) {
        return evaluateBoard();
    }
    if (maximizingPlayer) {
        int bestVal = -INF;
        // Generate candidate moves for myPlayer
        std::vector<std::pair<int,int>> moves;
        moves.reserve(100);
        bool anyPiece = false;
        for (int r = 0; r < BOARD_SIZE; ++r) {
            for (int c = 0; c < BOARD_SIZE; ++c) {
                if (boardArr[r][c] != '.') {
                    anyPiece = true;
                    break;
                }
            }
            if (anyPiece) break;
        }
        if (!anyPiece) {
            // no pieces on board, choose center
            moves.emplace_back(BOARD_SIZE/2, BOARD_SIZE/2);
        } else {
            for (int r = 0; r < BOARD_SIZE; ++r) {
                for (int c = 0; c < BOARD_SIZE; ++c) {
                    if (boardArr[r][c] == '.') {
                        // if neighbor within 2 cells exists
                        bool neighbor = false;
                        for (int dr = -2; dr <= 2 && !neighbor; ++dr) {
                            for (int dc = -2; dc <= 2 && !neighbor; ++dc) {
                                if (dr == 0 && dc == 0) continue;
                                int rr = r + dr, cc = c + dc;
                                if (rr >= 0 && rr < BOARD_SIZE && cc >= 0 && cc < BOARD_SIZE 
                                    && boardArr[rr][cc] != '.') {
                                    neighbor = true;
                                }
                            }
                        }
                        if (neighbor) {
                            moves.emplace_back(r, c);
                        }
                    }
                }
            }
        }
        if (moves.empty()) {
            // if somehow no moves found (shouldn't happen unless board full)
            for (int r = 0; r < BOARD_SIZE; ++r) {
                for (int c = 0; c < BOARD_SIZE; ++c) {
                    if (boardArr[r][c] == '.') moves.emplace_back(r, c);
                }
            }
        }
        // Move ordering: sort moves by heuristic value (descending)
        std::sort(moves.begin(), moves.end(), [&](const std::pair<int,int>& a, const std::pair<int,int>& b) {
            boardArr[a.first][a.second] = myPlayer;
            int evalA = evaluateBoard();
            boardArr[a.first][a.second] = '.';
            boardArr[b.first][b.second] = myPlayer;
            int evalB = evaluateBoard();
            boardArr[b.first][b.second] = '.';
            return evalA > evalB;
        });
        for (auto& mv : moves) {
            int r = mv.first, c = mv.second;
            boardArr[r][c] = myPlayer;
            int moveScore;
            if (checkFiveInRow(r, c, myPlayer)) {
                // immediate win achieved
                moveScore = INF;
            } else {
                moveScore = searchMinimax(depth - 1, false, alpha, beta);
            }
            boardArr[r][c] = '.';
            if (moveScore > bestVal) {
                bestVal = moveScore;
            }
            if (moveScore > alpha) {
                alpha = moveScore;
            }
            if (alpha >= beta) {
                // beta cut-off
                break;
            }
            if (bestVal == INF) {
                // found a winning move, no need to search further at this depth
                break;
            }
        }
        return bestVal;
    } else {  // minimizing player (opponent's turn)
        int bestVal = INF;
        std::vector<std::pair<int,int>> moves;
        moves.reserve(100);
        bool anyPiece = false;
        for (int r = 0; r < BOARD_SIZE; ++r) {
            for (int c = 0; c < BOARD_SIZE; ++c) {
                if (boardArr[r][c] != '.') {
                    anyPiece = true;
                    break;
                }
            }
            if (anyPiece) break;
        }
        if (!anyPiece) {
            moves.emplace_back(BOARD_SIZE/2, BOARD_SIZE/2);
        } else {
            for (int r = 0; r < BOARD_SIZE; ++r) {
                for (int c = 0; c < BOARD_SIZE; ++c) {
                    if (boardArr[r][c] == '.') {
                        bool neighbor = false;
                        for (int dr = -2; dr <= 2 && !neighbor; ++dr) {
                            for (int dc = -2; dc <= 2 && !neighbor; ++dc) {
                                if (dr == 0 && dc == 0) continue;
                                int rr = r + dr, cc = c + dc;
                                if (rr >= 0 && rr < BOARD_SIZE && cc >= 0 && cc < BOARD_SIZE 
                                    && boardArr[rr][cc] != '.') {
                                    neighbor = true;
                                }
                            }
                        }
                        if (neighbor) {
                            moves.emplace_back(r, c);
                        }
                    }
                }
            }
        }
        if (moves.empty()) {
            for (int r = 0; r < BOARD_SIZE; ++r) {
                for (int c = 0; c < BOARD_SIZE; ++c) {
                    if (boardArr[r][c] == '.') moves.emplace_back(r, c);
                }
            }
        }
        // Sort moves by heuristic (ascending, since opponent tries to minimize our score)
        std::sort(moves.begin(), moves.end(), [&](const std::pair<int,int>& a, const std::pair<int,int>& b) {
            boardArr[a.first][a.second] = oppPlayer;
            int evalA = evaluateBoard();
            boardArr[a.first][a.second] = '.';
            boardArr[b.first][b.second] = oppPlayer;
            int evalB = evaluateBoard();
            boardArr[b.first][b.second] = '.';
            return evalA < evalB;
        });
        for (auto& mv : moves) {
            int r = mv.first, c = mv.second;
            boardArr[r][c] = oppPlayer;
            int moveScore;
            if (checkFiveInRow(r, c, oppPlayer)) {
                moveScore = -INF;
            } else {
                moveScore = searchMinimax(depth - 1, true, alpha, beta);
            }
            boardArr[r][c] = '.';
            if (moveScore < bestVal) {
                bestVal = moveScore;
            }
            if (moveScore < beta) {
                beta = moveScore;
            }
            if (alpha >= beta) {
                break;
            }
            if (bestVal == -INF) {
                break;
            }
        }
        return bestVal;
    }
}

// Choose the best move for myPlayer from the current board state
std::pair<int,int> choose_move() {
    // 1. Immediate win check
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if (boardArr[r][c] == '.') {
                boardArr[r][c] = myPlayer;
                if (checkFiveInRow(r, c, myPlayer)) {
                    boardArr[r][c] = '.';  // revert
                    return {r, c};
                }
                boardArr[r][c] = '.';
            }
        }
    }
    // 2. Immediate block opponent's win
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if (boardArr[r][c] == '.') {
                boardArr[r][c] = oppPlayer;
                if (checkFiveInRow(r, c, oppPlayer)) {
                    boardArr[r][c] = '.';
                    return {r, c};
                }
                boardArr[r][c] = '.';
            }
        }
    }
    // 3. If board is empty, play in the center
    bool anyPiece = false;
    for (int r = 0; r < BOARD_SIZE && !anyPiece; ++r) {
        for (int c = 0; c < BOARD_SIZE && !anyPiece; ++c) {
            if (boardArr[r][c] != '.') {
                anyPiece = true;
            }
        }
    }
    if (!anyPiece) {
        int center = BOARD_SIZE / 2;
        return {center, center};
    }
    // 4. Search for the best move using iterative deepening
    std::pair<int,int> bestMove = {-1, -1};
    int maxDepthReached = 0;
    try {
        for (int depth = 1; depth <= 15; ++depth) {
            int alpha = -INF, beta = INF;
            int bestScore = -INF;
            std::pair<int,int> bestMoveThisDepth = {-1, -1};
            // Generate moves (same approach as in searchMinimax for maximizing root)
            std::vector<std::pair<int,int>> moves;
            moves.reserve(100);
            for (int r = 0; r < BOARD_SIZE; ++r) {
                for (int c = 0; c < BOARD_SIZE; ++c) {
                    if (boardArr[r][c] == '.') {
                        bool neighbor = false;
                        for (int dr = -2; dr <= 2 && !neighbor; ++dr) {
                            for (int dc = -2; dc <= 2 && !neighbor; ++dc) {
                                if (dr == 0 && dc == 0) continue;
                                int rr = r + dr, cc = c + dc;
                                if (rr >= 0 && rr < BOARD_SIZE && cc >= 0 && cc < BOARD_SIZE 
                                    && boardArr[rr][cc] != '.') {
                                    neighbor = true;
                                }
                            }
                        }
                        if (neighbor) {
                            moves.emplace_back(r, c);
                        }
                    }
                }
            }
            if (moves.empty()) {
                for (int r = 0; r < BOARD_SIZE; ++r) {
                    for (int c = 0; c < BOARD_SIZE; ++c) {
                        if (boardArr[r][c] == '.') {
                            moves.emplace_back(r, c);
                        }
                    }
                }
            }
            // Move ordering at root
            std::sort(moves.begin(), moves.end(), [&](const std::pair<int,int>& a, const std::pair<int,int>& b) {
                boardArr[a.first][a.second] = myPlayer;
                int evalA = evaluateBoard();
                boardArr[a.first][a.second] = '.';
                boardArr[b.first][b.second] = myPlayer;
                int evalB = evaluateBoard();
                boardArr[b.first][b.second] = '.';
                return evalA > evalB;
            });
            for (auto& mv : moves) {
                int r = mv.first, c = mv.second;
                boardArr[r][c] = myPlayer;
                int score;
                if (checkFiveInRow(r, c, myPlayer)) {
                    score = INF;
                } else {
                    score = searchMinimax(depth - 1, false, alpha, beta);
                }
                boardArr[r][c] = '.';
                if (score > bestScore) {
                    bestScore = score;
                    bestMoveThisDepth = mv;
                }
                if (score > alpha) alpha = score;
                if (alpha >= beta || bestScore == INF) {
                    // cut off further moves at this depth
                    break;
                }
            }
            // If we complete the depth search successfully, store the result
            bestMove = bestMoveThisDepth;
            maxDepthReached = depth;
            // If a winning move is found, we can break early
            if (bestScore == INF) break;
        }
    } catch (const TimeOutException&) {
        // Time limit reached during search; use bestMove from last completed depth
    }
    // Fallback: if no move was found (should not happen, but just in case)
    if (bestMove.first == -1) {
        for (int r = 0; r < BOARD_SIZE; r++) {
            for (int c = 0; c < BOARD_SIZE; c++) {
                if (boardArr[r][c] == '.') {
                    bestMove = {r, c};
                    break;
                }
            }
            if (bestMove.first != -1) break;
        }
    }
    return bestMove;
}

int main(int argc, char **argv){
    if(argc!=2){
        std::cerr<<"Usage: "<<argv[0]<<" /path/to/state.json\n";
        return 1;
    }
    // 1) load state.json
    json state;
    try {
        std::ifstream f(argv[1]);
        f >> state;
    } catch (...) {
        std::cerr<<"ERROR: Failed to read or parse state.json\n";
        return 1;
    }

    // 2) extract
    auto boardData = state["board"];
    std::string playerStr = state["player"];
    if (playerStr != "X" && playerStr != "O") {
        std::cerr << "ERROR: Invalid player value in JSON\n";
        return 1;
    }
    myPlayer = playerStr[0];
    oppPlayer = (myPlayer == 'X' ? 'O' : 'X');

    // 3) Fill boardArr
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            std::string cell = boardData[i][j].get<std::string>();
            if (cell == "" || cell == "") {
                boardArr[i][j] = '.';
            } else {
                // should be "X" or "O"
                boardArr[i][j] = cell[0];
            }
        }
    }
    startTime = std::chrono::steady_clock::now();
    std::pair<int,int> move;
    try {
        move = choose_move();
    } catch (const TimeOutException&) {
        // If somehow time elapsed before finishing (unlikely in choose_move), pick first available
        for (int r = 0; r < BOARD_SIZE; ++r) {
            for (int c = 0; c < BOARD_SIZE; ++c) {
                if (boardArr[r][c] == '.') {
                    move = {r, c};
                    goto MOVE_FOUND;
                }
            }
        }
        MOVE_FOUND: ;
    }

    // 4) output JSON array to stdout
    std::cout<<"["<<move.first<<", "<<move.second<<"]\n";
    return 0;
}
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

#include "chess.hpp"
#include "search.hpp"
#include "transposition_table.hpp"
#include "uci_utils.hpp"

using namespace engine;

namespace {

std::chrono::milliseconds computeTimeBudget(bool isWhite, int wtime, int btime, int winc, int binc, int movesToGo) {
    int myTime = isWhite ? wtime : btime;
    int myInc = isWhite ? winc : binc;
    int assumedMovesLeft = movesToGo > 0 ? movesToGo : 30;

    int budgetMs = myTime / assumedMovesLeft + myInc - 50;  // 50ms safety margin
    budgetMs = std::max(budgetMs, 50);
    return std::chrono::milliseconds(budgetMs);
}

void handlePosition(chess::Board& board, std::istringstream& iss) {
    std::string sub;
    iss >> sub;

    if (sub == "startpos") {
        board = chess::Board();
    } else if (sub == "fen") {
        std::string fen, part;
        for (int i = 0; i < 6 && iss >> part; ++i) {
            fen += part;
            if (i < 5) fen += " ";
        }
        board = chess::Board(fen);
    }

    std::string movesToken;
    if (iss >> movesToken && movesToken == "moves") {
        std::string mv;
        while (iss >> mv) applyUciMove(board, mv);
    }
}

void handleGo(chess::Board& board, TranspositionTable& tt, std::istringstream& iss) {
    int wtime = 0, btime = 0, winc = 0, binc = 0, movestogo = 0, depth = 0, movetime = -1;
    bool infinite = false;

    std::string p;
    while (iss >> p) {
        if (p == "wtime") iss >> wtime;
        else if (p == "btime") iss >> btime;
        else if (p == "winc") iss >> winc;
        else if (p == "binc") iss >> binc;
        else if (p == "movestogo") iss >> movestogo;
        else if (p == "depth") iss >> depth;
        else if (p == "movetime") iss >> movetime;
        else if (p == "infinite") infinite = true;
    }

    std::chrono::milliseconds budget(5000);  // default if no time info given at all
    int maxDepth = 64;

    if (depth > 0) {
        maxDepth = depth;
        budget = std::chrono::minutes(10);  // fixed-depth search, time isn't the limiting factor
    } else if (movetime > 0) {
        budget = std::chrono::milliseconds(movetime);
    } else if (infinite) {
        budget = std::chrono::minutes(10);
    } else if (wtime > 0 || btime > 0) {
        bool isWhite = board.sideToMove() == chess::Color::WHITE;
        budget = computeTimeBudget(isWhite, wtime, btime, winc, binc, movestogo);
    }

    Searcher searcher(tt);
    searcher.onDepthComplete = [](const Searcher::SearchResult& r) {
        std::cout << "info depth " << r.depthReached << " score cp " << r.score << " nodes " << r.nodes
                   << " pv " << moveToUci(r.bestMove) << "\n" << std::flush;
    };

    auto result = searcher.searchWithTimeLimit(board, budget, maxDepth);
    std::string best = (result.bestMove == chess::Move::NO_MOVE) ? "0000" : moveToUci(result.bestMove);
    std::cout << "bestmove " << best << "\n" << std::flush;
}

}  // namespace

int main() {
    chess::Board board;
    TranspositionTable tt(64);
    std::string line;

    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "uci") {
            std::cout << "id name NeuralGambit\n";
            std::cout << "id author vishalpatel04\n";
            std::cout << "uciok\n" << std::flush;
        } else if (token == "isready") {
            std::cout << "readyok\n" << std::flush;
        } else if (token == "ucinewgame") {
            tt.clear();
            board = chess::Board();
        } else if (token == "position") {
            handlePosition(board, iss);
        } else if (token == "go") {
            handleGo(board, tt, iss);
        } else if (token == "stop") {
            // Single-threaded engine: by the time "stop" can be read, the
            // blocking search for the current "go" has already returned its
            // bestmove. Not a true async interrupt — a known limitation.
        } else if (token == "quit") {
            break;
        }
    }
    return 0;
}

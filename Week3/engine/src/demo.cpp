#include <iostream>
#include <string>

#include "mate_solver.hpp"

namespace {

void printBoard(const chess::Board& board) {
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << (rank + 1) << " ";
        for (int file = 0; file < 8; ++file) {
            const chess::Square sq{chess::File(file), chess::Rank(rank)};
            std::cout << static_cast<std::string>(board.at<chess::Piece>(sq)) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "  a b c d e f g h\n";
}

}  // namespace

// Plays out a solved mate-in-N line move by move, printing the board after
// each move, so the solver's work is visible rather than just a UCI string.
int main(int argc, char** argv) {
    const std::string fen = argc > 1 ? argv[1] : "4r1rk/5K1b/7R/R7/8/8/8/8 w - - 0 1";
    const int n = argc > 2 ? std::stoi(argv[2]) : 2;

    neuralgambit::MateSolver solver(fen);
    std::cout << "Starting position (mate in " << n << "):\n";
    printBoard(solver.board());

    auto line = solver.findMate(n);
    if (!line) {
        std::cout << "\nNo forced mate in " << n << " found.\n";
        return 0;
    }

    chess::Board& board = solver.board();
    for (size_t i = 0; i < line->size(); ++i) {
        const auto& move = (*line)[i];
        const bool attackerMove = (i % 2 == 0);
        std::cout << "\n" << (attackerMove ? "Attacker" : "Defender") << " plays "
                  << neuralgambit::moveToUci(move) << ":\n";
        board.makeMove(move);
        printBoard(board);
    }
    std::cout << "\nCheckmate delivered in " << n << " moves.\n";
    return 0;
}

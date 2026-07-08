#include <iostream>
#include <string>

#include "mate_solver.hpp"

// CLI: mate_solver "<FEN>" <N>
// Prints the forced mating line (UCI moves) for the side to move, if one
// exists within N of their own moves.
int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <FEN> <mate-in-N>\n";
        return 1;
    }

    const std::string fen = argv[1];
    const int n = std::stoi(argv[2]);

    neuralgambit::MateSolver solver(fen);
    auto line = solver.findMate(n);

    if (!line) {
        std::cout << "No forced mate in " << n << " found.\n";
        return 0;
    }

    std::cout << "Mate in " << n << ":";
    for (const auto& move : *line) {
        std::cout << ' ' << neuralgambit::moveToUci(move);
    }
    std::cout << '\n';
    return 0;
}

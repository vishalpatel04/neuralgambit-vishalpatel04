#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

#include "mate_solver.hpp"
#include "tiny_json.hpp"

// CLI: puzzle_runner <puzzles.json> <mate-in-N> [limit]
// Runs MateSolver over every FEN in a mate_in_N.json puzzle file and reports
// how many were solved (a forced mate within N moves was found).
int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <puzzles.json> <mate-in-N> [limit]\n";
        return 1;
    }

    const std::string path = argv[1];
    const int n = std::stoi(argv[2]);
    const size_t limit = argc >= 4 ? static_cast<size_t>(std::stoul(argv[3])) : std::numeric_limits<size_t>::max();

    constexpr auto kPerPuzzleTimeLimit = std::chrono::milliseconds(20000);

    const auto puzzles = neuralgambit::parseFlatJsonObject(path);
    size_t total = 0;
    size_t solved = 0;
    size_t timedOut = 0;
    const auto start = std::chrono::steady_clock::now();

    for (const auto& [fen, expected] : puzzles) {
        if (total >= limit) break;
        ++total;

        neuralgambit::MateSolver solver(fen);
        auto result = solver.findMateBounded(n, kPerPuzzleTimeLimit);

        if (result.status == neuralgambit::MateSolver::Status::kMateFound) {
            ++solved;
        } else if (result.status == neuralgambit::MateSolver::Status::kTimedOut) {
            ++timedOut;
            std::cout << "TIMEOUT: " << fen << "  (expected: " << expected << ")\n";
        } else {
            std::cout << "FAILED: " << fen << "  (expected: " << expected << ")\n";
        }

        if (total % 25 == 0) {
            const double soFar = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            std::cout << "... " << total << " done (" << solved << " solved, " << timedOut << " timed out) at "
                      << soFar << "s\n"
                      << std::flush;
        }
    }

    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << solved << "/" << total << " puzzles solved (" << timedOut << " timed out after "
              << kPerPuzzleTimeLimit.count() << "ms each) in " << elapsed << "s\n";
    return (solved + timedOut == total) ? 0 : 1;
}

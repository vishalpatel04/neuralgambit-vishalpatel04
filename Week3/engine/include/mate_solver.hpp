#pragma once

#include <algorithm>
#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "chess.hpp"

namespace neuralgambit {

// Formats a move as a UCI string (e.g. "e2e4", "e7e8q"), matching the UCI
// protocol the wider engine speaks.
inline std::string moveToUci(const chess::Move& move) {
    std::string uci = static_cast<std::string>(move.from()) + static_cast<std::string>(move.to());
    if (move.typeOf() == chess::Move::PROMOTION) {
        uci += static_cast<std::string>(chess::PieceType(move.promotionType()))[0];
    }
    return uci;
}

// Solves "mate in N" puzzles: given a position, finds a sequence of moves for
// the side to move such that, against every possible defense, checkmate is
// delivered within N moves of the attacker (a forced mate, per the classical
// definition used in chess-puzzle compositions).
class MateSolver {
   public:
    enum class Status { kMateFound, kNoMate, kTimedOut };

    struct Result {
        Status status;
        std::optional<std::vector<chess::Move>> line;
    };

    explicit MateSolver(const std::string& fen) : board_(fen) {}

    // Returns the forced mating line (attacker move, defender reply, attacker
    // move, ...) if the side to move can force checkmate within `n` of their
    // own moves against any defense. Returns std::nullopt if no such forced
    // mate exists. Search runs to completion (no time limit).
    std::optional<std::vector<chess::Move>> findMate(int n) {
        deadline_.reset();
        timedOut_ = false;
        return search(n);
    }

    // Same as findMate, but aborts and reports kTimedOut if the search runs
    // longer than `timeLimit`. Some puzzle positions have a branching factor
    // that makes the unpruned exhaustive search impractically slow; this
    // lets callers (e.g. batch puzzle verification) bound the cost per
    // position instead of blocking indefinitely.
    Result findMateBounded(int n, std::chrono::milliseconds timeLimit) {
        deadline_ = std::chrono::steady_clock::now() + timeLimit;
        timedOut_ = false;
        nodesSinceClockCheck_ = 0;

        auto line = search(n);
        if (timedOut_) return {Status::kTimedOut, std::nullopt};
        return {line ? Status::kMateFound : Status::kNoMate, line};
    }

    chess::Board& board() { return board_; }

   private:
    chess::Board board_;
    std::optional<std::chrono::steady_clock::time_point> deadline_;
    bool timedOut_ = false;
    int nodesSinceClockCheck_ = 0;

    // Checking the clock on every node is wasteful, so it's amortized over
    // batches of nodes.
    bool deadlineExceeded() {
        if (timedOut_) return true;
        if (!deadline_) return false;
        if (++nodesSinceClockCheck_ < 1024) return false;
        nodesSinceClockCheck_ = 0;
        if (std::chrono::steady_clock::now() > *deadline_) timedOut_ = true;
        return timedOut_;
    }

    // Orders capturing moves first: a cheap heuristic that tends to surface
    // the winning line sooner during the exhaustive search below.
    static void orderMoves(chess::Movelist& moves, const chess::Board& board) {
        std::stable_sort(moves.begin(), moves.end(), [&board](const chess::Move& a, const chess::Move& b) {
            return board.isCapture(a) && !board.isCapture(b);
        });
    }

    // movesLeft = number of moves still available to the side to move
    // (the attacker) to complete the mate.
    std::optional<std::vector<chess::Move>> search(int movesLeft) {
        if (deadlineExceeded()) return std::nullopt;

        chess::Movelist moves;
        chess::movegen::legalmoves(moves, board_);
        orderMoves(moves, board_);

        for (const auto& move : moves) {
            if (deadlineExceeded()) return std::nullopt;

            board_.makeMove(move);

            chess::Movelist replies;
            chess::movegen::legalmoves(replies, board_);
            const bool inCheck = board_.inCheck();
            const bool noReplies = replies.empty();

            if (noReplies && inCheck) {
                // Immediate checkmate.
                board_.unmakeMove(move);
                return std::vector<chess::Move>{move};
            }

            if (movesLeft > 1 && !noReplies) {
                // Every defensive reply must still lose within the
                // remaining moves for this candidate to count as forced.
                bool allRepliesLose = true;
                std::optional<std::vector<chess::Move>> continuation;

                for (const auto& reply : replies) {
                    board_.makeMove(reply);
                    auto sub = search(movesLeft - 1);
                    board_.unmakeMove(reply);

                    if (!sub) {
                        allRepliesLose = false;
                        break;
                    }
                    if (!continuation) {
                        std::vector<chess::Move> line{reply};
                        line.insert(line.end(), sub->begin(), sub->end());
                        continuation = std::move(line);
                    }
                }

                if (allRepliesLose) {
                    std::vector<chess::Move> line{move};
                    line.insert(line.end(), continuation->begin(), continuation->end());
                    board_.unmakeMove(move);
                    return line;
                }
            }

            board_.unmakeMove(move);
        }

        return std::nullopt;
    }
};

}  // namespace neuralgambit

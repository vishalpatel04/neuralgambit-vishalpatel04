#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

#include "chess.hpp"
#include "evaluation.hpp"
#include "transposition_table.hpp"

namespace engine {

constexpr int INF = 1'000'000;
constexpr int MATE_SCORE = 900'000;

// Iterative-deepening negamax with alpha-beta pruning, a Zobrist-hash
// transposition table, and quiescence search at leaf nodes. Time-bounded:
// searchWithTimeLimit() runs depth 1, 2, 3, ... keeping the best move found
// by the last *fully completed* depth, and stops as soon as the time budget
// is exhausted rather than searching to a fixed depth.
class Searcher {
   public:
    explicit Searcher(TranspositionTable& tt) : tt_(tt) {}

    struct SearchResult {
        chess::Move bestMove = chess::Move::NO_MOVE;
        int score = 0;
        int depthReached = 0;
        long nodes = 0;
    };

    // Called after each fully-completed depth, e.g. to print UCI "info" lines.
    std::function<void(const SearchResult&)> onDepthComplete;

    SearchResult searchWithTimeLimit(chess::Board& board, std::chrono::milliseconds timeLimit, int maxDepth = 64) {
        deadline_ = std::chrono::steady_clock::now() + timeLimit;
        stop_ = false;
        nodesSinceCheck_ = 0;

        SearchResult result;
        result.nodes = 0;

        chess::Movelist rootMoves;
        chess::movegen::legalmoves(rootMoves, board);
        if (!rootMoves.empty()) result.bestMove = rootMoves[0];  // safety fallback

        for (int depth = 1; depth <= maxDepth; ++depth) {
            nodes_ = 0;
            chess::Move moveThisDepth = chess::Move::NO_MOVE;
            int score = negamaxRoot(board, depth, moveThisDepth);

            if (stop_) break;  // partial depth — discard, keep the previous depth's result

            result.bestMove = moveThisDepth;
            result.score = score;
            result.depthReached = depth;
            result.nodes += nodes_;
            if (onDepthComplete) onDepthComplete(result);

            // Found a forced mate — no need to search deeper.
            if (score >= MATE_SCORE - 1000 || score <= -MATE_SCORE + 1000) break;
        }
        return result;
    }

   private:
    TranspositionTable& tt_;
    std::chrono::steady_clock::time_point deadline_;
    bool stop_ = false;
    long nodes_ = 0;
    int nodesSinceCheck_ = 0;

    bool timeUp() {
        if (stop_) return true;
        if (++nodesSinceCheck_ < 2048) return false;
        nodesSinceCheck_ = 0;
        if (std::chrono::steady_clock::now() >= deadline_) stop_ = true;
        return stop_;
    }

    static void orderMoves(chess::Movelist& moves, const chess::Board& board, chess::Move ttMove) {
        std::stable_sort(moves.begin(), moves.end(), [&](const chess::Move& a, const chess::Move& b) {
            auto priority = [&](const chess::Move& m) {
                if (m == ttMove) return 2;
                if (board.isCapture(m)) return 1;
                return 0;
            };
            return priority(a) > priority(b);
        });
    }

    static void orderCaptures(std::vector<chess::Move>& moves, const chess::Board& board) {
        std::stable_sort(moves.begin(), moves.end(), [&board](const chess::Move& a, const chess::Move& b) {
            return board.isCapture(a) && !board.isCapture(b);
        });
    }

    int negamaxRoot(chess::Board& board, int depth, chess::Move& bestMoveOut) {
        chess::Movelist moves;
        chess::movegen::legalmoves(moves, board);

        std::uint64_t key = board.hash();
        const TTEntry* entry = tt_.probe(key);
        orderMoves(moves, board, entry ? entry->bestMove : chess::Move::NO_MOVE);

        int alpha = -INF, beta = INF;
        int best = -INF;
        chess::Move localBest = chess::Move::NO_MOVE;

        for (const auto& move : moves) {
            board.makeMove(move);
            int score = -negamax(board, depth - 1, -beta, -alpha, 1);
            board.unmakeMove(move);

            if (stop_) return best;

            if (score > best) {
                best = score;
                localBest = move;
            }
            alpha = std::max(alpha, score);
        }

        bestMoveOut = localBest;
        return best;
    }

    int negamax(chess::Board& board, int depth, int alpha, int beta, int ply) {
        if (timeUp()) return 0;
        ++nodes_;

        if (ply > 0 && (board.isRepetition() || board.isInsufficientMaterial())) return 0;

        std::uint64_t key = board.hash();
        int origAlpha = alpha;
        const TTEntry* entry = tt_.probe(key);
        if (entry && entry->depth >= depth) {
            if (entry->bound == Bound::EXACT) return entry->score;
            if (entry->bound == Bound::LOWER) alpha = std::max(alpha, entry->score);
            else if (entry->bound == Bound::UPPER) beta = std::min(beta, entry->score);
            if (alpha >= beta) return entry->score;
        }

        if (depth <= 0) return quiescence(board, alpha, beta, ply);

        chess::Movelist moves;
        chess::movegen::legalmoves(moves, board);
        if (moves.empty()) {
            return board.inCheck() ? (ply - MATE_SCORE) : 0;  // checkmate or stalemate
        }

        orderMoves(moves, board, entry ? entry->bestMove : chess::Move::NO_MOVE);

        int best = -INF;
        chess::Move bestMove = chess::Move::NO_MOVE;
        for (const auto& move : moves) {
            board.makeMove(move);
            int score = -negamax(board, depth - 1, -beta, -alpha, ply + 1);
            board.unmakeMove(move);

            if (stop_) return best == -INF ? 0 : best;

            if (score > best) {
                best = score;
                bestMove = move;
            }
            alpha = std::max(alpha, score);
            if (alpha >= beta) break;
        }

        Bound bound = best <= origAlpha ? Bound::UPPER : (best >= beta ? Bound::LOWER : Bound::EXACT);
        tt_.store(key, depth, best, bound, bestMove);
        return best;
    }

    // Extends the search through captures (and, if in check, all evasions)
    // past the nominal depth limit, so the static evaluation is never taken
    // mid-capture-sequence — that's what makes plain minimax's leaf scores
    // unreliable (the "horizon effect").
    int quiescence(chess::Board& board, int alpha, int beta, int ply) {
        if (timeUp()) return 0;
        ++nodes_;

        bool inCheck = board.inCheck();
        int standPat = evaluate(board);
        if (!inCheck) {
            if (standPat >= beta) return beta;
            alpha = std::max(alpha, standPat);
        }

        chess::Movelist moves;
        chess::movegen::legalmoves(moves, board);

        std::vector<chess::Move> candidates;
        candidates.reserve(moves.size());
        for (const auto& m : moves) {
            if (inCheck || board.isCapture(m) || m.typeOf() == chess::Move::PROMOTION) {
                candidates.push_back(m);
            }
        }

        if (candidates.empty()) {
            return inCheck ? (ply - MATE_SCORE) : standPat;
        }

        orderCaptures(candidates, board);

        for (const auto& move : candidates) {
            board.makeMove(move);
            int score = -quiescence(board, -beta, -alpha, ply + 1);
            board.unmakeMove(move);

            if (stop_) return alpha;
            if (score >= beta) return beta;
            alpha = std::max(alpha, score);
        }
        return alpha;
    }
};

}  // namespace engine

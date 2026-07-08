#pragma once

#include <string>

#include "chess.hpp"

namespace engine {

inline std::string moveToUci(const chess::Move& move) {
    std::string uci = static_cast<std::string>(move.from()) + static_cast<std::string>(move.to());
    if (move.typeOf() == chess::Move::PROMOTION) {
        uci += static_cast<std::string>(chess::PieceType(move.promotionType()))[0];
    }
    return uci;
}

// Finds the legal move matching a UCI move string (e.g. "e2e4", "e7e8q") and
// applies it to the board. No-op if no legal move matches.
inline void applyUciMove(chess::Board& board, const std::string& uciMove) {
    chess::Movelist moves;
    chess::movegen::legalmoves(moves, board);
    for (const auto& m : moves) {
        if (moveToUci(m) == uciMove) {
            board.makeMove(m);
            return;
        }
    }
}

}  // namespace engine

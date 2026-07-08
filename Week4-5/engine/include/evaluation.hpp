#pragma once

#include "chess.hpp"

namespace engine {

constexpr int PAWN_VALUE = 100;
constexpr int KNIGHT_VALUE = 320;
constexpr int BISHOP_VALUE = 330;
constexpr int ROOK_VALUE = 500;
constexpr int QUEEN_VALUE = 900;

inline int pieceValue(chess::PieceType::underlying pt) {
    switch (pt) {
        case chess::PieceType::PAWN: return PAWN_VALUE;
        case chess::PieceType::KNIGHT: return KNIGHT_VALUE;
        case chess::PieceType::BISHOP: return BISHOP_VALUE;
        case chess::PieceType::ROOK: return ROOK_VALUE;
        case chess::PieceType::QUEEN: return QUEEN_VALUE;
        default: return 0;
    }
}

// Simplified piece-square tables (Tomasz Michniewski's well-known values),
// indexed 0=a1 .. 63=h8 (matching chess::Square's indexing) from White's
// perspective. For Black, the square is mirrored vertically before lookup.
constexpr int PAWN_PST[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    5, 10, 10,-20,-20, 10, 10,  5,
    5, -5,-10,  0,  0,-10, -5,  5,
    0,  0,  0, 20, 20,  0,  0,  0,
    5,  5, 10, 25, 25, 10,  5,  5,
   10, 10, 20, 30, 30, 20, 10, 10,
   50, 50, 50, 50, 50, 50, 50, 50,
    0,  0,  0,  0,  0,  0,  0,  0,
};

constexpr int KNIGHT_PST[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50,
};

constexpr int BISHOP_PST[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20,
};

constexpr int ROOK_PST[64] = {
     0,  0,  0,  5,  5,  0,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     5, 10, 10, 10, 10, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0,
};

constexpr int QUEEN_PST[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -10,  5,  5,  5,  5,  5,  0,-10,
      0,  0,  5,  5,  5,  5,  0, -5,
     -5,  0,  5,  5,  5,  5,  0, -5,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20,
};

constexpr int KING_PST[64] = {
     20, 30, 10,  0,  0, 10, 30, 20,
     20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
};

inline int pstValue(chess::PieceType::underlying pt, chess::Square sq, chess::Color color) {
    int index = sq.index();
    if (color == chess::Color::BLACK) {
        int file = index % 8;
        int rank = index / 8;
        index = (7 - rank) * 8 + file;
    }
    switch (pt) {
        case chess::PieceType::PAWN: return PAWN_PST[index];
        case chess::PieceType::KNIGHT: return KNIGHT_PST[index];
        case chess::PieceType::BISHOP: return BISHOP_PST[index];
        case chess::PieceType::ROOK: return ROOK_PST[index];
        case chess::PieceType::QUEEN: return QUEEN_PST[index];
        case chess::PieceType::KING: return KING_PST[index];
        default: return 0;
    }
}

// Static evaluation, returned relative to the side to move (positive = good
// for whoever moves next), matching the sign convention negamax expects.
inline int evaluate(const chess::Board& board) {
    int score = 0;
    constexpr chess::PieceType::underlying kTypes[] = {
        chess::PieceType::PAWN,  chess::PieceType::KNIGHT, chess::PieceType::BISHOP,
        chess::PieceType::ROOK,  chess::PieceType::QUEEN,  chess::PieceType::KING,
    };
    constexpr chess::Color::underlying kColors[] = {chess::Color::WHITE, chess::Color::BLACK};

    for (auto colorEnum : kColors) {
        chess::Color color(colorEnum);
        int sign = (colorEnum == chess::Color::WHITE) ? 1 : -1;
        for (auto pt : kTypes) {
            chess::Bitboard bb = board.pieces(chess::PieceType(pt), color);
            while (bb) {
                int sqIndex = bb.pop();
                score += sign * (pieceValue(pt) + pstValue(pt, chess::Square(sqIndex), color));
            }
        }
    }

    return board.sideToMove() == chess::Color::WHITE ? score : -score;
}

}  // namespace engine

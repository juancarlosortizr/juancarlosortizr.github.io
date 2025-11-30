#ifndef MATERIAL_H
#define MATERIAL_H

#include "board.h"

namespace material {

static inline int piece_value(PieceKind kind) {
    switch (kind) {
        case PieceKind::Queen:  return 9;
        case PieceKind::Rook:   return 5;
        case PieceKind::Bishop: return 3;
        case PieceKind::Knight: return 3;
        case PieceKind::Pawn:   return 1;
        case PieceKind::King:   return 0;
        default:                return 0;
    }
}

static inline int balance(const Board& board) {
    const int count = board.get_piece_count();
    int score = 0;
    for (int i = 0; i < count; ++i) {
        const Piece& piece = board.get_piece(i);
        const int value = piece_value(piece.kind);
        if (value == 0) {
            continue;
        }
        score += piece.white ? value : -value;
    }
    return score;
}

} // namespace material

#endif // MATERIAL_H

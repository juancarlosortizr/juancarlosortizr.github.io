#ifndef LOST_PIECES_H
#define LOST_PIECES_H

#include <vector>
#include "board.h"

namespace piece_loss {

static inline int kind_index(PieceKind kind) {
    switch (kind) {
        case PieceKind::King:   return 0;
        case PieceKind::Queen:  return 1;
        case PieceKind::Rook:   return 2;
        case PieceKind::Bishop: return 3;
        case PieceKind::Knight: return 4;
        case PieceKind::Pawn:   return 5;
        default:                return 0;
    }
}

static constexpr int STARTING_COUNTS[6] = {
    1, // King
    1, // Queen
    2, // Rook
    2, // Bishop
    2, // Knight
    8  // Pawn
};

static constexpr PieceKind DISPLAY_ORDER[6] = {
    PieceKind::Queen,
    PieceKind::Rook,
    PieceKind::Bishop,
    PieceKind::Knight,
    PieceKind::Pawn,
    PieceKind::King
};

inline void compute_lost_pieces(const Board& board,
                                std::vector<PieceKind>& whiteLost,
                                std::vector<PieceKind>& blackLost) {
    int whiteCounts[6] = {0};
    int blackCounts[6] = {0};
    const int count = board.get_piece_count();
    for (int i = 0; i < count; ++i) {
        const Piece& piece = board.get_piece(i);
        const int idx = kind_index(piece.kind);
        if (piece.white) {
            ++whiteCounts[idx];
        } else {
            ++blackCounts[idx];
        }
    }

    whiteLost.clear();
    blackLost.clear();
    whiteLost.reserve(16);
    blackLost.reserve(16);

    auto accumulate_losses = [&](bool white, std::vector<PieceKind>& dest) {
        const int* counts = white ? whiteCounts : blackCounts;
        for (PieceKind kind : DISPLAY_ORDER) {
            const int idx = kind_index(kind);
            int missing = STARTING_COUNTS[idx] - counts[idx];
            if (missing <= 0) continue;
            for (int j = 0; j < missing; ++j) {
                dest.push_back(kind);
            }
        }
    };

    accumulate_losses(true, whiteLost);
    accumulate_losses(false, blackLost);
}

} // namespace piece_loss

#endif // LOST_PIECES_H

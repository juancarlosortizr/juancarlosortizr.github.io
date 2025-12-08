#ifndef BOARD_PRINT_H
#define BOARD_PRINT_H

#include <array>
#include <cctype>
#include <ostream>
#include "board.h"

inline char board_print_piece_char(const Piece& piece) {
    char base = kind_to_char(piece.kind);
    return piece.white ? base : static_cast<char>(std::tolower(base));
}

inline std::ostream& operator<<(std::ostream& os, const Board& board) {
    std::array<std::array<char, 8>, 8> grid{};  // TODO leverage board.occupancy?
    for (auto& row : grid) {
        row.fill('.');
    }
    const int count = board.get_piece_count();
    for (int i = 0; i < count; ++i) {
        const Piece& piece = board.get_piece(i);
        grid[piece.y][piece.x] = board_print_piece_char(piece);
    }

    os << "  +-----------------+\n";
    for (int rank = 7; rank >= 0; --rank) {
        os << rank + 1 << " | ";
        for (int file = 0; file < 8; ++file) {
            os << grid[rank][file] << ' ';
        }
        os << "|\n";
    }
    os << "  +-----------------+\n";
    os << "    a b c d e f g h\n";

    os << "Turn: " << (board.is_white_to_move() ? "White" : "Black");
    os << "\nEn Passant: " << board.get_en_passant().to_string();
    os << "\nCastling: " << board.get_castling_rights();
    return os;
}

#endif // BOARD_PRINT_H

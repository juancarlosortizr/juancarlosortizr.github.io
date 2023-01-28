#include <optional>
#include "square.hpp"

#ifndef CHESS_PIECE_H
#define CHESS_PIECE_H 

enum class Shape {
    QUEEN,
    ROOK,
    BISHOP,
    KNIGHT
};

enum class Owner {
    WHITE,
    BLACK
};

// Encode a colourless chess piece like a Queen on a1
// This does *not* include Kings or Pawns (only Q/B/N/R)
typedef struct Piece {
    Square square;
    Shape shape;

    // Comparison (to e.g. sort a vector)
    bool operator==(const Piece& p) const {
        return (shape == p.shape) && (square == p.square);
    }
    bool operator<(const Piece& p) const {
        return (square < p.square) || 
               (square == p.square && shape < p.shape);
    }
} Piece;

// Encodes a colourful piece/king/pawn, not anywhere specifically on board
typedef struct Figure {
    std::optional<Shape> shape;
    Owner colour;
    bool king;  // If no shape, and this is false, then it's a pawn

    Figure(Shape s, Owner c) : shape{s}, colour{c}, king{false} {}
    Figure(Owner c, bool k) : shape{}, colour{c}, king{k} {}
} Figure;

#endif  // CHESS_PIECE_H
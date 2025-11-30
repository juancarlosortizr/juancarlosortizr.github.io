#ifndef PIECE_H
#define PIECE_H

#include <stdexcept> 

/*
* Piece representation
* Only knows the piece kind, colour, and position.
*/

enum class PieceKind { King, Queen, Rook, Bishop, Knight, Pawn };

// Helper to convert from character to PieceKind and vice-versa
static PieceKind char_to_kind(char c) {
    switch (c) {
        case 'K': return PieceKind::King;
        case 'Q': return PieceKind::Queen;
        case 'R': return PieceKind::Rook;
        case 'B': return PieceKind::Bishop;
        case 'N': return PieceKind::Knight;
        case 'P': return PieceKind::Pawn;
        default:  throw std::runtime_error("Invalid character for PieceKind");
    }
}
static char kind_to_char(PieceKind k) {
    switch (k) {
        case PieceKind::King:   return 'K';
        case PieceKind::Queen:  return 'Q';
        case PieceKind::Rook:   return 'R';
        case PieceKind::Bishop: return 'B';
        case PieceKind::Knight: return 'N';
        case PieceKind::Pawn:   return 'P';
        default:                throw std::runtime_error("Invalid PieceKind");
    }
}

struct Piece {
    int x, y;            // grid coordinates (0..gridSize-1)
    bool white;          // true = white, false = black
    PieceKind kind;      // piece kind

    // Constructors
    Piece(int x_, int y_, bool white_, char kindChar) : x(x_), y(y_), white(white_), kind(char_to_kind(kindChar)) {}
    Piece(int x_, int y_, bool white_, PieceKind kind_) : x(x_), y(y_), white(white_), kind(kind_) {}

    // String representation for debugging
    friend std::ostream& operator<<(std::ostream& os, const Piece& p) {
        os << (p.white ? "W" : "B") << kind_to_char(p.kind) << "@" << "(" << p.x << "," << p.y << ")";
        return os;
    }

    // Compare two pieces
    bool operator<(const Piece& rhs) const {
        if (x != rhs.x) return x < rhs.x;
        if (y != rhs.y) return y < rhs.y;
        if (white != rhs.white) return white < rhs.white;
        return static_cast<int>(kind) < static_cast<int>(rhs.kind);
    }
    bool operator==(const Piece& rhs) const {
        return x == rhs.x && y == rhs.y && white == rhs.white && kind == rhs.kind;
    }
};

#endif // PIECE_H
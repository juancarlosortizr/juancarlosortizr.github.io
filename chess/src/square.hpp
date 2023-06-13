#include <iostream>
#include <string>
#include <utility>

#ifndef CHESS_SQUARE_H
#define CHESS_SQUARE_H

// Encode a chess square in 1 byte
typedef struct Square {
private:
    char encoder;
    bool valid;

    // Translate char to and from (row,rank)
    void encode(char row, int rank) {
        encoder = '0' + (8*(row-'a') + (rank-1));
    }
    std::pair<char, int> decode() const {
        return {(encoder - '0')/8 + 'a', 1 + (encoder - '0') % 8};
    }

public:
    // Interpret 'a', 1
    Square(char row, int rank) : valid{true} {encode(row,rank);}

    // Interpret "a1"
    Square(std::string square) : Square(square.at(0), atoi(&square.at(1))) {}

    // Create invalid squares
    Square() : Square("a1") {valid=false;}

    // Get row and rank
    char row() const {return decode().first;}
    int rank() const {return decode().second;}

    // Enable printing a square to the terminal
    friend std::ostream& operator<<(std::ostream& os, const Square& b);

    // Comparison (to e.g. sort a vector)
    // TODO worry about operator< with invalid squares?
    bool operator==(const Square& s) const {
        return encoder == s.encoder && valid == s.valid;
    }
    bool operator!=(const Square& s) const {return !(*this == s);}
    bool operator<(const Square& s) const {
        return (row() < s.row()) || (row() == s.row() && rank() < s.rank());
    }

    // Return if the other square is diagonally in front (for white)
    // This serves mainly for pawn-capture verification
    // A diagonally in front of B means white's pawn can capture B from A
    // and black's pawn can capture A from B
    bool diagonallyInFront(const Square& s) const {
        return ((row() + 1 == s.row()) || (row() - 1 == s.row())) &&
            (rank() + 1 == s.rank());
    }

    // Return if the other square is directly in front of (for white)
    bool inFront(const Square& s) const {
        return (rank() + 1 == s.rank()) && (row() == s.row());
    }

    // Return square in front. Throws error if this square is on last rank
    Square squareInFront() const {
        if (rank()==8) return Square{};
        return {row(), rank()+1};
    }

    // Return square behind. Throws error if this square is on first rank
    Square squareBehind() const {
        if (rank()==1) return Square{};
        return {row(), rank()-1};
    }

    friend class SquareHash;
} Square;

// Class to hash squares using their encoder;
class SquareHash {
public:
    size_t operator()(const Square& s) const {
        return (size_t)(s.encoder);
    }
};

// Print a square like a1
std::ostream& operator<< (std::ostream& os, const Square& s) {
    os << s.row() << s.rank();
    return os;
}

#endif  // CHESS_SQUARE_H

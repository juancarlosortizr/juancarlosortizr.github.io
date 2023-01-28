#include <string>
#include <ostream>
#include "board.hpp"

#ifndef CHESS_PRINT_H
#define CHESS_PRINT_H

namespace WHITE {
    constexpr char KNIGHT = 'N';
    constexpr char KING = 'K';
    constexpr char QUEEN = 'Q';
    constexpr char BISHOP = 'B';
    constexpr char ROOK = 'R';
    constexpr char PAWN = 'P';
}

namespace BLACK {
    constexpr char KNIGHT = 'n';
    constexpr char KING = 'k';
    constexpr char QUEEN = 'q';
    constexpr char BISHOP = 'b';
    constexpr char ROOK = 'r';
    constexpr char PAWN = 'p';
}

const std::string HORIZONTAL_LINE = "---------------------------------";
constexpr char VERTICAL_LINE = '|';
constexpr char PLACEHOLDER = '.';

// Construct an 8x8 board (char array) from a Board instance
void BoardToArray::print(const Board& b, char board[][8]) {
    for (int row=0; row<8; row++) {
        for (int col=0; col<8; col++) board[row][col] = PLACEHOLDER;
    }
    
    // Kings
    board[b.WKing.row()-'a'][b.WKing.rank()-1] = WHITE::KING;
    board[b.BKing.row()-'a'][b.BKing.rank()-1] = BLACK::KING;

    // Pawns
    for (const Square& s : b.WPawns) {
        board[s.row()-'a'][s.rank()-1] = WHITE::PAWN;
    }
    for (const Square& s : b.BPawns) {
        board[s.row()-'a'][s.rank()-1] = BLACK::PAWN;
    }

    // Pieces
    for (const Piece& p : b.WPieces) {
        char c = PLACEHOLDER;
        switch (p.shape) {
            case Shape::BISHOP : 
                c = WHITE::BISHOP;
                break;
            case Shape::KNIGHT :
                c = WHITE::KNIGHT;
                break;
            case Shape::ROOK :
                c = WHITE::ROOK;
                break;
            case Shape::QUEEN :
                c = WHITE::QUEEN;
                break;
        }
        board[p.square.row()-'a'][p.square.rank()-1] = c;
    }
    for (const Piece& p : b.BPieces) {
        char c = PLACEHOLDER;
        switch (p.shape) {
            case Shape::BISHOP : 
                c = BLACK::BISHOP;
                break;
            case Shape::KNIGHT :
                c = BLACK::KNIGHT;
                break;
            case Shape::ROOK :
                c = BLACK::ROOK;
                break;
            case Shape::QUEEN :
                c = BLACK::QUEEN;
                break;
        }
        board[p.square.row()-'a'][p.square.rank()-1] = c;
    }
    return;
}

// We print the board so the ASCII art coincides
// with the standard view of white-bottom, black-top, left-to-right
std::ostream& operator<<(std::ostream& os, const Board& b) {
    char board[8][8];
    BoardToArray BTA;
    BTA.print(b, board);

    // Print the 8x8 board
    os << HORIZONTAL_LINE << '\n';
    for (int col=7; col>-1; col--) {
        os << VERTICAL_LINE;
        for (int row=0; row<8; row++) os << ' ' << 
            board[row][col] << ' ' << VERTICAL_LINE;
        os << '\n' << HORIZONTAL_LINE << '\n';
    }
    return os;
}

#endif  // CHESS_PRINT_H
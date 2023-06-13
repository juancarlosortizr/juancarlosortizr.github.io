#include <sstream>
#include "stateless_fen.hpp"
#include "state.hpp"

#ifndef CHESS_FEN_H
#define CHESS_FEN_H

constexpr char EXTRA_SEPARATOR = ' ';
constexpr char WHITE_TO_MOVE = 'w';
constexpr char BLACK_TO_MOVE = 'b';
constexpr char NA_SEPARATOR = '-';
constexpr char WHITE_KING_CASTLE = 'K';
constexpr char WHITE_QUEEN_CASTLE = 'Q';
constexpr char BLACK_KING_CASTLE = 'k';
constexpr char BLACK_QUEEN_CASTLE = 'q';

/* 
Complete StatelessFEN class by adding complete info that allows, e.g. to
determine if a threefold repetition has occurred.
Namely, add player to move, castling rights, en passant info, halfmove clock,
and fullmove number.
*/
std::string FEN::repr(const State& s) const {
    StatelessFEN sfen;
    std::stringstream fen;
    sfen(s.b , fen);

    // Colour info
    fen << EXTRA_SEPARATOR;
    if (s.whiteToMove) fen << WHITE_TO_MOVE;
    else {fen << BLACK_TO_MOVE;}

    // Castling info
    fen << EXTRA_SEPARATOR;
    if (s.WKCastle) fen << WHITE_KING_CASTLE;
    if (s.WQCastle) fen << WHITE_QUEEN_CASTLE;
    if (s.BKCastle) fen << BLACK_KING_CASTLE;
    if (s.BQCastle) fen << BLACK_QUEEN_CASTLE;
    if ((!s.WKCastle) && (!s.WQCastle) && (!s.BKCastle) && (!s.BQCastle)) {
        fen << NA_SEPARATOR;
    }

    // En Passant info
    fen << EXTRA_SEPARATOR;
    if (s.enPassantSquare.has_value()) fen << s.enPassantSquare.value();
    else {fen << NA_SEPARATOR;}

    // Halfmoves and fullmoves
    fen << EXTRA_SEPARATOR << s.halfmoves << EXTRA_SEPARATOR << s.fullmoves;
    return fen.str();
};

// For the hash function to work, we convert the string to a size_t by hashing
size_t FEN::operator()(const State& s) const {
    std::cout << "[DELETE] Now printing FEN: " << repr(s) << std::endl;
    return hasher(repr(s));
}

#endif  // CHESS_FEN_H
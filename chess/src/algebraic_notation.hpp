#include <cctype>
#include <optional>
#include <string>
#include "board.hpp"
#include "move.hpp"

#ifndef CHESS_ALGEBRAIC_NOTATION_H
#define CHESS_ALGEBRAIC_NOTATION_H 

constexpr char CAPTURE_CHAR = 'x';
constexpr char KING = 'K';
const std::string KINGSIDE_CASTLING_1 = "O-O";
const std::string KINGSIDE_CASTLING_2 = "0-0";
const std::string QUEENSIDE_CASTLING_1 = "O-O-O";
const std::string QUEENSIDE_CASTLING_2 = "0-0-0";

Shape toShape(char c) {
    if (c=='Q') return Shape::QUEEN;
    if (c=='R') return Shape::ROOK;
    if (c=='B') return Shape::BISHOP;
    if (c=='N') return Shape::KNIGHT;
    throw std::invalid_argument{"Char " +std::string{c}+ " is not a Piece shape (Q/R/B/N)"};
}

// TODO improve whole file (linter, etc)
// TODO implement checks, checkmates, stalemates notation
// TODO implement promotions

// Interpret a meaningful (but not necessarily legal)
// move from algebraic notation
std::optional<Move> readAlgNot(
    const Board* b,
    std::string alg,
    PLAYERTOMOVE ptm
) {
    bool white = (ptm == PLAYERTOMOVE::WHITE);
    
    // Castling
    if (alg == KINGSIDE_CASTLING_1 || alg == KINGSIDE_CASTLING_2) {
        return Move{MOVE::CASTLE, ptm, CASTLING::KINGSIDE};
    }
    if (alg == QUEENSIDE_CASTLING_1 || alg == QUEENSIDE_CASTLING_2) {
        return Move{MOVE::CASTLE, ptm, CASTLING::QUEENSIDE};
    }

    // Simple pawn move (no captures/enpassant/promotions)
    if (alg.size() == 2) {
        Square end{alg};
        const std::vector<Square>* pawns = (white ? &(b->WPawns) : &(b->BPawns));

        // Pawn move one square forward
        Square oneBefore = (white ? end.squareBehind() : end.squareInFront());
        for (size_t idx=0; idx<pawns->size(); idx++) {
            if ((*pawns)[idx] == oneBefore) {
                return Move{MOVE::NORMAL, ptm, oneBefore, end};
            }
        }
        // Pawn move two squares forward
        Square twoBefore = (white ? end.squareBehind().squareBehind() 
            : end.squareInFront().squareInFront());
        if (end.rank() == (white ? 4 : 5)) {
            for (size_t idx=0; idx<pawns->size(); idx++) {
                if ((*pawns)[idx] == twoBefore) {
                    return Move{MOVE::NORMAL, ptm, twoBefore, end};
                }
            }
        }

        return {};
    }

    // Non-promotion pawn capture (including En Passant)
    if (islower(alg.at(0)) && alg.size() == 4 && alg.at(1) == CAPTURE_CHAR) {
        char startRow = alg.at(0);
        Square end{alg.substr(2,2)};
        Square start{startRow, end.rank() + (white ? -1 : 1)};
        if (!b->colourPresent(end).has_value()) return Move{MOVE::ENPASSANT, ptm, start, end};
        return Move{MOVE::CAPTURE, ptm, start, end};
    }

    // Simple piece/king move like Nc3
    if (alg.size() == 3 && isupper(alg.at(0))) {
        char who = alg.at(0);
        Square end{alg.substr(1, 2)};

        // Piece move
        if (who != KING) {
            Shape sh = toShape(who);
            const std::vector<Piece>* pieces = (white ? &(b->WPieces) : &(b->BPieces)); 
            for (size_t idx=0; idx<pieces->size(); idx++) {
                if ((*pieces)[idx].shape != sh) continue;
                if (b->validMotion(Figure{sh, toOwner(ptm)}, (*pieces)[idx].square, end)) {
                    return Move(MOVE::NORMAL, ptm, (*pieces)[idx].square, end);
                }
            }
        } else {
            // King move
            return Move{MOVE::NORMAL, ptm, (white ? b->WKing : b->BKing), end};
        }
    }

    // Piece captures enemy figure
    if (alg.size() == 4 && isupper(alg.at(0)) && alg.at(1) == CAPTURE_CHAR) {
        char who = alg.at(0);
        Square end{alg.substr(2, 2)};

        // Piece move
        if (who != KING) {
            Shape sh = toShape(who);
            const std::vector<Piece>* pieces = (white ? &(b->WPieces) : &(b->BPieces)); 
            for (size_t idx=0; idx<pieces->size(); idx++) {
                if ((*pieces)[idx].shape != sh) continue;
                if (b->validMotion(Figure{sh, toOwner(ptm)}, (*pieces)[idx].square, end)) {
                    return Move(MOVE::CAPTURE, ptm, (*pieces)[idx].square, end);
                }
            }
        } else {
            // King move
            return Move{MOVE::CAPTURE, ptm, (white ? b->WKing : b->BKing), end};
        }
    }

    return {};
}

#endif  // CHESS_ALGEBRAIC_NOTATION_H
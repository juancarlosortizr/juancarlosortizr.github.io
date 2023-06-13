#include <algorithm>
#include <string>
#include <optional>
#include <set>
#include <utility>
#include <unordered_set>
#include <vector>
#include "move.hpp"
#include "piece.hpp"
#include "square.hpp"

#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

// Translate between Owner and PlayerToMove
// TODO necessary?
Owner toOwner(const PLAYERTOMOVE& ptm) {
    return (ptm == PLAYERTOMOVE::WHITE) ? Owner::WHITE : Owner::BLACK;
}
PLAYERTOMOVE toPTM(const Owner& o) {
    return (o == Owner::WHITE) ? PLAYERTOMOVE::WHITE : PLAYERTOMOVE::BLACK;
}

// A single 8x8 board with no game history memory or state
class Board {
    // White pieces
    Square WKing;
    std::vector<Square> WPawns;
    std::vector<Piece> WPieces;

    // Black pieces
    Square BKing;
    std::vector<Square> BPawns;
    std::vector<Piece> BPieces;

    // Redundant info but faster computation:
    // Set of all the unoccupied squares
    std::unordered_set<Square, SquareHash> Occupied;
    // TODO implement? Makes colourPresent() faster
    // Or instead just store the figures() map here instead of in State

    // Return the index of a piece in the corresponding vector,
    // and its colour, if it exists at that square. Else, return null.
    // This only finds non-pawn non-king pieces, as per
    // standard notation throughout this project.
    std::optional<std::pair<size_t, Owner>> getPiece(Square s) const {
        for (size_t idx=0; idx<WPieces.size(); idx++) {
            if (WPieces[idx].square == s) return {{idx, Owner::WHITE}};
        }
        for (size_t idx=0; idx<BPieces.size(); idx++) {
            if (BPieces[idx].square == s) return {{idx, Owner::BLACK}};
        }
        return {};
    }

    // Returns Owner of the king situated on the argument square.
    // If argument square doesn't contain a king, returns null.
    std::optional<Owner> getKing(Square s) const {
        if (WKing == s) return Owner::WHITE;
        if (BKing == s) return Owner::BLACK;
        return {};
    }

    // Returns the index of a pawn in the corresponding vector,
    // and its colour, if it exists at that square. Else return null.
    std::optional<std::pair<size_t, Owner>> getPawn(Square s) const {
        for (size_t idx=0; idx<WPawns.size(); idx++) {
            if (WPawns[idx] == s) return {{idx, Owner::WHITE}};
        }
        for (size_t idx=0; idx<BPawns.size(); idx++) {
            if (BPawns[idx] == s) return {{idx, Owner::BLACK}};
        }
        return {};
    }

    /*
    Six methods for each figure-shape move meaningfulness
    No legality verifications are made regarding checks
    Only whether the motion is valid without jumping over pieces (except
    knights). No checks are made regarding the figure is actually present
    in the start square, nor that the end square is available for that
    figure to move there, except for pawn moves, whose moves change regarding
    what piece is in front of them
    */

    // Checks squares are horizontally/vertically/diagonally adjacent,
    // without verifying for any checks: ONLY verifies the motion makes sense
    bool validKingMotion(Square start, Square end) const {
        std::cout << "Board: Can king move from " << start << " to " << end << std::endl; // TODO delete
        int rowDelta = start.row() - end.row();
        int rankDelta = start.rank() - end.rank();
        std::cout << "Row delta " << rowDelta << ", rank delta " << rankDelta << std::endl; // TODO delete
        return (
            (start != end)
            && (-1 <= rowDelta) && (rowDelta <= 1)
            && (-1 <= rankDelta) && (rankDelta <= 1)
        );
    }

    // Checks squares are horizontally/vertically/diagonally observable,
    // with no pieces in-between to jump over.
    // TODO: modularize each direction, to reuse in Rook and Bishop method?
    bool validQueenMotion(Square start, Square end) const {
        std::cout << "Board? Can queen move from " << start << " to " << end << std::endl; // TODO delete
        if (start==end) return false;
        if (validRookMotion(start, end)) return true;
        return validBishopMotion(start, end);
    }

    // Checks squares are opposite corners of some 2x3 rectangle
    bool validKnightMotion(Square start, Square end) const {
        return (
            (std::abs(start.rank() - end.rank()) == 1 
            && std::abs(start.row() - end.row()) == 2)
            || (std::abs(start.rank() - end.rank()) == 2
            && std::abs(start.row() - end.row()) == 1)
        );
    }

    // Checks squares are diagonally observable,
    // with no pieces in-between to jump over.
    bool validBishopMotion(Square start, Square end) const {
        if (start==end) return false;

        // Diagonal
        if (std::abs(start.row() - end.row())
            == std::abs(start.rank() - end.rank())) {
            std::cout << "Analyzing queen diagonal motion" << std::endl; // TODO delete
            std::vector<Square> squaresToCheck;
            bool northEast = (start.row() - end.row())
                == (start.rank() - end.rank());

            // Collect all squares on the diagonal
            for (int delta=1;
                delta < std::abs(start.rank() - end.rank()); delta++) {
                squaresToCheck.push_back({
                    static_cast<char>(std::min(start.row(), end.row()) + delta),
                    northEast ? std::min(start.rank(), end.rank()) + delta :
                        std::max(start.rank(), end.rank()) - delta
                });
            }

            for (Square s : squaresToCheck) {
                if (colourPresent(Square{s}).has_value()) return false;
            }
            return true;
        }

        return false;
    }

    // Checks squares are horizontally/vertically observable,
    // with no pieces in-between to jump over.
    bool validRookMotion(Square start, Square end) const {
        if (start==end) return false;

        // Horizontal
        if (start.rank() == end.rank()) {
            for (char row = std::min(start.row(), end.row())+1;
                row < std::max(start.row(), end.row()); 
                row++) {
                if (colourPresent(Square{row, start.rank()}).has_value()) {
                    return false;
                }
            }
            return true;
        }

        // Vertical
        if (start.row() == end.row()) {
            for (int rank = std::min(start.rank(), end.rank())+1;
                rank < std::max(start.rank(), end.rank()); 
                rank++) {
                if (colourPresent(Square{start.row(), rank}).has_value()) {
                    return false;
                }
            }
            return true;
        }

        return false;
    }

    // Verifies one of three alternatives:
    // 1. end is in front of start, in owner's direction, and end is empty
    // 2. end is diagonally in front of start, in owner's direction,
    //    and contains an enemy piece
    // 3. end is on 4th rank and start on 2nd rank (in owner's perspective)
    //    and end is empty, and the square in between is also empty
    bool validPawnMotion(Owner o, Square start, Square end) const {
        if (o == Owner::WHITE) {
            if (start.inFront(end)
                && !colourPresent(end).has_value()) return true;
            if (start.diagonallyInFront(end)
                && colourPresent(end).value_or(Owner::WHITE) == Owner::BLACK)
                return true;
            return (
                start.squareInFront().squareInFront() == end
                && !colourPresent(end).has_value()
                && !colourPresent(start.squareInFront())
            );
        } else {
            if (end.inFront(start)
                && !colourPresent(end).has_value()) return true;
            if (end.diagonallyInFront(start)
                && colourPresent(end).value_or(Owner::BLACK) == Owner::WHITE)
                return true;
            return (
                start.squareBehind().squareBehind() == end
                && !colourPresent(end).has_value()
                && !colourPresent(start.squareBehind())
            );
        }
    }

    // Assume a figure exists on start square and that end square is empty.
    // Teletransport figure from start to end square. Do nothing else.
    // If assumptions broken, method has undefined behaviour
    void teletransport(Square start, Square end) {

        // Move kings
        if (WKing == start) {
            WKing = end;
            return;
        }
        if (BKing == start) {
            BKing = end;
            return;
        }

        // Move pawns
        auto pawn = getPawn(start);
        if (pawn.has_value()) {
            if (pawn.value().second == Owner::WHITE) {
                WPawns[pawn.value().first] = end;
            } else {
                BPawns[pawn.value().first] = end;
            }
            return;
        }

        // Move piece
        auto piece = getPiece(start);
        if (piece.has_value()) {
            if (piece.value().second == Owner::WHITE) {
                WPieces[piece.value().first] = Piece{end, WPieces[piece.value().first].shape};
            } else {
                BPieces[piece.value().first] = Piece{end, BPieces[piece.value().first].shape};
            }
            return;
        }

        throw std::invalid_argument{"Cannot teletransport"};
    }

    // Assume a non-king figure exists on the square, then kill it
    // If a king is there, or if the square is empty, this is undefined
    void kill(Square s) {

        // Kill pawns
        auto pawn = getPawn(s);
        if (pawn.has_value()) {
            if (pawn.value().second == Owner::WHITE) {
                WPawns.erase(WPawns.begin() + pawn.value().first);
            } else {
                BPawns.erase(BPawns.begin() + pawn.value().first);
            }
            return;
        }

        // Kill piece
        auto piece = getPiece(s);
        if (piece.has_value()) {
            if (piece.value().second == Owner::WHITE) {
                WPieces.erase(WPieces.begin() + piece.value().first);
            } else {
                BPieces.erase(BPieces.begin() + piece.value().first);
            }
            return;
        }

        throw std::invalid_argument{"Cannot Kill"};
    }
    
public:

    // Initiate standard starting chess position
    Board() :
        // White pieces
        // King
        WKing{"e1"},
        // Pawns
        WPawns{{"a2"},{"b2"},{"c2"},{"d2"},{"e2"},{"f2"},{"g2"},{"h2"}},
        // Pieces
        WPieces{
            // Queen
            {{"d1"}, Shape::QUEEN},
            // Bishops
            {{"c1"}, Shape::BISHOP},
            {{"f1"}, Shape::BISHOP},
            // Knights
            {{"b1"}, Shape::KNIGHT},
            {{"g1"}, Shape::KNIGHT},
            // ROOKS
            {{"a1"}, Shape::ROOK},
            {{"h1"}, Shape::ROOK},
        },
        // Black pieces
        // King
        BKing{"e8"},
        // Pawns
        BPawns{{"a7"},{"b7"},{"c7"},{"d7"},{"e7"},{"f7"},{"g7"},{"h7"}},
        // Pieces
        BPieces{
            // Queen
            {{"d8"}, Shape::QUEEN},
            // Bishops
            {{"c8"}, Shape::BISHOP},
            {{"f8"}, Shape::BISHOP},
            // Knights
            {{"b8"}, Shape::KNIGHT},
            {{"g8"}, Shape::KNIGHT},
            // ROOKS
            {{"a8"}, Shape::ROOK},
            {{"h8"}, Shape::ROOK},
        } {}

    // Compare two board states (without game history)
    bool operator==(const Board& b) const {

        // White pieces equal
        if (WKing != b.WKing) return false;
        std::vector<Square> sortedWPawns {WPawns};
        std::vector<Square> sortedWPawnsB {b.WPawns};
        sort(sortedWPawns.begin(), sortedWPawns.end());
        sort(sortedWPawnsB.begin(), sortedWPawnsB.end());
        if (sortedWPawns != sortedWPawnsB) return false;
        std::vector<Piece> sortedWPieces {WPieces};
        std::vector<Piece> sortedWPiecesB {b.WPieces};
        sort(sortedWPieces.begin(), sortedWPieces.end());
        sort(sortedWPiecesB.begin(), sortedWPiecesB.end());
        if (sortedWPieces != sortedWPiecesB) return false;

        // Black pieces equal
        if (BKing != b.BKing) return false;
        std::vector<Square> sortedBPawns {BPawns};
        std::vector<Square> sortedBPawnsB {b.BPawns};
        sort(sortedBPawns.begin(), sortedBPawns.end());
        sort(sortedBPawnsB.begin(), sortedBPawnsB.end());
        if (sortedBPawns != sortedBPawnsB) return false;
        std::vector<Piece> sortedBPieces {BPieces};
        std::vector<Piece> sortedBPiecesB {b.BPieces};
        sort(sortedBPieces.begin(), sortedBPieces.end());
        sort(sortedBPiecesB.begin(), sortedBPiecesB.end());
        if (sortedBPieces != sortedBPiecesB) return false;
        
        return true;
    }

    // Return whether or not a white/black piece is there
    std::optional<Owner> colourPresent(Square s) const {
        auto p = getPiece(s);
        if (p.has_value()) return p.value().second;
        auto pa = getPawn(s);
        if (pa.has_value()) return pa.value().second;
        auto pk = getKing(s);
        if (pk.has_value()) return pk.value();
        return {};
    }

    // Return whether a certain figure motion is valid
    // without jumping pieces (except for the knight)
    // This method makes no checks as to what kind of figure if any, lies
    // on end square.
    bool validMotion(const Figure& f, Square start, Square end) const {
        if (f.shape.has_value()) {
            // Piece figure (Q/B/N/R)
            switch (f.shape.value()) {
                case Shape::KNIGHT:
                    return validKnightMotion(start, end);
                case Shape::QUEEN:
                    return validQueenMotion(start, end);
                case Shape::BISHOP:
                    return validBishopMotion(start, end);
                case Shape::ROOK:
                    return validRookMotion(start, end);
                default:
                    return false;  // never executes
            }
        }
        std::cout << "Checking valid motion of king/pawn. King? " << f.king << std::endl; // TODO delete
        return (f.king ? validKingMotion(start, end) :
            validPawnMotion(f.colour, start, end));
    }

    // Interpret a meaningful (but not necessarily legal)
    // move from algebraic notation
    // Logic fleshed out in algebraic_notation.hpp
    std::optional<Move> readAlgebraicNotation(
        std::string alg,
        PLAYERTOMOVE ptm
    ) const { 
        // TODO Always use the same alg-not instance for this?
        return readAlgNot(this, alg, ptm);
    }

    // Enable state to access raw board info
    friend class State;
    // Enable printing a board state to the terminal
    friend class BoardToArray;
    // Enable reading algebraic notation
    friend std::optional<Move> readAlgNot(const Board* b_, std::string alg_, PLAYERTOMOVE ptm_);
};

// Print board info to an 8x8 char array
class BoardToArray {
public:
    void print(const Board& b, char board[][8]);
};

// Convert alg-not to moves, and vice-versa
std::optional<Move> readAlgNot(const Board* b, std::string alg, PLAYERTOMOVE ptm);

#endif  // CHESS_BOARD_H
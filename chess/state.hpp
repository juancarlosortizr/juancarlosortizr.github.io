#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include "board.hpp"
#include "move.hpp"

#ifndef CHESS_STATE_H
#define CHESS_STATE_H

// Compare a piece's owner type to a state's player-to-move type
bool operator==(const PLAYERTOMOVE& ptm, const Owner& o) {
    return (ptm == PLAYERTOMOVE::WHITE && o == Owner::WHITE) ||
        (ptm == PLAYERTOMOVE::BLACK && o == Owner::BLACK);
}

/*  
State Class:
------------
Expands on the Board class by adding active color, castling availability
en passant target square (if it exists), halfmove clock (number of
halfmoves since the last capture or pawn advance), fullmove number.

Its only limiting factor is past moves. It does NOT contain sufficient
info to determine a draw by threefold repetition.
*/
class State {
    Board b;
    bool whiteToMove;
    bool WKCastle;  // White Kingside castle available
    bool WQCastle;  // White Queenside castle available
    bool BKCastle;  // Black Kingside castle available
    bool BQCastle;  // Black Queenside castle available
    std::optional<Square> enPassantSquare;
    int halfmoves;
    int fullmoves;

    // Additional info for faster checking which squares have which pieces
    // TODO make unordered map?
    std::map<Square, Figure> figures;

    // Analyze if a castling move is meaningful, i.e. king and rook where
    // they should be, and no pieces in-between. No checks or castling
    // availability are analyzed.
    bool meaningfulCastle(const Move& m) const {
        if (m.type != MOVE::CASTLE) return false;
        if (m.player == PLAYERTOMOVE::WHITE) {
            // King on e1
            if (b.WKing != Square("e1")) return false;
            if (m.side == CASTLING::KINGSIDE) {
                // Rook on h1
                auto rook = figures.find({"h1"});
                if (rook == figures.end()) return false;
                if (rook->second.shape.value_or(Shape::QUEEN)
                    != Shape::ROOK) return false;
                // f1, g1 empty
                return (figures.find({"f1"}) == figures.end()) &&
                    (figures.find({"g1"}) == figures.end());
            } else {
                // Queenside
                // Rook on a1
                auto rook = figures.find({"a1"});
                if (rook == figures.end()) return false;
                if (rook->second.shape.value_or(Shape::QUEEN)
                    != Shape::ROOK) return false;
                // b1, c1, d1 empty
                return (figures.find({"b1"}) == figures.end()) &&
                    (figures.find({"c1"}) == figures.end()) &&
                    (figures.find({"d1"}) == figures.end());
            }
        }
        // Black moves
        // King on e8
        if (b.BKing != Square("e8")) return false;
        if (m.side == CASTLING::KINGSIDE) {
            // Rook on h8
            auto rook = figures.find({"h8"});
            if (rook == figures.end()) return false;
            if (rook->second.shape.value_or(Shape::QUEEN)
                != Shape::ROOK) return false;
            // f8, g8 empty
            return (figures.find({"f8"}) == figures.end()) &&
                (figures.find({"g8"}) == figures.end());
        } else {
            // Queenside
            // Rook on a8
            auto rook = figures.find({"a8"});
            if (rook == figures.end()) return false;
            if (rook->second.shape.value_or(Shape::QUEEN)
                != Shape::ROOK) return false;
            // b8, c8, d8 empty
            return (figures.find({"b8"}) == figures.end()) &&
                (figures.find({"c8"}) == figures.end()) &&
                (figures.find({"d8"}) == figures.end());
        }
    }

    // Check if an en passant moves makes sense
    // End square must be diagonally in front of (according to colour)
    // starting square. Owner's Pawn must be in start square.
    // We don't verify if an opponent pawn just moved where it should have
    // moved, nor that the end square is empty, as these are both guaranteed
    // by legality checks, not meaningfulness checks.
    bool meaningfulEnPassant(const Move& m) const {
        if (m.type != MOVE::ENPASSANT) return false;
        // end square must be diagonally in front
        if (m.player == PLAYERTOMOVE::WHITE) {
            if (!m.start.value().diagonallyInFront(m.end.value())) return false;
        } else {
            if (!m.end.value().diagonallyInFront(m.start.value())) return false;
        }
        // start square must contain correct-colour pawn
        auto pawn = figures.find(m.start.value());
        if (pawn == figures.end()) return false;
        if (pawn->second.shape.has_value() 
            || pawn->second.king) return false;
        return pawn->second.colour == m.player;
    }

    // Check if a capture move makes sense.
    // Player figure must be present in starting square.
    // End square must contain enemy figure.
    // Figure motion must be valid
    bool meaningfulCapture(const Move& m) const {
        if (m.type != MOVE::CAPTURE) return false;

        // Find own piece and enemy piece
        auto mine = figures.find(m.start.value());
        if (mine == figures.end()) return false;
        if (mine->second.colour != m.player) return false;
        auto enemy = figures.find(m.end.value());
        if (enemy == figures.end()) return false;
        if (enemy->second.colour == m.player) return false;

        // Valid motion
        return b.validMotion(mine->second, m.start.value(), m.end.value());
    }

    // Check if a normal move makes sense.
    // Player piece must be present in starting square.
    // End square must be empty.
    // Figure motion must be valid
    bool meaningfulNormalMove(const Move& m) const {
        std::cout << "Entering meaningful normal move" << std::endl; // TODO delete
        if (m.type != MOVE::NORMAL) return false;

        // Find own piece and enemy piece
        auto mine = figures.find(m.start.value());
        if (mine == figures.end()) {
            std::cout << "No piece found at "
                      << m.start.value() << std::endl;
            return false;
        }
        if (mine->second.colour != m.player) {
            std::cout << "Piece found at "<< m.start.value()
                      << " has wrong colour" << std::endl;
            return false;
        }
        if (figures.find(m.end.value()) != figures.end()) {
            std::cout << "Square " << m.end.value()
                      << " not empty" << std::endl;
            return false;
        }

        std::cout << "Normal move passes initial tests, checking board now for validMotion" << std::endl;  // TODO delete

        // Valid motion
        return b.validMotion(mine->second, m.start.value(), m.end.value());
    }

public:
    State() : whiteToMove{true}, WKCastle{true}, WQCastle{true},
              BKCastle{true}, BQCastle{true}, enPassantSquare{},
              halfmoves{0}, fullmoves{1}, figures{} {
        // Insert King figures
        figures.insert({{"e1"}, {Owner::WHITE, true}});
        figures.insert({{"e8"}, {Owner::BLACK, true}});
        // Insert pawns figures 
        for (char c : {'a','b','c','d','e','f','g','h'}) {
            figures.insert({{c, 2}, {Owner::WHITE, false}});
            figures.insert({{c, 7}, {Owner::BLACK, false}});
        }
        // Insert piece figures
        figures.insert({{"d1"}, {Shape::QUEEN, Owner::WHITE}});
        figures.insert({{"d8"}, {Shape::QUEEN, Owner::BLACK}});
        figures.insert({{"c1"}, {Shape::BISHOP, Owner::WHITE}});
        figures.insert({{"c8"}, {Shape::BISHOP, Owner::BLACK}});
        figures.insert({{"b1"}, {Shape::KNIGHT, Owner::WHITE}});
        figures.insert({{"b8"}, {Shape::KNIGHT, Owner::BLACK}});
        figures.insert({{"a1"}, {Shape::ROOK, Owner::WHITE}});
        figures.insert({{"a8"}, {Shape::ROOK, Owner::BLACK}});
        figures.insert({{"f1"}, {Shape::BISHOP, Owner::WHITE}});
        figures.insert({{"f8"}, {Shape::BISHOP, Owner::BLACK}});
        figures.insert({{"g1"}, {Shape::KNIGHT, Owner::WHITE}});
        figures.insert({{"g8"}, {Shape::KNIGHT, Owner::BLACK}});
        figures.insert({{"h1"}, {Shape::ROOK, Owner::WHITE}});
        figures.insert({{"h8"}, {Shape::ROOK, Owner::BLACK}});
    }

    /* Check if a contextless Move makes sense but not necessarily its legality
       A piece must exist there and be able to move to that spot with the given
       move type, but checks or other legality concerns (namely casting
       availability and enPassant availability) are ignored.
       Also, player-to-move must match between the board-state and the move. */
    bool meaningful(const Move& m) const {
        if (whiteToMove && m.player == PLAYERTOMOVE::BLACK) return false;
        if (!whiteToMove && m.player == PLAYERTOMOVE::WHITE) return false;
        switch (m.type) {
            case MOVE::CASTLE:
                return meaningfulCastle(m);
            case MOVE::ENPASSANT:
                return meaningfulEnPassant(m);
            case MOVE::CAPTURE:
                return meaningfulCapture(m);
            case MOVE::NORMAL:
                return meaningfulNormalMove(m);
            default:
                return false;
        }
    }

    /* Check a contextless move is meaningful *AND* legal.
       Extra checks made here are (exhaustively):
        1. Castling availability
        2. Castling through/from/into check
        3. En Passant availability
        4. No moving into check */
    bool legal(const Move& m) {
        if (!meaningful(m)) return false;

        // Verify castling
        if (m.type == MOVE::CASTLE) {

            // Verify castling availabililty
            if (m.side == CASTLING::KINGSIDE) {
                if (m.player == PLAYERTOMOVE::WHITE) {
                    if (!WKCastle) return false;
                } else {
                    if (!BKCastle) return false;
                }
            } else {
                if (m.player == PLAYERTOMOVE::WHITE) {
                    if (!WQCastle) return false;
                } else {
                    if (!BQCastle) return false;
                }
            }

            // Verify no castling from/through/into check
            // TODO implement
            return true;
        }

        // Legality en passant: enemy pawn just moved 2 squares
        // to appropriate square
        if (m.type == MOVE::ENPASSANT) {
            // TODO delete
            std::cout << enPassantSquare.value_or(Square{"a1"}) << std::endl;
            if (!enPassantSquare.has_value()) return false;
            return enPassantSquare.value() == m.end.value();
        }

        // Legality capture: enemy piece there
        // TODO: No moving into check
        if (m.type == MOVE::CAPTURE) {
            return (figures.find(m.end.value()) != figures.end())
                && !(figures.find(m.end.value())->second.colour == m.player);
        }

        // Legality normal move: end square empty
        // TODO: No moving into check
        if (m.type == MOVE::NORMAL) {
            return figures.find(m.end.value()) == figures.end();
        }

        return false;
    }

    // Execute a move in-place from a contextless Move instance
    // Before doing so, verify the move is meaningful and legal
    // If NOT legal, throw an error
    void move(const Move& m) {
        if (!legal(m)) {
            throw std::invalid_argument{"Move not legal"};
        }
        bool enablesEnPassant = false;

        // Castle
        if (m.type == MOVE::CASTLE) {

            // Change king in figures map and board
            Square kingStart{(m.player == PLAYERTOMOVE::WHITE ? "e1" : "e8")};
            Square kingEnd{((m.player == PLAYERTOMOVE::WHITE) ? 
                ((m.side.value() == CASTLING::KINGSIDE) ? "g1" : "c1") :
                ((m.side.value() == CASTLING::KINGSIDE) ? "g8" : "c8")    
            )};
            auto it = figures.find(kingStart);
            figures.erase(it->first);
            figures.insert({kingEnd, Figure{it->second.colour, true}});
            b.teletransport(kingStart, kingEnd);

            // Change rook in figures map
            Square rookStart{((m.player == PLAYERTOMOVE::WHITE) ? 
                ((m.side.value() == CASTLING::KINGSIDE) ? "h1" : "a1") :
                ((m.side.value() == CASTLING::KINGSIDE) ? "h8" : "a8")    
            )};
            Square rookEnd{((m.player == PLAYERTOMOVE::WHITE) ? 
                ((m.side.value() == CASTLING::KINGSIDE) ? "f1" : "d1") :
                ((m.side.value() == CASTLING::KINGSIDE) ? "f8" : "d8")    
            )};
            auto ii = figures.find(rookStart);
            figures.erase(ii->first);
            figures.insert({rookEnd, Figure{Shape::ROOK, ii->second.colour}});
            b.teletransport(rookStart, rookEnd);

            // Forbid future castling for this player
            if (m.player == PLAYERTOMOVE::WHITE) {
                WKCastle = false;
                WQCastle = false;
            } else {
                BKCastle = false;
                BQCastle = false;
            }
        }

        // En Passant
        if (m.type == MOVE::ENPASSANT) {

            // Find enemy pawn
            Square enemy = (m.player == PLAYERTOMOVE::WHITE)
                ? m.end.value().squareBehind() 
                : m.end.value().squareInFront();

            // Change figures map
            auto it = figures.find(m.start.value());
            auto itt = figures.find(enemy);
            figures.erase(itt->first);
            Figure fig = it->second;
            figures.erase(it->first);
            figures.insert({m.end.value(), fig});

            // Change board
            b.kill(enemy);
            b.teletransport(m.start.value(), m.end.value());
        }

        // Capture
        if (m.type == MOVE::CAPTURE) {

            // Change figures map
            auto it = figures.find(m.start.value());
            auto itt = figures.find(m.end.value());
            figures.erase(itt->first);
            Figure fig = it->second;
            figures.erase(it->first);
            figures.insert({m.end.value(), fig});

            // Change board
            b.kill(m.end.value());
            b.teletransport(m.start.value(), m.end.value());

            // If king/rook moved, disable castling
            if ((fig.shape.value_or(Shape::BISHOP) == Shape::ROOK)
                || (!fig.shape.has_value() && fig.king)) {
                if (fig.colour == Owner::WHITE) {
                    WKCastle = false;
                    WQCastle = false;
                } else {
                    BKCastle = false;
                    BQCastle = false;
                }
            }
        }

        // Normal move
        if (m.type == MOVE::NORMAL) {

            // Change figures map
            auto it = figures.find(m.start.value());
            Figure fig = it->second;
            figures.erase(it->first);
            figures.insert({m.end.value(), fig});

            // Change board
            b.teletransport(m.start.value(), m.end.value());

            // Check if move enables en passant
            if ((!fig.shape.has_value()) && (!fig.king)
                && (((m.start.value().squareInFront().squareInFront() == m.end.value()) && (m.player == PLAYERTOMOVE::WHITE))
                || ((m.start.value().squareBehind().squareBehind() == m.end.value()) && (m.player == PLAYERTOMOVE::BLACK)))) {
                enablesEnPassant = true;
            }

            // If king/rook moved, disable castling
            if ((fig.shape.value_or(Shape::BISHOP) == Shape::ROOK)
                || (!fig.shape.has_value() && fig.king)) {
                if (fig.colour == Owner::WHITE) {
                    WKCastle = false;
                    WQCastle = false;
                } else {
                    BKCastle = false;
                    BQCastle = false;
                }
            }
        }

        // Update state
        whiteToMove = !whiteToMove;
        if (!enablesEnPassant) enPassantSquare = std::optional<Square>{};
        else {
            enPassantSquare = Square{
                (m.player == PLAYERTOMOVE::WHITE) ? m.start.value().squareInFront() : m.start.value().squareBehind()
            };
        }
        halfmoves++;
        if (m.player == PLAYERTOMOVE::BLACK) {fullmoves++;}
        return;
    }

    // Get state's player to move
    PLAYERTOMOVE playerToMove() const {
        return whiteToMove ? PLAYERTOMOVE::WHITE : PLAYERTOMOVE::BLACK;
    }

    friend class Game;
    friend class FEN;
    friend std::ostream& operator<<(std::ostream& os, const State& s);
};

class FEN {
    std::hash<std::string> hasher;
public:
    std::string repr(const State& s) const;
    size_t operator()(const State& s) const;
};

std::ostream& operator<<(std::ostream& os, const State& s) {
    os << s.b;
    return os;
}

#endif  // CHESS_STATE_H
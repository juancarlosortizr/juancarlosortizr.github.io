#include <optional>
#include <stdexcept>
#include "square.hpp"

#ifndef CHESS_MOVE_H
#define CHESS_MOVE_H

enum class MOVE {
    CASTLE,
    ENPASSANT,
    CAPTURE,
    NORMAL
};

enum class PLAYERTOMOVE {
    WHITE,
    BLACK
};

// Print White or Black
std::ostream& operator<< (std::ostream& os, const PLAYERTOMOVE& ptm) {
    os << ((ptm == PLAYERTOMOVE::WHITE) ? "White" : "Black");
    return os;
}

enum class CASTLING {
    KINGSIDE,
    QUEENSIDE,
};

/*
Class to represent context-less move. No board or state info is given.
Instead, the move is contextless, and could be illegal.
The information given is who moves, whether it's a castling (in which case
the remaining info is rendered irrelevant), whether it's en passant,
whether it's a capture (forcefully true if en passant), and the starting
square and ending square of the moving piece (which are present only if
the move is not castling).

Notice that it's impossible to instantiate a Move with algebraic notation,
as it doesn't have board visibility.
*/
class Move {
    MOVE type;
    PLAYERTOMOVE player;
    std::optional<CASTLING> side;  // present only for castling moves
    std::optional<Square> start;  // present only for non-castling moves
    std::optional<Square> end;  // present only for non-castling moves
public:
    // Constructor for a castling move
    Move(MOVE t, PLAYERTOMOVE p, CASTLING k) :
        type{t}, player{p}, side{k}, start{}, end{} {
        if (t!=MOVE::CASTLE) throw std::invalid_argument(
            "Castling move must be instantiated with MOVE::CASTLE type"
        );
    }

    // Constructor for a non-castling move
    Move (MOVE t, PLAYERTOMOVE p, Square s, Square e) :
        type{t}, player{p}, side{}, start{s}, end{e} {
        if (t==MOVE::CASTLE) throw std::invalid_argument(
            "Non-castling move must be instantiated with MOVE::CAPTURE, \
            ENPASSANT, or NORMAL type"
        );
    }

    friend class State;
};

#endif  // CHESS_MOVE_H
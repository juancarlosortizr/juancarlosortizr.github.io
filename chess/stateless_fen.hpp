#include <sstream>
#include "board.hpp"
#include "print.hpp"

#ifndef CHESS_STATELESS_FEN_H
#define CHESS_STATELESS_FEN_H

constexpr char ROW_SEPARATOR = '/';

/* 
Hash function to store board state history in unordered_set
Use FEN string representation, excluding the second part of FEN
which contains game state (e.g. castling availability, or colour to move).
Example: Start at standard starting state:
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR
1. e4
rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR
1. ...c5
rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR
2. Nf3
rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R

Three-Move Rule:
Two positions are by definition "the same" if the same types of pieces occupy
the same squares, the same player has the move, the remaining castling rights
are the same and the possibility to capture en passant is the same. The
repeated positions need not occur in succession.
*/
class StatelessFEN {
public:
    void operator()(const Board& b, std::stringstream& sfen) const {
        char board[8][8];
        BoardToArray BTA;
        BTA.print(b, board);

        for (int col=7; col>-1; col--) {
            if (col!=7) sfen << ROW_SEPARATOR;

            // Read row, e.g. "pP...Q.." -> "pP3Q2"
            int count_since_last_piece = 0;
            for (int row=0; row<8; row++) {
                if (board[row][col] == PLACEHOLDER) count_since_last_piece++;
                else {
                    if (count_since_last_piece > 0) {
                        sfen << count_since_last_piece;
                        count_since_last_piece = 0;
                    } else {
                        sfen << board[row][col];
                    }
                }
            }
            if (count_since_last_piece > 0) sfen << count_since_last_piece;
        }

        return;
    }
};

#endif  // CHESS_STATELESS_FEN_H
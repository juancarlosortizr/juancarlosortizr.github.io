#ifndef MOVE_H
#define MOVE_H

#include <sstream>
#include <optional>
#include <set>
#include <utility>
#include "piece.h"
#include "board.h"

/*
* class Move
*
* Barebones struct, knows the move details and nothing else.
* Does NOT store the board after initialization.
*/

class Move {
private:
    const int fromX, fromY, toX, toY;
    std::optional<PieceKind> promote_to;  // to which piece are we promoting
    const bool attempted_castling;
    const bool attempted_kingside_castling;
    const bool attempted_capture_or_pawn_move;  // for 50-move rule (halfmove clock)
    const bool attempted_capture;
    const bool attempted_promotion;
    const bool attempted_en_passant;
    const bool attempted_initial_2_square_pawn_move;
    const bool valid;  // is the move valid? (not necessarily legal)
    const bool white;  // who is moving?

    // NOTE:
    // promote_to is not constant because we can create a promotion move without this field
    // to see if it would be legal regardless of promotion choice. If legal, we can then ask
    // the user for promotion choice. Otherwise, no need, reject the move immediately.

    static bool compute_attempted_capture_or_pawn_move(const Board& board, int fromX, int fromY, int toX, int toY) {
        int from_idx = board.find_piece_at(fromX, fromY);
        if (from_idx != -1 && board.get_piece(from_idx).kind == PieceKind::Pawn) return true;
        int to_idx = board.find_piece_at(toX, toY);
        return (to_idx != -1);
    }

    static bool compute_attempted_en_passant(const Board& board, int fromX, int fromY, int toX, int toY) {
        int from_idx = board.find_piece_at(fromX, fromY);
        if (from_idx == -1) return false;
        if (board.get_piece(from_idx).kind != PieceKind::Pawn) return false;
        int to_idx = board.find_piece_at(toX, toY);
        if (to_idx != -1) return false;
        if (abs(toX - fromX) != 1) return false;
        int dir = (board.get_piece(from_idx).white) ? 1 : -1;
        if (toY != fromY + dir) return false;
        if (dir == 1) {
            if (fromY != 4) return false;
        } else {
            if (fromY != 3) return false;
        }
        if (board.get_piece(from_idx).white && toY - fromY == 1) return true;
        if (!board.get_piece(from_idx).white && toY - fromY == -1) return true;
        return false;
    }

    static bool compute_attempted_capture(const Board& board, int fromX, int fromY, int toX, int toY) {
        if (board.find_piece_at(toX, toY) != -1) return true;
        return compute_attempted_en_passant(board, fromX, fromY, toX, toY);
    }

    static bool compute_attempted_castling(const Board& board, int fromX, int fromY, int toX, int toY) {
        int from_idx = board.find_piece_at(fromX, fromY);
        if (from_idx == -1) return false;
        if (board.get_piece(from_idx).kind != PieceKind::King) return false;
        if (fromY != toY) return false;
        return abs(toX - fromX) == 2;
    }

    static bool compute_attempted_kingside_castling(const Board& board, int fromX, int fromY, int toX, int toY) {
        if (!compute_attempted_castling(board, fromX, fromY, toX, toY)) return false;
        return (fromX < toX);
    }

    static bool compute_attempted_promotion(const Board& board, int fromX, int fromY, int toX, int toY) {
        int from_idx = board.find_piece_at(fromX, fromY);
        if (from_idx == -1) return false;
        if (board.get_piece(from_idx).kind != PieceKind::Pawn) return false;
        if (board.get_piece(from_idx).white && toY == 7) return true;
        if (!board.get_piece(from_idx).white && toY == 0) return true;
        return false;
    }

    static bool compute_attempted_initial_2_square_pawn_move(const Board& board, int fromX, int fromY, int toX, int toY) {
        int from_idx = board.find_piece_at(fromX, fromY);
        if (from_idx == -1) return false;
        if (board.get_piece(from_idx).kind != PieceKind::Pawn) return false;
        if (board.get_piece(from_idx).white) {
            if (fromY == 1 && toY == 3 && fromX == toX) return true;
        } else {
            if (fromY == 6 && toY == 4 && fromX == toX) return true;
        }
        return false;
    }

    static bool compute_white(const Board& board) {
        return board.is_white_to_move();
    }

    static bool compute_valid(const Board& board, int fromX, int fromY, int toX, int toY) {
        const int idx = board.find_piece_at(fromX, fromY);
        if (idx == -1) return false;
        const Piece p = board.get_piece(idx);
        if (p.white != compute_white(board)) return false;
        std::set<std::pair<int,int>> targets = board.get_targets(fromX, fromY);
        return targets.find({toX, toY}) != targets.end();
    }

    void validate_attempted_move_flags() const {
        if (attempted_castling) {
            if (attempted_promotion || attempted_en_passant || attempted_initial_2_square_pawn_move) {
                throw std::runtime_error("Invalid castling flag combination");
            }
        }
        if (attempted_capture) {
            if (!attempted_capture_or_pawn_move) {
                throw std::runtime_error("Invalid capture flag combination");
            }
        }
        if (attempted_promotion) {
            if (attempted_castling || attempted_initial_2_square_pawn_move || attempted_en_passant) {
                throw std::runtime_error("Invalid promotion flag combination");
            }
        }
        if (attempted_en_passant) {
            if (!attempted_capture_or_pawn_move || !attempted_capture || attempted_initial_2_square_pawn_move
            || attempted_promotion || attempted_castling) {
                throw std::runtime_error("Invalid en-passant flag combination");
            }
        }
        if (attempted_initial_2_square_pawn_move) {
            if (attempted_castling || attempted_promotion || attempted_en_passant) {
                throw std::runtime_error("Invalid initial 2-square pann move flag combination");
            }
        }
    }

    std::string compute_notation(const Board& board) const;

public:
    Move(int fx, int fy, int tx, int ty, const Board& board)
        : fromX(fx), fromY(fy), toX(tx), toY(ty),
          promote_to(std::nullopt),
          attempted_castling(compute_attempted_castling(board, fx, fy, tx, ty)),
          attempted_kingside_castling(compute_attempted_kingside_castling(board, fx, fy, tx, ty)),
          attempted_capture_or_pawn_move(compute_attempted_capture_or_pawn_move(board, fx, fy, tx, ty)),
          attempted_capture(compute_attempted_capture(board, fx, fy, tx, ty)),
          attempted_promotion(compute_attempted_promotion(board, fx, fy, tx, ty)),
          attempted_en_passant(compute_attempted_en_passant(board, fx, fy, tx, ty)),
          attempted_initial_2_square_pawn_move(compute_attempted_initial_2_square_pawn_move(board, fx, fy, tx, ty)),
          valid(compute_valid(board, fx, fy, tx, ty)),
          white(compute_white(board))
    {
        validate_attempted_move_flags();
    }

    // Promo helpers
    void set_promotion(PieceKind promo) { promote_to = promo; }
    bool has_promotion() const { return promote_to.has_value(); }
    PieceKind get_promotion() const {
        if (!promote_to.has_value()) {
            throw std::runtime_error("get_promotion: no promotion set");
        }
        return promote_to.value();
    }

    // "Attempted" helpers
    bool is_attempted_capture_or_pawn_move(void) const { return attempted_capture_or_pawn_move; }
    bool is_attempted_capture(void) const { return attempted_capture; }
    bool is_attempted_castling(void) const { return attempted_castling; }
    bool is_attempted_kingside_castling(void) const { return attempted_kingside_castling; }
    bool is_attempted_initial_two_square_pawn_move(void) const { return attempted_initial_2_square_pawn_move; }
    bool is_attempted_promotion(void) const { return attempted_promotion; }
    bool is_attempted_en_passant(void) const { return attempted_en_passant; }
    // Other helpers
    bool is_valid(void) const { return valid; }
    int from_x() const { return fromX; }
    int from_y() const { return fromY; }
    int to_x() const { return toX; }
    int to_y() const { return toY; }
    bool is_a_white_move() const { return white; }

    // String representation for debugging
    friend std::ostream& operator<<(std::ostream& os, const Move& m) {
        os << (m.white ? 'W' : 'B');
        os << "[" << m.fromX << "," << m.fromY << "] --> [" << m.toX << "," << m.toY << "]";
        if (m.promote_to.has_value()) {
            os << " promote to " << kind_to_char(m.promote_to.value());
        }
        if (!m.valid) {
            os << " INVALID";
        }
        return os;
    }

    friend bool operator==(const Move& lhs, const Move& rhs) {
        return lhs.fromX == rhs.fromX &&
               lhs.fromY == rhs.fromY &&
               lhs.toX == rhs.toX &&
               lhs.toY == rhs.toY &&
               lhs.white == rhs.white &&
               lhs.promote_to == rhs.promote_to &&
               lhs.attempted_castling == rhs.attempted_castling &&
               lhs.attempted_kingside_castling == rhs.attempted_kingside_castling &&
               lhs.attempted_capture_or_pawn_move == rhs.attempted_capture_or_pawn_move &&
               lhs.attempted_capture == rhs.attempted_capture &&
               lhs.attempted_promotion == rhs.attempted_promotion &&
               lhs.attempted_en_passant == rhs.attempted_en_passant &&
               lhs.attempted_initial_2_square_pawn_move == rhs.attempted_initial_2_square_pawn_move &&
               lhs.valid == rhs.valid;
    }
};

#endif // MOVES_H

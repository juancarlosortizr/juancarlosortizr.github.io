#ifndef GAME_H
#define GAME_H

#include <vector>
#include <sstream>
#include <optional>
#include <memory>
#include <set>
#include <iostream> 
#include <stdexcept>
#include "piece.h"
#include "board.h"
#include "en_passant.h"
#include "castling.h"
#include "lawyer.h"
#include "move.h"

struct Game {
private:
    Board board_;
    GameStatus status_;
    GameWinner winner_;
    std::vector<Board> undo_stack_;
    std::vector<Board> redo_stack_;
    int halfmove_clock_ = 0;
    std::vector<int> undo_halfmove_clock_;
    std::vector<int> redo_halfmove_clock_;

    // Update Game Status and Game Winner
    void update_outcome(void) {
        Lawyer& lawyer = Lawyer::instance();
        status_ = lawyer.game_status(board_, undo_stack_, halfmove_clock_);
        if (status_ == GameStatus::Checkmate) {
            winner_ = board_.is_white_to_move() ? GameWinner::Black : GameWinner::White;
        } else if (status_ == GameStatus::Ongoing) {
            winner_ = GameWinner::TBD;
        } else {
            winner_ = GameWinner::Draw;
        }
    }

public:
    Game() : board_(), status_(GameStatus::Ongoing), winner_(GameWinner::TBD),
             undo_stack_(), redo_stack_(), halfmove_clock_(0),
             undo_halfmove_clock_(), redo_halfmove_clock_() {
        board_.reset();
    }

    void reset() {
        board_.reset();
        status_ = GameStatus::Ongoing;
        winner_ = GameWinner::TBD;
        undo_stack_.clear();
        redo_stack_.clear();
        undo_halfmove_clock_.clear();
        redo_halfmove_clock_.clear();
        halfmove_clock_ = 0;
    }

    const Board& board() const { return board_; }
    int get_halfmove_clock() const { return halfmove_clock_; }
    GameStatus status() const { return status_; }
    GameWinner winner() const { return winner_; }

    // -1 if illegal, -2 if invalid, 0 if OK
    int verify(const Move& attempted) {
        if (!attempted.is_valid()) {
            return -2;
        }
        if (attempted.is_attempted_promotion()) {
            if (!attempted.has_promotion()) {
                return -2;
            }
        } else if (attempted.has_promotion()) {
            return -2;
        }
        Lawyer& lawyer = Lawyer::instance();
        if (!lawyer.legal(board_, attempted)) {
            return -1;
        }
        return 0;
    }

    // Make a move, but firsy verify if it's valid and legal first (if not then don't make it). 
    // Return -2 if invalid, -1 if illegal, 0 if OK.
    int verify_and_move(const Move& attempted) {
        // Basic checks
        if (status_ != GameStatus::Ongoing) {
            return -2;
        }
        if (winner_ != GameWinner::TBD) {
            return -2;
        }
        if (!in_bounds(attempted.from_x(), attempted.from_y()) || !in_bounds(attempted.to_x(), attempted.to_y())) {
            return -2;
        }
        if (attempted.is_attempted_promotion()) {
            if (!attempted.has_promotion()) {
                return -2;
            }
        } else if (attempted.has_promotion()) {
            return -2;
        }

        // Validity and legality checks
        int ok = verify(attempted);
        if (ok != 0) return ok;
    
        // Move OK, let's perform it!
        undo_stack_.push_back(board_);  // Copy board
        undo_halfmove_clock_.push_back(halfmove_clock_);
        Lawyer& lawyer = Lawyer::instance();
        lawyer.perform_move(board_, attempted);
        redo_stack_.clear();
        redo_halfmove_clock_.clear();
        if (attempted.is_attempted_capture_or_pawn_move()) {
            halfmove_clock_ = 0;
        } else {
            ++halfmove_clock_;
        }
        update_outcome();
        return 0;
    }
    // This just creates a Move object and then uses the above API.
    int verify_and_move(int fromX, int fromY, int toX, int toY, std::optional<PieceKind> promotion = std::nullopt) {
        Move attempted(fromX, fromY, toX, toY, board_);
        if (promotion.has_value()) {
            attempted.set_promotion(promotion.value());
        }
        return verify_and_move(attempted);
    }

    // Undo the most recent move (if any). Returns true if a move was undone.
    bool undo(void) {
        if (undo_stack_.empty()) { return false; }
        // Load current board to redo stack (so redo can restore it)
        redo_stack_.push_back(std::move(board_));
        redo_halfmove_clock_.push_back(halfmove_clock_);
        // Load most recent past state into current board
        board_ = std::move(undo_stack_.back());
        undo_stack_.pop_back();
        halfmove_clock_ = undo_halfmove_clock_.back();
        undo_halfmove_clock_.pop_back();
        update_outcome();
        return true;
    }

    // Redo the most recent move (if any). Returns true if a move was redone.
    bool redo(void) {
        if (redo_stack_.empty()) { return false ; }
        // Load current board to undo stack (so undo can restore it)
        undo_stack_.push_back(std::move(board_));
        undo_halfmove_clock_.push_back(halfmove_clock_);
        // Load closest future state into current board
        board_ = std::move(redo_stack_.back());
        redo_stack_.pop_back();
        halfmove_clock_ = redo_halfmove_clock_.back();
        redo_halfmove_clock_.pop_back();
        update_outcome();
        return true;
    }
};

#endif // GAME_H

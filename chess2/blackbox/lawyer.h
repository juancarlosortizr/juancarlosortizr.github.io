#ifndef LAWYER_H
#define LAWYER_H

#include <sstream>
#include <set>
#include <utility>
#include <sstream> 
#include <stdexcept>
#include <memory>
#include "piece.h"
#include "board.h"
#include "move.h"
#include "castling.h"
#include "en_passant.h"

constexpr int PROMO_OPTIONS = 4;

static constexpr PieceKind promoKinds[PROMO_OPTIONS] = {
    PieceKind::Queen,
    PieceKind::Rook,
    PieceKind::Bishop,
    PieceKind::Knight
};

// TODO implement three-fold draw
enum class GameStatus { Checkmate, Stalemate, Ongoing, FiftyMoveRule, ThreefoldRepetition };
enum class GameWinner { White, Black, Draw, TBD };

/*
* singleton class Lawyer
* Everything to do with king safety:
* - check for checks
* - move legality (leave king in check, castle through/from/into check, walk into check, discover a check, etc)
* - check for checkmates
* - check for stalemates
* This class is also capacitated to actually PERFORM the moves on a live board.
* No other part of the code should do this.
*
* NOTE:
* To check for legality, one must check that the king's square after moving,
* (and the squares it passes through/moves from during castling) are not under attack.
* To check under-attack status, one must generate all possible VALID moves for the opponent
* and see if any of them attack the aforementioned square(s).
* This is why validity is an important concept, separate from legality.
* None of those moves would be legal, as capturing a king never actually happens, but they are valid.
*/

class Lawyer {
public:

    // Access the single instance
    static Lawyer& instance() {
        static Lawyer instance;
        return instance;
    }

private:

    // If true, we can perform illegal (but valid) moves. Mostly used for checking legality.
    bool in_simulation = false;

    Lawyer() = default;                    // private constructor
    ~Lawyer() = default;           // private destructor
    Lawyer(const Lawyer&) = delete;      // disable copy
    Lawyer& operator=(const Lawyer&) = delete;
    Lawyer(Lawyer&&) = delete;           // disable move
    Lawyer& operator=(Lawyer&&) = delete;

public:

    /*
    * Perform a move
    */
    void perform_move(Board& board, const Move& move) {
        if (!in_simulation) {
            if (!legal(board, move)) {
                throw std::runtime_error("Cannot perform illegal move");
            }
        }
        const int fromX = move.from_x();
        const int fromY = move.from_y();
        const int toX = move.to_x();
        const int toY = move.to_y();
        int from_idx = board.find_piece_at(fromX, fromY);
        if (from_idx == -1) {
            throw std::runtime_error("Lawyer::simulate_move: mover piece not found");
        }
        int to_idx = board.find_piece_at(toX, toY);
        const Piece mover = board.get_piece(from_idx);
        CastlingRights cr = board.get_castling_rights();  // copy

        if (move.is_attempted_castling()) {
            const bool kingside = toX > fromX;
            const int rook_from_x = kingside ? 7 : 0;
            const int rook_to_x = kingside ? (fromX + 1) : (fromX - 1);
            const int rook_idx = board.find_piece_at(rook_from_x, fromY);
            if (rook_idx == -1) {
                throw std::runtime_error("Lawyer::simulate_move: rook missing for castling");
            }
            // King moved later, rook moved now
            board.teletransport_piece(rook_idx, rook_to_x, fromY);
            if (mover.white) {
                cr.white_kingside = false;
                cr.white_queenside = false;
            } else {
                cr.black_kingside = false;
                cr.black_queenside = false;
            }
        } else if (move.is_attempted_en_passant()) {
            const int capture_idx = board.find_piece_at(toX, fromY);
            if (capture_idx == -1) {
                throw std::runtime_error("Lawyer::simulate_move: en-passant capture target missing");
            }
            const bool adjust_from = capture_idx < from_idx;
            board.delete_piece(capture_idx);
            if (adjust_from) --from_idx;
        } else if (move.is_attempted_capture() && to_idx != -1) {
            const Piece& captured = board.get_piece(to_idx);
            cr.revoke_for_rook(captured);
            const bool adjust_from = to_idx < from_idx;
            board.delete_piece(to_idx);
            if (adjust_from) --from_idx;
        }

        if (move.is_attempted_promotion()) {
            if (!move.has_promotion()) {
                throw std::runtime_error("Lawyer::simulate_move: promotion target not provided");
            }
            board.change_pawn_kind(from_idx, move.get_promotion());
        }

        board.teletransport_piece(from_idx, toX, toY);

        if (move.is_attempted_initial_two_square_pawn_move()) {
            const int direction = mover.white ? 1 : -1;
            const EnPassantVulnerable vulnerable = mover.white ?
                EnPassantVulnerable::White : EnPassantVulnerable::Black;
            board.set_en_passant(EnPassant(fromX, fromY + direction, vulnerable));
        } else {
            board.clear_en_passant();
        }

        if (mover.kind == PieceKind::King) {
            if (mover.white) {
                cr.white_kingside = false;
                cr.white_queenside = false;
            } else {
                cr.black_kingside = false;
                cr.black_queenside = false;
            }
        } else if (mover.kind == PieceKind::Rook) {
            cr.revoke_for_rook(mover);
        }

        board.set_castling(cr);

        if (move.is_attempted_capture_or_pawn_move()) {
            board.reset_halfmove_clock();
        } else {
            board.increase_halfmove_clock();
        }

        board.toggle_white_to_move();
   }

public:
    // Verify if the move is legal
    bool legal(const Board& board, const Move& move) {
        if (!move.is_valid()) return false;
        // Cannot castle from check
        if (move.is_attempted_castling() && board.is_player_in_check(move.is_a_white_move())) return false;
        // Cannot castle through check
        if (move.is_attempted_castling()) {
            auto [midpoint_x, midpoint_y] = board._midpoint_castling(move.is_a_white_move(), move.is_attempted_kingside_castling());
            if (board._is_under_attack(!move.is_a_white_move(), midpoint_x, midpoint_y)) return false;
        }
    
        // Simulate the move and see if the player who moved is left in check afterwards
        Board sim = board;
        in_simulation = true;
        sim.reset_halfmove_clock();  // For purposes of simulating, we don't need to rememver the fifty move clock.
        perform_move(sim, move);
        const bool output = !(sim.is_player_in_check(move.is_a_white_move()));
        in_simulation = false;
        return output;
    }

    // Verify if an attempted promotion would be legal without actually setting the promotion piece
    bool attempted_promotion_would_be_legal(const Board& board, const Move& attempted_promotion) {
        if (!attempted_promotion.is_attempted_promotion()) {
            throw std::runtime_error("Move is not attempted promotion");
        }
        Move move = attempted_promotion;
        // If it would be legal, it would also be legal by promoting to Queen
        move.set_promotion(PieceKind::Queen);
        return legal(board, move);
    }

    // Verify if the game has ended because the player-to-move has no legal moves
    // This can be because of a stalemate or checkmate
    // If at least one legal move, the game continues
    // The history should NOT include this board itself.
    GameStatus game_status(const Board& board, const std::vector<Board>& history) {
        const bool white_to_move = board.is_white_to_move();
        const bool player_to_move_in_check = board.is_player_in_check(white_to_move);
        const int piece_count = board.get_piece_count();

        // Determine if the board is 3-fold repetition
        int repeats = 0;
        for (const Board& past : history) {
            if (board == past) ++repeats;
        }
        if (repeats >= 3) throw std::runtime_error("Fourfold (or more) repetition found; you didn't catch a draw");

        // Verify if there are legal moves available
        for (int idx = 0; idx < piece_count; ++idx) {
            const Piece& mover = board.get_piece(idx);
            if (mover.white != white_to_move) continue;
            const auto targets = board.get_targets(idx);
            for (const auto& target : targets) {
                Move candidate(mover.x, mover.y, target.first, target.second, board);
                if (!candidate.is_valid()) continue;
                if (candidate.is_attempted_promotion() && !candidate.has_promotion()) {
                    // If a promotion is legal, it will also be legal by promoting to a queen
                    candidate.set_promotion(PieceKind::Queen);
                }
                if (legal(board, candidate)) {
                    if (repeats >= 2) { return GameStatus::ThreefoldRepetition; }
                    if (board.get_halfmove_clock() >= FIFTY_MOVE_RULE_LIMIT) { return GameStatus::FiftyMoveRule; }
                    return GameStatus::Ongoing;
                }
            }
        }
        
        // No legal moves available
        return (player_to_move_in_check ? GameStatus::Checkmate : GameStatus::Stalemate);
    }

    /* 
    * TODO Move to DFS
    */
    bool has_mate_in_one(const Board& board) {
        const bool white_to_move = board.is_white_to_move();
        const int piece_count = board.get_piece_count();
        auto leads_to_checkmate = [&](const Move& candidate) -> bool {
            if (!candidate.is_valid()) {
                return false;
            }
            if (!legal(board, candidate)) {
                return false;
            }
            // The following is NOT a simulation, since it still requires legal moves.
            Board hypoth(board);
            perform_move(hypoth, candidate);
            return game_status(hypoth, std::vector<Board>{}) == GameStatus::Checkmate;
        };
        for (int idx = 0; idx < piece_count; ++idx) {
            const Piece& mover = board.get_piece(idx);
            if (mover.white != white_to_move) {
                continue;
            }
            const auto targets = board.get_targets(idx);
            for (const auto& target : targets) {
                Move move(mover.x, mover.y, target.first, target.second, board);
                if (move.is_attempted_promotion() && !move.has_promotion()) {
                    for (PieceKind promo : promoKinds) {
                        Move promo_move(mover.x, mover.y, target.first, target.second, board);
                        promo_move.set_promotion(promo);
                        if (leads_to_checkmate(promo_move)) {
                            return true;
                        }
                    }
                } else if (leads_to_checkmate(move)) {
                    return true;
                }
            }
        }
        return false;
    }
};

#endif // LAWYER_H

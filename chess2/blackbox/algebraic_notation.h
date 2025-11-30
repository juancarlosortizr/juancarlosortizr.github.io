#ifndef ALGEBRAIC_NOTATION_H
#define ALGEBRAIC_NOTATION_H

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include "lawyer.h"
#include "square_utils.h"

namespace algebraic_notation_detail {

using square_utils::file_char;
using square_utils::rank_char;
using square_utils::square_to_string;

inline std::string disambiguate_piece(const Board& board, const Move& move, const Piece& mover) {
    if (mover.kind == PieceKind::Pawn) return "";

    const std::pair<int, int> target{move.to_x(), move.to_y()};
    const int piece_count = board.get_piece_count();
    bool conflict_found = false;
    bool file_unique = true;
    bool rank_unique = true;

    for (int idx = 0; idx < piece_count; ++idx) {
        const Piece& candidate = board.get_piece(idx);
        if (candidate.white != mover.white) continue;
        if (candidate.kind != mover.kind) continue;
        if (candidate.x == mover.x && candidate.y == mover.y) continue;
        const auto targets = board.get_targets(idx);
        if (targets.find(target) == targets.end()) continue;
        Move candidate_move(candidate.x, candidate.y, target.first, target.second, board);
        if (!Lawyer::instance().legal(board, candidate_move)) continue;
        conflict_found = true;
        if (candidate.x == mover.x) file_unique = false;
        if (candidate.y == mover.y) rank_unique = false;
    }

    if (!conflict_found) return "";

    std::string output;
    if (file_unique) {
        output.push_back(file_char(mover.x));
    } else if (rank_unique) {
        output.push_back(rank_char(mover.y));
    } else {
        output.push_back(file_char(mover.x));
        output.push_back(rank_char(mover.y));
    }
    return output;
}

inline Board board_after_move(const Board& board, const Move& move) {
    Board result(board);
    const int fromX = move.from_x();
    const int fromY = move.from_y();
    const int toX = move.to_x();
    const int toY = move.to_y();
    int from_idx = result.find_piece_at(fromX, fromY);
    if (from_idx == -1) throw std::runtime_error("Mover not found");
    int to_idx = result.find_piece_at(toX, toY);
    const Piece mover = result.get_piece(from_idx);
    CastlingRights cr = result.get_castling_rights();

    if (move.is_attempted_castling()) {
        const bool kingside = move.is_attempted_kingside_castling();
        const int rook_from_x = kingside ? 7 : 0;
        const int rook_to_x = kingside ? (fromX + 1) : (fromX - 1);
        const int rook_idx = result.find_piece_at(rook_from_x, fromY);
        if (rook_idx == -1) throw std::runtime_error("Castling rook missing");
        result.teletransport_piece(rook_idx, rook_to_x, fromY);
        if (mover.white) {
            cr.white_kingside = false;
            cr.white_queenside = false;
        } else {
            cr.black_kingside = false;
            cr.black_queenside = false;
        }
    } else if (move.is_attempted_en_passant()) {
        const int capture_idx = result.find_piece_at(toX, fromY);
        if (capture_idx == -1) throw std::runtime_error("En-passant capture missing");
        const bool adjust_from = capture_idx < from_idx;
        result.delete_piece(capture_idx);
        if (adjust_from) --from_idx;
    } else if (move.is_attempted_capture() && to_idx != -1) {
        const Piece& captured = result.get_piece(to_idx);
        cr.revoke_for_rook(captured);
        const bool adjust_from = to_idx < from_idx;
        result.delete_piece(to_idx);
        if (adjust_from) --from_idx;
    }

    if (move.is_attempted_promotion()) {
        if (!move.has_promotion()) throw std::runtime_error("Promotion missing");
        result.change_pawn_kind(from_idx, move.get_promotion());
    }

    result.teletransport_piece(from_idx, toX, toY);

    if (move.is_attempted_initial_two_square_pawn_move()) {
        const int direction = mover.white ? 1 : -1;
        const EnPassantVulnerable vulnerable = mover.white ?
            EnPassantVulnerable::White : EnPassantVulnerable::Black;
        result.set_en_passant(EnPassant(fromX, fromY + direction, vulnerable));
    } else {
        result.clear_en_passant();
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

    result.set_castling(cr);

    if (move.is_attempted_capture_or_pawn_move()) {
        result.reset_halfmove_clock();
    } else {
        result.increase_halfmove_clock();
    }

    result.toggle_white_to_move();
    return result;
}

} // namespace algebraic_notation_detail

inline std::string to_algebraic_notation(const Move& move, const Board& board) {
    const int mover_idx = board.find_piece_at(move.from_x(), move.from_y());
    if (mover_idx == -1) throw std::runtime_error("Mover missing for notation");
    const Piece mover = board.get_piece(mover_idx);
    std::string notation;

    if (move.is_attempted_castling()) {
        notation = move.is_attempted_kingside_castling() ? "O-O" : "O-O-O";
    } else {
        const bool is_pawn = mover.kind == PieceKind::Pawn;
        if (!is_pawn) {
            notation.push_back(kind_to_char(mover.kind));
            notation += algebraic_notation_detail::disambiguate_piece(board, move, mover);
        }

        if (is_pawn && move.is_attempted_capture()) {
            notation.push_back(algebraic_notation_detail::file_char(mover.x));
        }

        if (move.is_attempted_capture()) {
            notation.push_back('x');
        }

        notation += algebraic_notation_detail::square_to_string(move.to_x(), move.to_y());

        if (move.is_attempted_promotion()) {
            if (!move.has_promotion()) throw std::runtime_error("Promotion choice missing");
            notation.push_back('=');
            notation.push_back(kind_to_char(move.get_promotion()));
        }
    }

    Board after = algebraic_notation_detail::board_after_move(board, move);
    const bool opponent_in_check = after.is_player_in_check(after.is_white_to_move());
    const GameStatus status = Lawyer::instance().game_status(after, std::vector<Board>{});
    if (status == GameStatus::ThreefoldRepetition) {
        throw std::runtime_error("Algebraic notation found a 3-fold repetition draw");
    }

    if (status == GameStatus::Checkmate) {
        notation.push_back('#');
    } else if (opponent_in_check) {
        notation.push_back('+');
    }

    std::string outcome;
    if (status == GameStatus::Checkmate) {
        const bool winner_white = !after.is_white_to_move();
        outcome = winner_white ? "1-0" : "0-1";
    } else if (status == GameStatus::Stalemate || status == GameStatus::FiftyMoveRule) {
        outcome = "1/2-1/2";
    }

    if (!outcome.empty()) {
        if (!notation.empty()) notation.push_back(' ');
        notation += outcome;
    }

    return notation;
}

inline std::optional<Move> from_algebraic_notation(const Board& board, const std::string& notation) {
    const bool white_to_move = board.is_white_to_move();
    const int piece_count = board.get_piece_count();

    auto matches_notation = [&](const Move& candidate) -> bool {
        if (!Lawyer::instance().legal(board, candidate)) return false;
        return to_algebraic_notation(candidate, board) == notation;
    };

    for (int idx = 0; idx < piece_count; ++idx) {
        const Piece& mover = board.get_piece(idx);
        if (mover.white != white_to_move) continue;
        const auto targets = board.get_targets(idx);

        for (const auto& target : targets) {
            Move move(mover.x, mover.y, target.first, target.second, board);
            if (!move.is_valid()) continue;

            if (!move.is_attempted_promotion()) {
                if (matches_notation(move)) return move;
                continue;
            }

            const PieceKind promotions[] = {
                PieceKind::Queen,
                PieceKind::Rook,
                PieceKind::Bishop,
                PieceKind::Knight
            };
            for (PieceKind promotion : promotions) {
                Move candidate = move;
                candidate.set_promotion(promotion);
                if (matches_notation(candidate)) return candidate;
            }
        }
    }

    return std::nullopt;
}

#endif // ALGEBRAIC_NOTATION_H

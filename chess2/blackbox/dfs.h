#ifndef DFS_H
#define DFS_H

#include <limits>
#include <optional>
#include <vector>
#include "board.h"
#include "lawyer.h"
#include "move.h"
#include "oracle.h"

/*
* Class DFS.
* If `white`, try to find a win for white. Else, for black.
*
* Performs DFS on chess, assigning a score to each node.
* A score of +infty means our player (white if white, else black) is winning.
* This is in contrast with the Oracle, which always returns the score from
* white's perspective, as is standard in sites like Chess.com.
*/

class DFS {
public:
    static int MAX_DEPTH;

    explicit DFS(Oracle oracle, bool white)
        : oracle_(std::move(oracle)), white_(white) {}

    Move explore(const Board& root) {
        if (root.is_white_to_move() != white_) {
            throw std::runtime_error("DFS player-to-move not same as board player-to-move");
        }
        Lawyer& lawyer = Lawyer::instance();
        GameStatus status = lawyer.game_status(root, {});
        if (status != GameStatus::Ongoing) {
            throw std::runtime_error("DFS::explore called on terminal board");
        }
        auto result = explore_recursive(root, 0);
        if (!result.best_move.has_value()) {
            throw std::runtime_error("DFS::explore failed to find any legal move, board should've been caught as terminal");
        }
        return result.best_move.value();
    }

private:
    struct NodeResult {
        std::optional<Move> best_move;
        double score = -std::numeric_limits<double>::infinity();
    };

    NodeResult explore_recursive(const Board& board, int depth) const {
        Lawyer& lawyer = Lawyer::instance();
        GameStatus status = lawyer.game_status(board, {});

        if (status != GameStatus::Ongoing) {
            double terminal_score = 0.0;
            if (status == GameStatus::Checkmate) {
                // std::cout << "Found checkmate: " << board << std::endl;
                terminal_score = std::numeric_limits<double>::infinity();
                if (white_ == board.is_white_to_move()) terminal_score *= -1;
            } else if (status == GameStatus::Stalemate || status == GameStatus::FiftyMoveRule) {
                terminal_score = 0.0;
            } else if (status == GameStatus::ThreefoldRepetition) {
                // TODO maybe we should prune 2-fold repetitions: means this path is at best a draw.
                // Currently the DFS does explore loops but we don't catch 3fold because we always feed an empty history,
                // see the above call to lawyer.game_status()
                throw std::runtime_error("DFS found a 3-fold repetition draw");
            }
            return NodeResult{std::nullopt, terminal_score};
        }

        if (depth == MAX_DEPTH) {
            double score = oracle_.evaluate(board);
            if (!white_) score *= -1;
            return NodeResult{std::nullopt, score};
        } else if (depth > MAX_DEPTH) {
            throw std::runtime_error("DFS went over its MAX_DEPTH");
        }

        const int piece_count = board.get_piece_count();
        const bool white_to_move = board.is_white_to_move();

        NodeResult best;
        best.score = -std::numeric_limits<double>::infinity();

        for (int idx = 0; idx < piece_count; ++idx) {
            const Piece& piece = board.get_piece(idx);
            if (piece.white != white_to_move) continue;
            const auto targets = board.get_targets(idx);
            for (const auto& target : targets) {
                Move move(piece.x, piece.y, target.first, target.second, board);
                if (!move.is_valid()) continue;
                if (move.is_attempted_promotion() && !move.has_promotion()) {
                    // TODO detect when underpromotion is better
                    // Due to stalemate, or a knight checkmate
                    // (OK to only check for knight checkmate a few layers deep)
                    move.set_promotion(PieceKind::Queen);
                }
                if (!lawyer.legal(board, move)) continue;
                Board next = board;
                lawyer.perform_move(next, move);
                auto child = explore_recursive(next, depth + 1);

                if (child.score > best.score) {
                    best.score = child.score;
                    best.best_move.emplace(move);
                }
            }
        }

        return best;
    }

    const Oracle oracle_;
    const bool white_;
};

inline int DFS::MAX_DEPTH = 2;

#endif // DFS_H

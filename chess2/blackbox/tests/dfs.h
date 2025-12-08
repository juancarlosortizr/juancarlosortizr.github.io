#ifndef TESTS_DFS_H
#define TESTS_DFS_H

#include <cassert>
#include <string>
#include <vector>
#include "../board.h"
#include "../game.h"
#include "../lawyer.h"
#include "../dfs.h"
#include "../algebraic_notation.h"

namespace tests {

inline Move make_move(Game& game, const std::string& notation) {
    const Board& board = game.board();
    auto move_opt = from_algebraic_notation(board, notation);
    if (!move_opt.has_value()) {
        throw std::runtime_error("Failed to parse move " + notation);
    }
    Move move = move_opt.value();
    if (move.is_attempted_promotion() && !move.has_promotion()) {
        throw std::runtime_error("Move " + notation + " is attempted promotion but has no promotion");
    }
    const int ok = game.verify_and_move(move);
    if (ok != 0) {
        throw std::runtime_error("Move " + notation + " rejected");
    }
    return move;
}

inline void dfs_e4_e5_maxdepth1_material_oracle_test() {
    Game game;
    make_move(game, "e4");
    make_move(game, "d5");

    DFS::MAX_DEPTH = 1;
    DFS dfs(make_material_oracle(), true);
    const Move best = dfs.explore(game.board());
    const std::string notation = to_algebraic_notation(best, game.board());
    if (notation.rfind("exd5", 0) != 0) {
        throw std::runtime_error("Expected exd5, got " + notation);
    }
}

inline void dfs_fools_mate_test() {
    Game game;
    make_move(game, "f3");
    make_move(game, "e5");
    make_move(game, "g4");

    DFS::MAX_DEPTH = 1;
    Oracle o1{};
    Oracle o2 = make_material_oracle();
    while (DFS::MAX_DEPTH <= 3) {
        for (Oracle* o : { &o1, &o2 }) {
            DFS dfs{*o, false};
            const Move best = dfs.explore(game.board());
            const std::string notation = to_algebraic_notation(best, game.board());
            if (notation.rfind("Qh4# 0-1", 0) != 0) {
                throw std::runtime_error("Expected Qh4# 0-1, got " + notation);
            }
        }
        DFS::MAX_DEPTH++;
    }
}

inline void dfs_scholars_mate_test() {
    Game game;
    make_move(game, "e4");
    make_move(game, "e5");
    make_move(game, "Bc4");
    make_move(game, "a6");
    make_move(game, "Qf3");
    make_move(game, "Nc6");

    DFS::MAX_DEPTH = 1;
    Oracle o1{};
    Oracle o2 = make_material_oracle();
    while (DFS::MAX_DEPTH <= 3) {
        for (Oracle* o : { &o1, &o2 }) {
            DFS dfs{*o, true};
            const Move best = dfs.explore(game.board());
            const std::string notation = to_algebraic_notation(best, game.board());
            if (notation.rfind("Qxf7# 1-0", 0) != 0) {
                throw std::runtime_error("Expected Qxf6# 0-1, got " + notation);
            }
        }
        DFS::MAX_DEPTH++;
    }
}

inline void run_all() {
    dfs_e4_e5_maxdepth1_material_oracle_test();
    dfs_fools_mate_test();
    dfs_scholars_mate_test();
}

} // namespace tests

#endif // TESTS_DFS_H

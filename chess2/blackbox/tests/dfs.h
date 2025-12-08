#ifndef TESTS_DFS_H
#define TESTS_DFS_H

#include <cassert>
#include <string>
#include <vector>
#include <set>
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

inline void run_scenario(const std::string& test_name,
                         const std::vector<std::string>& moves,
                         const std::set<std::string>& expected_best,
                         Oracle oracle,
                         int max_depth) {
    Game game;
    for (const auto& san : moves) {
        make_move(game, san);
    }

    int orig_max_depth = DFS::MAX_DEPTH;
    DFS::MAX_DEPTH = max_depth;
    const bool white_to_move = game.board().is_white_to_move();
    DFS dfs(std::move(oracle), white_to_move);
    const Move best = dfs.explore(game.board(), game.get_halfmove_clock());
    const std::string notation = to_algebraic_notation(best, game.board());
    if (expected_best.find(notation) == expected_best.end()) {
        std::string expected_best_str = "";
        for (auto eb : expected_best) {
            expected_best_str = expected_best_str + eb + ";";
        }
        throw std::runtime_error("[" + test_name + "] Expected " + expected_best_str + " but got " + notation);
    }
    DFS::MAX_DEPTH = orig_max_depth;
}

inline void dfs_e4_e5_material_oracle_test() {
    run_scenario("dfs_e4_e5_material_depth",
                {"e4", "d5"},
                {"exd5"},
                make_material_oracle(),
                1);
}

inline void dfs_bishop_check_test() {
    run_scenario("dfs_e4_e5_material_depth",
                {"e4", "d5", "Be2", "c5"},
                {"Bb5+"},
                make_material_oracle(),
                3);
}

inline void dfs_fools_mate_test() {
    const std::vector<std::string> moves = {"f3", "e5", "g4"};
    Oracle basic_oracle{};
    Oracle material = make_material_oracle();
    for (int depth = 1; depth <= 4; ++depth) {
        run_scenario("dfs_fools_mate_basic_depth_" + std::to_string(depth),
                     moves,
                     {"Qh4# 0-1"},
                     basic_oracle,
                     depth);
        run_scenario("dfs_fools_mate_material_depth_" + std::to_string(depth),
                     moves,
                     {"Qh4# 0-1"},
                     material,
                     depth);
    }
}

inline void dfs_scholars_mate_test() {
    const std::vector<std::string> moves = {"e4", "e5", "Bc4", "a6", "Qf3", "Nc6"};
    Oracle basic_oracle{};
    Oracle material = make_material_oracle();
    for (int depth = 1; depth <= 4; ++depth) {
        run_scenario("dfs_scholars_mate_basic_depth_" + std::to_string(depth),
                     moves,
                     {"Qxf7# 1-0"},
                     basic_oracle,
                     depth);
        run_scenario("dfs_scholars_mate_material_depth_" + std::to_string(depth),
                     moves,
                     {"Qxf7# 1-0"},
                     material,
                     depth);
    }
}

inline void dfs_lose_bishop_test() {
    const std::vector<std::string> moves = {"e4", "e5", "Ba6"};
    Oracle material = make_material_oracle();
    for (int depth = 1; depth <= 4; ++depth) {
        run_scenario("dfs_lose_bishop_material_depth_" + std::to_string(depth),
                     moves,
                     {"Nxa6", "bxa6"},
                     material,
                     depth);
    }
}

inline void run_all() {
    dfs_e4_e5_material_oracle_test();
    dfs_fools_mate_test();
    dfs_scholars_mate_test();
    dfs_lose_bishop_test();
}

} // namespace tests

#endif // TESTS_DFS_H

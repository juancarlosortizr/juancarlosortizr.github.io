#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <cctype>
#include "game.h"
#include "algebraic_notation.h"

/*
* chess_cmdline
* Mostly for testing purposes.
*/

static void announce_outcome(const Game& game) {
    switch (game.status()) {
        case GameStatus::Checkmate:
            std::cout << "Checkmate! "
                      << (game.winner() == GameWinner::White ? "White" : "Black")
                      << " wins.\n";
            break;
        case GameStatus::Stalemate:
            std::cout << "Stalemate. Draw.\n";
            break;
        case GameStatus::FiftyMoveRule:
            std::cout << "Draw by fifty-move rule.\n";
            break;
        case GameStatus::ThreefoldRepetition:
            std::cout << "Draw by 3-fold repetition.\n";
            break;
        case GameStatus::Ongoing:
            break;
        default:
            throw std::runtime_error("Unrecognized GameStatus");
    }
}

int main() {
    Game game;

    while (true) {
        if (game.status() != GameStatus::Ongoing) {
            announce_outcome(game);
            return 0;
        }

        std::cout << (game.board().is_white_to_move() ? "White" : "Black")
                  << " move (algebraic notation): "
                  << std::flush;

        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << "\nInput stream closed. Exiting.\n";
            return 0;
        }

        std::string trimmed;
        std::istringstream iss(line);
        iss >> trimmed;
        if (trimmed.empty()) {
            std::cout << "Invalid input. Please enter algebraic notation.\n";
            continue;
        }

        std::optional<Move> attempted = from_algebraic_notation(game.board(), trimmed);
        if (!attempted.has_value()) {
            std::cout << "Could not find a legal move matching that notation.\n";
            continue;
        }

        int result = game.verify_and_move(attempted.value());
        if (result == 0) {
            if (game.status() != GameStatus::Ongoing) {
                announce_outcome(game);
                return 0;
            }
            continue;
        }

        if (result == -1) {
            std::cout << "Illegal move. Try again.\n";
        } else {
            std::cout << "Invalid move. Try again.\n";
        }
    }
}

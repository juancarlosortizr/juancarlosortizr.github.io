#include <iostream>
#include <sstream>
#include "board.hpp"
#include "fen.hpp"
#include "game.hpp"
#include "move.hpp"
#include "print.hpp"
#include "stateless_fen.hpp"

int main() {
    State s;

    // // Print initial state
    // std::cout << "Initial state" << std::endl;
    // std::cout << s << std::endl;

    // // Check White e4 meaningful
    // std::cout << "Meaningful e4? " << std::flush;
    // std::cout << ((s.meaningful({
    //     MOVE::NORMAL, PLAYERTOMOVE::WHITE, {"e2"}, {"e4"}
    // })) ? "Yes" : "No") << std::endl;

    // // Check White Nf3 meaningful
    // std::cout << "Meaningful Nf3? " << std::flush;
    // std::cout << ((s.meaningful({
    //     MOVE::NORMAL, PLAYERTOMOVE::WHITE, {"g1"}, {"f3"}
    // })) ? "Yes" : "No") << std::endl;

    // // Check White Nf3 not meaningful
    // std::cout << "Meaningful Nf4? " << std::flush;
    // std::cout << ((s.meaningful({
    //     MOVE::NORMAL, PLAYERTOMOVE::WHITE, {"g1"}, {"f4"}
    // })) ? "Yes" : "No") << std::endl;

    // // Check White Bh3 not meaningful
    // std::cout << "Meaningful Bh3? " << std::flush;
    // std::cout << ((s.meaningful({
    //     MOVE::NORMAL, PLAYERTOMOVE::WHITE, {"f1"}, {"h3"}
    // })) ? "Yes" : "No") << std::endl;

    // // Check White kingside castling not meaningful
    // std::cout << "Meaningful O-O " << std::flush;
    // std::cout << ((s.meaningful({
    //     MOVE::CASTLE, PLAYERTOMOVE::WHITE, CASTLING::KINGSIDE
    // })) ? "Yes" : "No") << std::endl;

    // // Check en passant not meaningful
    // std::cout << "Meaningful en passant exd6? " << std::flush;
    // std::cout << ((s.meaningful({
    //     MOVE::ENPASSANT, PLAYERTOMOVE::WHITE, {"e5"}, {"d6"}
    // })) ? "Yes" : "No") << std::endl;

    // // Check Black move not meaningful
    // std::cout << "Meaningful Black e6? " << std::flush;
    // std::cout << ((s.meaningful({
    //     MOVE::NORMAL, PLAYERTOMOVE::BLACK, {"e7"}, {"e6"}
    // })) ? "Yes" : "No") << std::endl;

    // // Move e4
    // s.move({
    //     MOVE::NORMAL, PLAYERTOMOVE::WHITE, {"e2"}, {"e4"}
    // });
    // std::cout << "After 1. e4" << std::endl;
    // std::cout << s << std::endl;

    // // Move e5
    // s.move({
    //     MOVE::NORMAL, PLAYERTOMOVE::BLACK, {"e7"}, {"e5"}
    // });
    // std::cout << "After 1... e4" << std::endl;
    // std::cout << s << std::endl;

    std::cout << "New game" << std::endl;
    Game g;
    g.play();

    return 0;
} 
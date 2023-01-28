#include <cstring>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <vector>
#include "fen.hpp"
#include "move.hpp"
#include "state.hpp"

#ifndef CHESS_GAME_H
#define CHESS_GAME_H 

// TODO better place for these constants?
const std::string QUIT = "QUIT";
constexpr char COMMAND_SEPARATOR = ',';

// TODO better place for this method?
// move type, player to move, castling side
// OR
// move type, player to move, start, end
// E.G.
// 'CASTLE,WHITE,KINGSIDE'
// 'ENPASSANT,WHITE,e5,e6'
// 'CAPTURE,WHITE,e6,g8'
// 'NORMAL,BLACK,e7,e6'
std::optional<Move> readRawNotation(std::string raw) {
    std::string temp; 
    std::stringstream ss(raw);
    std::vector<std::string> words;

    while (getline(ss, temp, COMMAND_SEPARATOR)){
        words.push_back(temp);
    }
    if (words.size() < 3 || words.size() > 4) return {};

    // Castle
    if (words[0] == "CASTLE") {
        if (words[1] == "WHITE") {
            if (words[2] == "KINGSIDE") return Move(
                MOVE::CASTLE, PLAYERTOMOVE::WHITE, CASTLING::KINGSIDE
            );
            return Move(
                MOVE::CASTLE, PLAYERTOMOVE::WHITE, CASTLING::QUEENSIDE
            );
        } else {
            if (words[2] == "KINGSIDE") return Move(
                MOVE::CASTLE, PLAYERTOMOVE::BLACK, CASTLING::KINGSIDE
            );
            return Move(
                MOVE::CASTLE, PLAYERTOMOVE::BLACK, CASTLING::QUEENSIDE
            );
        }
    }

    // En passant
    if (words[0] == "ENPASSANT") {
        if (words[1] == "WHITE") {
            return Move(
                MOVE::ENPASSANT, PLAYERTOMOVE::WHITE, {words[2]}, {words[3]}
            );
        } else {
            return Move(
                MOVE::ENPASSANT, PLAYERTOMOVE::BLACK, {words[2]}, {words[3]}
            );
        }
    }

    // Capture
    if (words[0] == "CAPTURE") {
        if (words[1] == "WHITE") {
            return Move(
                MOVE::CAPTURE, PLAYERTOMOVE::WHITE, {words[2]}, {words[3]}
            );
        } else {
            return Move(
                MOVE::CAPTURE, PLAYERTOMOVE::BLACK, {words[2]}, {words[3]}
            );
        }
    }

    // En passant
    if (words[0] == "NORMAL") {
        if (words[1] == "WHITE") {
            return Move(
                MOVE::NORMAL, PLAYERTOMOVE::WHITE, {words[2]}, {words[3]}
            );
        } else {
            return Move(
                MOVE::NORMAL, PLAYERTOMOVE::BLACK, {words[2]}, {words[3]}
            );
        }
    }

    return {};
}

// Class to represent an ongoing or finished chess game
class Game {
    // All previous positions, for the 3-time repetition rule
    std::unordered_set<State, FEN> onceRepeatedPositions;
    std::unordered_set<State, FEN> twiceRepeatedPositions;
    std::vector<Move> allMoves;  // for game history purposes
    State currPos;
public:
    Game() : currPos{}, onceRepeatedPositions{}, twiceRepeatedPositions{} {}

    void play() {
        std::cout << currPos << std::endl;
        std::string command;

        while (true) {
            std::cout << "Input a move, or type " << QUIT 
                    << " to quit. Player to move: "
                    << currPos.playerToMove() << std::endl;
            std::cin >> command;
            if (command == QUIT) break;
            auto move = readRawNotation(command);
            if (!move.has_value() || !currPos.meaningful(move.value())) {
                std::cout << "That move is meaningless. Try again." << std::endl;
                continue;
            }
            if (!currPos.legal(move.value())) {
                std::cout << "That moves is illegal. Try again" << std::endl;
                continue;
            }
            std::cout << "That move is legal. Proceeding to make it" << std::endl;
            currPos.move(move.value());
            allMoves.push_back(move.value());
            std::cout << currPos << std::endl;
        }

        return;
    }

};

#endif  // CHESS_GAME_H
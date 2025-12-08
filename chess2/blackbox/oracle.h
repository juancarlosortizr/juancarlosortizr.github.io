#ifndef ORACLE_H
#define ORACLE_H

#include <functional>
#include "board.h"
#include "material.h"

/*
* class Oracle
*
* A score of 0 should be for draws or very even positions.
* A score of +infty means White wins, and -infty means Black wins.
* This is consistent with most "score" notations from other engines.
*/

class Oracle {
public:
    using Evaluator = std::function<double(const Board&)>;

    Oracle()
        : evaluator_([](const Board&) { return 0.0; }) {}
    explicit Oracle(Evaluator evaluator)
        : evaluator_(std::move(evaluator)) {}

    double evaluate(const Board& board) const {
        return evaluator_(board);
    }

private:
    Evaluator evaluator_;
};

inline Oracle make_material_oracle() {
    return Oracle([](const Board& board) {
        return static_cast<double>(material::balance(board));
    });
}

#endif // ORACLE_H

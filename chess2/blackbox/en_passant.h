#ifndef EN_PASSANT_H
#define EN_PASSANT_H

#include <sstream>
#include <stdexcept>
#include <ostream>
#include <string>
#include "square_utils.h"

/*
* En passant rights.
* Only knows the square we would capture on if en passant is active,
* and which player activated (and is vulnerable to a capture via) en passant.
*/

enum class EnPassantVulnerable { White, Black, None };

class EnPassant {
private:
    int x, y;
    bool active;
    EnPassantVulnerable vulnerable;

public:
    // Inactive Constructor
    EnPassant() : x(-1), y(-1), vulnerable(EnPassantVulnerable::None), active(false) {}
    // Active constructor
    EnPassant(int x_, int y_, EnPassantVulnerable v) : x(x_), y(y_), vulnerable(v), active(true) {
        if (x < 0 || x > 7 || y < 0 || y > 7) {
            throw std::runtime_error("Invalid square for en-passant");
        }
        if (v == EnPassantVulnerable::None) {
            throw std::runtime_error("No vulnerable player for en-passant");
        }
    }

    bool white_vulnerable(void) const {
        if (!active) {
            throw std::runtime_error("Inactive en-passant, no vulnerable player");
        }
        return (vulnerable == EnPassantVulnerable::White);
    }

    int get_x(void) const { return x; }
    int get_y(void) const {return y; }
    int is_active(void) const { return active; }
    bool operator==(const EnPassant& other) const {
        if (active != other.active) return false;
        if (!active) return true;
        return x == other.x && y == other.y && vulnerable == other.vulnerable;
    }

    std::string to_string() const {
        if (!active) return "X";
        const char* colour = (vulnerable == EnPassantVulnerable::White) ? "White" : "Black";
        return std::string(colour) + " @" + square_utils::square_to_string(x, y);
    }

    friend std::ostream& operator<<(std::ostream& os, const EnPassant& ep) {
        if (!ep.active) {
            if (ep.vulnerable != EnPassantVulnerable::None) {
                throw std::runtime_error("Inactive en-passant has vulnerable player");
            }
            os << "-";
            return os;
        }
        const char* colour = (ep.vulnerable == EnPassantVulnerable::White) ? "White" : "Black";
        os << "En Passant: " << colour << " vulnerable @" << square_utils::square_to_string(ep.x, ep.y);
        return os;
    }
};

#endif // EN_PASSANT_H

#ifndef CASTLING_H
#define CASTLING_H

#include "piece.h"
#include <ostream>

struct CastlingRights {
    bool white_kingside  = true;
    bool white_queenside = true;
    bool black_kingside  = true;
    bool black_queenside = true;

    CastlingRights(void) : white_kingside(false),
                           white_queenside(false),
                           black_kingside(false),
                           black_queenside(false)
    {}
    CastlingRights(bool wk, bool wq, bool bk, bool bq) : white_kingside(wk),
                                                   white_queenside(wq),
                                                   black_kingside(bk),
                                                   black_queenside(bq)
    {}

    bool operator==(const CastlingRights& other) const {
        return white_kingside == other.white_kingside &&
               white_queenside == other.white_queenside &&
               black_kingside == other.black_kingside &&
               black_queenside == other.black_queenside;
    }

    // Given a piece, if it is a rook, revoke castling rights of that rook,
    // only if the rook is in its starting square.
    void revoke_for_rook(const Piece& rook) {
        if (rook.kind != PieceKind::Rook) return;
        if (rook.white) {
            if (rook.x == 0 && rook.y == 0) white_queenside = false;
            if (rook.x == 7 && rook.y == 0) white_kingside = false;
        } else {
            if (rook.x == 0 && rook.y == 7) black_queenside = false;
            if (rook.x == 7 && rook.y == 7) black_kingside = false;
        }
    }

    // String representation
    friend std::ostream& operator<<(std::ostream& os, const CastlingRights& cr) {
        bool printed = false;
        auto append = [&](bool flag, const char* label) {
            if (!flag) return;
            if (printed) os << ", ";
            os << label;
            printed = true;
        };
        if (cr.white_kingside && cr.white_queenside && cr.black_kingside && cr.black_queenside) {
            os << "All";
            printed = true;
        } else {
            append(cr.white_kingside, "wk");
            append(cr.white_queenside, "wq");
            append(cr.black_kingside, "bk");
            append(cr.black_queenside, "bq");
        }
        if (!printed) os << "-";
        return os;
    }
};

#endif // CASTLING_H

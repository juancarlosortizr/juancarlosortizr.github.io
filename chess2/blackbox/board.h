#ifndef BOARD_H
#define BOARD_H

#include <vector> 
#include <set>
#include <utility>
#include <algorithm>
#include <ostream>
#include <string>
#include "piece.h"
#include "en_passant.h"
#include "castling.h"

/*
* Class Board.
* Barebones class that is a container for pieces and game state.
* No actual logic here.
* Board encapsulates piece placement plus auxiliary game state:
* - castling rights
* - en-passant target (active when a pawn just moved 2 squares)
* - whose turn it is
* Notice for 3-fold repetition, castling rights, en passant rights, and whose turn it is
* all matter.
*
* This just stores the state, no move legality logic (this is in moves.h).
* It does knows the basics of how pieces attack, including:
* castling (king horizontal 2 squares), pawn capture, pawn initial 2-square move,
* plus all the normal moves.
*
* Piece location is stored twice, in `pieces` and `occupancy`, because it's easier
* to check things like if a rook move has no pieces in-between this way.
*
* This does NOT store move history, so no undoes nor threefold repetition detection.
* Knowledge of 3-fold repetition is unnecessary 
* for a player agent that using graph algorithms
* and state evaluations. If it is a draw by 3-fold repetition,
* the agent will simply loop indefinitely.
*/

const auto in_bounds = [](int x, int y)->bool { return x >= 0 && x < 8 && y >= 0 && y < 8; };

const int knight_offs[8][2] = {
    {1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}
};

// queen dirs is just the combination of the below two
const int bishop_dirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
const int rook_dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

const PieceKind startingBackRank[8] = {
    PieceKind::Rook,
    PieceKind::Knight,
    PieceKind::Bishop,
    PieceKind::Queen,
    PieceKind::King,
    PieceKind::Bishop,
    PieceKind::Knight,
    PieceKind::Rook,
};

class Board {
private:
    // pieces on the board (0..8-1 coordinates)
    std::vector<Piece> pieces;
    int occupancy[8][8];

    // auxiliary game state
    CastlingRights castling;
    EnPassant en_passant;

    // player to move: true = white to move, false = black to move
    bool white_to_move = true;

public:
    // Construtor produces an empty board with no casting nor en passant rights.
    // For a starting position, call reset().
    Board(void) : pieces(),
                   castling(),
                   en_passant(),
                   white_to_move(true)
    {
        for (int i=0; i<8; i++) {
            for (int j=0; j<8; j++) {
                occupancy[i][j] = -1;
            }
        }
    }

    // initialize pieces to standard starting position and reset state
    void reset(void) {
        pieces.clear();
        pieces.reserve(32);
        // White back rank (row 0) and pawns (row 1)
        for (int x = 0; x < 8; ++x) pieces.push_back({x, 0, true, startingBackRank[x]});
        for (int x = 0; x < 8; ++x) pieces.push_back({x, 1, true, 'P'});
        // Black pawns (row 6) and back rank (row 7)
        for (int x = 0; x < 8; ++x) pieces.push_back({x, 6, false, 'P'});
        for (int x = 0; x < 8; ++x) pieces.push_back({x, 7, false, startingBackRank[x]});

        // All castling rights available. No en passant rights. White to move.
        castling = CastlingRights{true, true, true, true};
        en_passant = EnPassant{};
        white_to_move = true;

        for (int i=0; i<8; i++) {
            for (int j=0; j<8; j++) {
                occupancy[i][j] = -1;
            }
        }
        for (size_t i = 0; i < pieces.size(); i++) {
            occupancy[pieces[i].x][pieces[i].y] = i;
        }
    }

    // Get a const reference of piece at index idx.
    const Piece& get_piece(int idx) const {
        if (idx < 0 || idx >= (int)pieces.size()) {
            throw std::runtime_error("get_piece: index out of bounds");
        }
        return pieces[idx];
    }

    // Teletransport a piece. This doesn't change any other attributes.
    // The target square must be empty.
    void teletransport_piece(const int idx, const int x, const int y) {
        if (idx < 0 || idx >= (int)pieces.size()) {
            throw std::runtime_error("teletransport_piece: index out of bounds");
        }
        if (occupancy[x][y] != -1) {
            throw std::runtime_error("teletransport_piece: target square non-empty");
        }
        occupancy[pieces[idx].x][pieces[idx].y]= -1;
        occupancy[x][y] = idx;
        pieces[idx].x = x;
        pieces[idx].y = y;
    }

    // Promote a pawn to another piece kind. This ONLY changes its piece kind.
    void change_pawn_kind(const int idx, const PieceKind newKind) {
        if (idx < 0 || idx >= (int)pieces.size()) {
            throw std::runtime_error("promote_pawn: index out of bounds");
        }
        if (pieces[idx].kind != PieceKind::Pawn) {
            throw std::runtime_error("promote_pawn: piece is not a pawn");
        }
        pieces[idx].kind = newKind;
    }

    // Get total piece count
    int get_piece_count() const {
        return (int)pieces.size();
    }

    // Delete a piece
    void delete_piece(int idx) {
        if (idx < 0 || idx >= (int)pieces.size()) {
            throw std::runtime_error("delete_piece: index out of bounds");
        }
        int orig_occ = occupancy[pieces[idx].x][pieces[idx].y];
        occupancy[pieces[idx].x][pieces[idx].y] = -1;
        for (int i=0; i<8; i++) {
            for (int j=0; j<8; j++) {
                if (occupancy[i][j] > orig_occ) {
                    --occupancy[i][j];
                } else if (occupancy[i][j] == orig_occ) {
                    throw std::runtime_error("Duplicate value in occupancy");
                }
            }
        }
        pieces.erase(pieces.begin() + idx);
    }

    bool is_white_to_move() const {
        return white_to_move;
    }

    void toggle_white_to_move() {
        white_to_move = !white_to_move;
    }

    // find index of piece at square (x,y) or -1 if no piece
    int find_piece_at(int x, int y) const {
        return occupancy[x][y];
    }

    // en-passant helpers
    void set_en_passant(EnPassant ep) { en_passant = ep; }
    void clear_en_passant(void) { en_passant = EnPassant{}; }
    bool has_en_passant(void) const { return en_passant.is_active(); }
    EnPassant get_en_passant() const { return en_passant; }

    // convenience: set or get (by copy) castling rights
    void set_castling(CastlingRights cr) { castling = cr; }
    CastlingRights get_castling_rights() const { return castling; }

    friend std::ostream& operator<<(std::ostream& os, const Board& board);

    // Is castling a valid move?
    bool can_castle(const bool white, const bool kingside) const {
        const bool rights_ok = white ?
            (kingside ? castling.white_kingside : castling.white_queenside) :
            (kingside ? castling.black_kingside : castling.black_queenside);
        if (!rights_ok) return false;
        const int rookX = kingside ? 7 : 0;
        const int step = kingside ? 1 : -1;
        const int kingX = 4;
        const int kingY = white ? 0 : 7;  // same as rook Y
        int rook_idx = occupancy[rookX][kingY];
        if (rook_idx == -1) {
            throw std::runtime_error("Castling rights should not be active here");
        }
        const Piece& rook = pieces[rook_idx];
        if (rook.kind != PieceKind::Rook || rook.white != white) {
            throw std::runtime_error("Castling rights should not be active here");
        }
        int king_idx = occupancy[kingX][kingY];
        if (king_idx == -1) {
            throw std::runtime_error("Castling rights should not be active here");
        }
        const Piece& king = pieces[king_idx];
        if (king.kind != PieceKind::King || king.white != white) {
            throw std::runtime_error("Castling rights should not be active here");
        }
        // Check for empty space between king and rook
        for (int px = kingX + step; px != rookX; px += step) {
            if (occupancy[px][kingY] != -1) return false;
        }
        return true;
    }
    // Helper: get the "midpoint" of castling. If it is attacked, castling is valid but not legal
    // Does NOT perform any verifications regarding castling rights, the rook actually being there, etc
    inline std::pair<int,int> _midpoint_castling(const bool white, const bool kingside) const {
        const int step = kingside ? 1 : -1;
        const int kingX = 4;
        const int kingY = white ? 0 : 7;  // same as rook Y
        return {kingX + step, kingY};
    }
                        

    /*
    * Board::get_targets()
    * TODO move to an attribute? That is recomputed upon every move
    *
    * Generate all target squares that are reachable by the piece at index idx.
    * This always considers possible en-passant captures or diagonal pawn captures.
    * If the path is blocked, do not consider the target as reachable (except for knights).
    * For pawn non-captures, a path is also considered blocked if the target is occupied.
    * For non-pawn moves, a path is also considered block if the target is occupied by a same-colour piece,
    * but not if it's occupied by an enemy piece.
    */
    std::set<std::pair<int,int>> get_targets(const size_t idx) const {
        if (idx < 0 || idx >= (int)pieces.size()) {
            throw std::runtime_error("get_targets: index out of bounds");
        }
        std::set<std::pair<int,int>> targets;
        const Piece& p = pieces[idx];

        // Find piece kind
        switch (p.kind) {
            case PieceKind::Pawn: {
                const int dir = p.white ? 1 : -1;

                // Non-en-pasasnt diagonal capture
                int ty = p.y + dir;
                for (int tx = p.x - 1; tx != p.x + 3; tx += 2) {
                    if (in_bounds(tx, ty)
                        && occupancy[tx][ty] != -1
                        && pieces[occupancy[tx][ty]].white != p.white) {
                        targets.emplace(tx, ty);
                    }
                }
                // En-passant capture
                if (en_passant.is_active() && ty == en_passant.get_y() && p.white != en_passant.white_vulnerable()) {
                    if (abs(en_passant.get_x() - p.x) == 1) {
                        targets.emplace(en_passant.get_x(), en_passant.get_y());
                    }
                }

                // One-square forward
                int tx = p.x;
                if (!in_bounds(tx, ty)) throw std::runtime_error("Pawn forward move not in bounds");
                if (occupancy[tx][ty] == -1) targets.emplace(tx, ty);

                // Initial two-square forward
                tx = p.x; ty = p.y + 2 * dir;
                const bool on_start = (p.white && p.y == 1) || (!p.white && p.y == 6);
                if (on_start) {
                    if (!in_bounds(tx, ty)) throw std::runtime_error("Initial 2-square move not in bounds");
                    if (occupancy[p.x][p.y + dir] == -1 && occupancy[tx][ty] == -1) { 
                        targets.emplace(tx, ty);
                    }
                }
                break;
            }
            case PieceKind::Knight: {
                for (const auto& o : knight_offs) {
                    int tx = p.x + o[0];
                    int ty = p.y + o[1];
                    if (!in_bounds(tx, ty)) continue;
                    int occ = occupancy[tx][ty];
                    if (occ == -1 || pieces[occ].white != p.white) targets.emplace(tx, ty);
                }
                break;
            }
            case PieceKind::King: {
                for (int dx = -1; dx <= 1; ++dx) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        if (dx == 0 && dy == 0) continue;
                        int tx = p.x + dx;
                        int ty = p.y + dy;
                        if (!in_bounds(tx, ty)) continue;
                        int occ = occupancy[tx][ty];
                        if (occ == -1 || pieces[occ].white != p.white) targets.emplace(tx, ty);
                    }
                }
                // Castling
                if (p.x == 4) {
                    const int back_rank = p.white ? 0 : 7;
                    if (p.y == back_rank) {
                        auto try_castle = [&](bool kingside) {
                            if (!can_castle(p.white, kingside)) return;
                            const int step = kingside ? 1 : -1;
                            targets.emplace(p.x + 2 * step, p.y);
                        };
                        try_castle(true);
                        try_castle(false);
                    }
                }
                break;
            }
            case PieceKind::Bishop: {
                for (const auto& d : bishop_dirs) {
                    int tx = p.x + d[0];
                    int ty = p.y + d[1];
                    while (in_bounds(tx, ty)) {
                        int occ = occupancy[tx][ty];
                        if (occ == -1) {
                            targets.emplace(tx, ty);
                        } else {
                            if (pieces[occ].white != p.white) targets.emplace(tx, ty);
                            break;
                        }
                        tx += d[0];
                        ty += d[1];
                    }
                }
                break;
            }
            case PieceKind::Rook: {
                for (const auto& d : rook_dirs) {
                    int tx = p.x + d[0];
                    int ty = p.y + d[1];
                    while (in_bounds(tx, ty)) {
                        int occ = occupancy[tx][ty];
                        if (occ == -1) {
                            targets.emplace(tx, ty);
                        } else {
                            if (pieces[occ].white != p.white) targets.emplace(tx, ty);
                            break;
                        }
                        tx += d[0];
                        ty += d[1];
                    }
                }
                break;
            }
            case PieceKind::Queen: {
                for (const auto& d : bishop_dirs) {
                    int tx = p.x + d[0];
                    int ty = p.y + d[1];
                    while (in_bounds(tx, ty)) {
                        int occ = occupancy[tx][ty];
                        if (occ == -1) {
                            targets.emplace(tx, ty);
                        } else {
                            if (pieces[occ].white != p.white) targets.emplace(tx, ty);
                            break;
                        }
                        tx += d[0];
                        ty += d[1];
                    }
                }
                for (const auto& d : rook_dirs) {
                    int tx = p.x + d[0];
                    int ty = p.y + d[1];
                    while (in_bounds(tx, ty)) {
                        int occ = occupancy[tx][ty];
                        if (occ == -1) {
                            targets.emplace(tx, ty);
                        } else {
                            if (pieces[occ].white != p.white) targets.emplace(tx, ty);
                            break;
                        }
                        tx += d[0];
                        ty += d[1];
                    }
                }
                break;
            }
            default:
                throw std::runtime_error("get_targets: invalid piece kind");
        }
        return targets;
    }
    std::set<std::pair<int,int>> get_targets(const int x, const int y) const {
        const int occ = occupancy[x][y];
        if (occ == -1) return std::set<std::pair<int,int>>{};
        return get_targets(occ);
    }

    // Check if a square is under attack by a player
    bool _is_under_attack(const bool white, const int x, const int y) const {
        for (size_t i = 0; i < pieces.size(); ++i) {
            if (pieces[i].white != white) continue;
            const std::set<std::pair<int,int>> targets = get_targets(i);
            if (targets.find({x, y}) != targets.end()) {
                return true;
            }
        }
        return false;
    }

    // If `white`, check if white king in check, else check if black king is in check.
    bool is_player_in_check(const bool white) const {
        int king_x = -1, king_y = -1;
        for (const auto& piece : pieces) {
            if (piece.white == white && piece.kind == PieceKind::King) {
                king_x = piece.x;
                king_y = piece.y;
                break;
            }
        }
        if (king_x == -1) {
            throw std::runtime_error("is_player_in_check: king not found");
        }

        return _is_under_attack(!white, king_x, king_y);
    }

    // Compare two boards, for 3-fold repetition
    bool operator==(const Board& other) const {
        if (white_to_move != other.white_to_move) return false;
        if (!(castling == other.castling)) return false;
        if (!(en_passant == other.en_passant)) return false;

        auto sorted_pieces = [&](const Board& board) {
            // Sort a copy of the pieces vector. Notice `occupancy`
            // still referes to the original pieces, not the copy.
            std::vector<Piece> ps = board.pieces;
            std::sort(ps.begin(), ps.end());
            return ps;
        };

        const std::vector<Piece> lhs = sorted_pieces(*this);
        const std::vector<Piece> rhs = sorted_pieces(other);
        return lhs == rhs;
    }

    friend std::ostream& operator<<(std::ostream& os, const Board& board);
};

#endif // BOARD_H

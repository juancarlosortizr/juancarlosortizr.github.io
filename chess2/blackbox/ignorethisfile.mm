/*

ALL THIS COMMENTED-OUT CODE IS SCRATCHWORK. DO NOT READ IT

    // In hypothetical mode, we are checking legality without actually making the move
    // - capturing our king is allowed
    // - If capturing king via promotion, promove_to can be empty
    bool is_valid(void) const {
        if (tried_to_move) throw std::runtime_error("Cannot query move validity after move");
        // from_piece is available and of the right colour
        int from_idx = board.find_piece_at(fromX, fromY);
        if (from_idx == -1) return false;
        if (
            board.get_piece(from_idx).white != board.is_white_to_move()
        ) {
            std::cout << "Move invalid: " << (board.is_white_to_move() ? "white" : "black")
                        << " to move, but piece is " << board.get_piece(from_idx)
                        << std::endl;
            return false;
        }

        // to_piece, if exists, is of the opponent colour and not a king
        int to_idx = board.find_piece_at(toX, toY);
        if (to_idx != -1) {
            if (board.get_piece(to_idx).white == board.get_piece(from_idx).white) return false;
            if (!board.is_hypothetical_mode() && (board.get_piece(to_idx).kind == PieceKind::King)) return false;
        }

        // promote_to is consistent with pawn promotion
        if (is_attempted_promotion() && !board.is_hypothetical_mode()) {
            if (!promote_to.has_value()) return false;
        } else {
            if (promote_to.has_value()) return false;
        }

        // Check castling rights
        if (is_attempted_castling()) {
            if (board.get_piece(from_idx).white) {
                if (toX > fromX) {
                    // white kingside
                    if (!board.get_castling_rights().white_kingside) return false;
                } else {
                    // white queenside
                    if (!board.get_castling_rights().white_queenside) return false;
                }
            } else {
                if (toX > fromX) {
                    // black kingside
                    if (!board.get_castling_rights().black_kingside) return false;
                } else {
                    // black queenside
                    if (!board.get_castling_rights().black_queenside) return false;
                }
            }
        }

        // Check en-passant rights
        if (is_attempted_en_passant()) {
            if (!board.has_en_passant()) return false;
            auto [epX, epY] = board.en_passant_target();
            if (toX != epX || toY != epY) return false; // must match en-passant target
        }

        // The piece must be able to move that way according to its movement rules
        switch (board.get_piece(from_idx).kind) {

            case PieceKind::King:
                // one square any direction, or castling
                if (abs(toX - fromX) <= 1 && abs(toY - fromY) <= 1) {
                    if (is_attempted_castling()) {
                        throw std::runtime_error("Attempted castling but King is moving to adjacent square");
                    }
                    return true;
                } else if (is_attempted_castling()) {
                    // Castling (we already checked for castling rights)
                    // King should be in starting square
                    if (fromX != 4) {
                        throw std::runtime_error("Attempted castling with rights, but king not in starting square");
                    }
                    if (board.get_piece(from_idx).white && fromY != 0) {
                        throw std::runtime_error("Attempted castling with rights, but white king not in starting square");
                    }
                    if (!board.get_piece(from_idx).white && fromY != 7) {
                        throw std::runtime_error("Attempted castling with rights, but black king not in starting square");
                    }
                    int rook_idx = -2;
                    if (board.get_piece(from_idx).white) {
                        if (toX > fromX) {
                            // white kingside
                            rook_idx = board.find_piece_at(7, 0);
                        } else {
                            // white queenside
                            rook_idx = board.find_piece_at(0, 0);
                        }
                    } else {
                        if (toX > fromX) {
                            // black kingside
                            rook_idx = board.find_piece_at(7, 7);
                        } else {
                            // black queenside
                            rook_idx = board.find_piece_at(0, 7);
                        }
                    }
                    if (rook_idx == -2) throw std::runtime_error("Internal error in castling rook index");
                    if (rook_idx == -1) throw std::runtime_error("Rook missing for castling");
                    if (board.get_piece(rook_idx).kind != PieceKind::Rook || (
                        board.get_piece(rook_idx).white != board.get_piece(from_idx).white )
                    ) {
                        throw std::runtime_error("Rook missing or wrong colour for castling");
                    }
                    // No pieces in-between castling
                    if (!board.is_path_clear(fromX, fromY, toX, toY)) return false;
                    return true;
                } else {
                    return false;
                }
                throw std::runtime_error("Unreachable code in King move");


            case PieceKind::Queen:
                // any number of squares along rank, file, or diagonal
                if (toX == fromX || toY == fromY || abs(toX - fromX) == abs(toY - fromY)) {
                    // No pieces in-between queen move
                    if (!board.is_path_clear(fromX, fromY, toX, toY)) return false;
                    return true;
                } else {
                    return false;
                }
                throw std::runtime_error("Unreachable code in Queen move");

            

            case PieceKind::Rook:
                // any number of squares along rank or file
                if (toX == fromX || toY == fromY) {
                    // No pieces in-between rook move
                    if (!board.is_path_clear(fromX, fromY, toX, toY)) return false;
                    return true;
                } else {
                    return false;
                }
                throw std::runtime_error("Unreachable code in Rook move");

            case PieceKind::Bishop:
                // any number of squares along diagonal
                if (abs(toX - fromX) == abs(toY - fromY)) {
                    // No pieces in-between bishop move
                    if (!board.is_path_clear(fromX, fromY, toX, toY)) return false;
                    return true;
                } else {
                    return false;
                }
                throw std::runtime_error("Unreachable code in Bishop move");
            
            case PieceKind::Knight:
                // L-shape: 2+1 squares
                if ((abs(toX - fromX) == 2 && abs(toY - fromY) == 1) ||
                    (abs(toX - fromX) == 1 && abs(toY - fromY) == 2)) {
                    return true;
                } else {
                    return false;
                }
                throw std::runtime_error("Unreachable code in Knight move");

            case PieceKind::Pawn: {
                if (fromY == 0 || fromY == 7) throw std::runtime_error("Invalid pawn position");
                int direction = board.get_piece(from_idx).white ? 1 : -1;
                //int initial_rank = board.get_piece(from_idx).white ? 1 : 6;
                
                if (is_attempted_initial_two_square_pawn_move()) {
                    // No in-between piece and target square must be empty
                    if (board.find_piece_at(fromX, fromY + direction) != -1) return false;
                    if (board.find_piece_at(toX, toY) != -1) return false;
                    return true;
                } else if (is_attempted_en_passant()) {
                    // en-passant capture already checked
                    return true;
                } else if (toY - fromY == direction && abs(toX - fromX) == 1) {
                    // diagonal capture or promotion capture
                    if (to_idx == -1) return false;
                    return true;
                } else if (toX == fromX && toY - fromY == direction) {
                    // one-square move forward
                    if (to_idx != -1) return false;
                    return true;
                } else {
                    return false;
                }
            }
                throw std::runtime_error("Unreachable code in Pawn move");

            
            default:
                throw std::runtime_error("Invalid PieceKind in is_valid()");
        }
        // Not reachable
        throw std::runtime_error("Unreachable code in is_valid()");
    }

    bool is_legal(void) const {
        if (tried_to_move) throw std::runtime_error("Cannot query move legality after move");
        if (!is_valid()) return false;
        if (!board.is_hypothetical_mode()) std::cout << " Move " << *this << " is valid, checking legality..." << std::endl;

        int from_idx = board.find_piece_at(fromX, fromY);
        if (from_idx == -1) return false;

        // The following squares must be non-attacked:
        // - Where the mover's king ends up
        // - If castling, where the mover's king moves from and passes through.
        if (!is_attempted_castling()) {
            Move hmove = hypothesize();
            hmove.move(false);
            if (hmove.board.is_player_in_check(false)) {
                // check whether this hypothetical move would've landed us in check
                return false;
            }
        } else {
            // King cannot castle from, through, or into check, so is_player_in_check() does not suffice.
            std::set<std::pair<int,int>> king_squares_to_check;
            king_squares_to_check.insert({fromX, fromY});
            int step = (toX > fromX) ? 1 : -1;
            king_squares_to_check.insert({fromX + step, fromY});
            king_squares_to_check.insert({toX, toY});

            // Make this hypothetical move
            Move hmove = hypothesize();
            hmove.move(false);

            // Go through all the opponent's VALID moves, see if any attack those squares
            for (int i = 0; i < hmove.board.get_piece_count(); ++i) {
                const Piece& opponent_piece = hmove.board.get_piece(i);
                if (opponent_piece.white != hmove.board.is_white_to_move()) continue;

                // Compute all squares the opponent pieces could attack if our king was there and no obstacles
                std::set<std::pair<int,int>> targets = hmove.board.get_targets(i, true);

                for (auto target_square : targets) {
                    // Check that opponent piece moving to target square is a VALID move (e.g. no blocking pieces, etc)
                    Move opponent_move(opponent_piece.x, opponent_piece.y, target_square.first, target_square.second, hmove.board);
                    if (!opponent_move.is_valid()) continue; // not a valid move, maybe because of blocking pieces.
                    if (king_squares_to_check.find(target_square) != king_squares_to_check.end()) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    // Verify if the move is check before moving it
    bool is_check(void) const {
        if (tried_to_move) throw std::runtime_error("Cannot check check-ness after making move");
        Move hmove = hypothesize();
        hmove.move();
        return hmove.board.is_player_in_check(true);
    }
    // Verify if, after making this move hypothetically, opponent would have no legal moves. Don't actually make this move.
    bool no_legal_moves(void) const {
        if (tried_to_move) throw std::runtime_error("Cannot check checkmate-ness after making move");
        Move hmove = hypothesize();
        hmove.move();

        // Check for all possible legal
        for (int i = 0; i < hmove.board.get_piece_count(); ++i) {
            const Piece& piece = hmove.board.get_piece(i);
            if (piece.white != hmove.board.is_white_to_move()) continue;

            // Compute all squares this piece could possibly move to if no obstacles
            std::set<std::pair<int,int>> targets = hmove.board.get_targets(i, false);

            for (auto target_square : targets) {
                Move legal_move(piece.x, piece.y, target_square.first, target_square.second, hmove.board);
                if (legal_move.is_legal()) {
                    std::cout << "\tDEBUG: found legal move: " << legal_move << std::endl;
                    return false;
                }
            }
        }

        // No legal moves found
        return false;
    }
    // TODO check for stalemate-ness/checkmate-ness AFTER making the move?
    // Check for stalemate before making the move
    bool is_stalemate(void) const {
        if (tried_to_move) throw std::runtime_error("Cannot check stalemate-ness after making move");
        if (is_check()) return false;
        return no_legal_moves();
    }
    // Check for checkmate before making the move
    bool is_checkmate(void) const {
        if (tried_to_move) throw std::runtime_error("Cannot check checkmate-ness after making move");
        if (!is_check()) return false;
        return no_legal_moves();
    }

    // Make the move on the board
    void move(void) {
        move(true);
    }

    // NOTE: Check legality is skipped in hypothetical mode
    void move(const bool check_legality) {
        const bool should_increase_halfmove_clock = !is_attempted_capture_or_pawn_move();
        if (tried_to_move) throw std::runtime_error("Cannot make a move twice");
        if (!board.is_hypothetical_mode() && check_legality) {
            if (!is_legal()) {
                throw std::runtime_error("Attempted to make an illegal move");
            }
        }

        // locate mover and possible captured piece
        int from_idx = board.find_piece_at(fromX, fromY);
        if (from_idx == -1) throw std::runtime_error("Internal error: mover not found");
        int to_idx = board.find_piece_at(toX, toY);
        const Piece& mover = board.get_piece(from_idx);

        // Check for captures and castling
        if (is_attempted_castling()) {
            // Move corresponding rook but NOT the king! King is moved later
            int rook_from_x = (toX > fromX) ? 7 : 0;
            int rook_to_x   = (toX > fromX) ? (fromX + 1) : (fromX - 1);
            int rook_idx = board.find_piece_at(rook_from_x, fromY);
            if (rook_idx != -1 && board.get_piece(rook_idx).kind == PieceKind::Rook && board.get_piece(rook_idx).white == mover.white) {
                board.teletransport_piece(rook_idx, rook_to_x, fromY);
            } else {
                throw std::runtime_error("Internal error: same-colour rook not found for castling");
            }

            // Update castling rights for the mover's colour
            CastlingRights cr = board.get_castling_rights();
            if (mover.white) {
                cr.white_kingside = false;
                cr.white_queenside = false;
            } else {
                cr.black_kingside = false;
                cr.black_queenside = false;
            }
            board.set_castling(cr);
        } else if (is_attempted_en_passant()) {
            int direction = mover.white ? 1 : -1;
            // captured pawn sits on (toX, fromY)
            int cap_idx = board.find_piece_at(toX, fromY);
            if (cap_idx == -1 || board.get_piece(cap_idx).kind != PieceKind::Pawn || board.get_piece(cap_idx).white == mover.white) {
                throw std::runtime_error("Internal error: en-passant capture piece not found");
            }
            // If captured index is before mover, adjust from_idx after delete
            bool adjust_from = (cap_idx < from_idx);
            board.delete_piece(cap_idx);
            if (adjust_from) --from_idx;
        } else if (to_idx != -1) {
            // Non-en-passant capture
            if (to_idx == from_idx) {
                throw std::runtime_error("Internal error: mover and target are the same piece");
            }
            const Piece& target = board.get_piece(to_idx);
            if (target.white == mover.white) {
                throw std::runtime_error("Internal error: attempted to capture same-colour piece");
            }

            // If a rook on its original corner is captured, update castling rights
            CastlingRights cr = board.get_castling_rights();
            if (target.kind == PieceKind::Rook) {
                if (target.white) {
                    if (target.x == 0 && target.y == 0) cr.white_queenside = false;
                    if (target.x == 7 && target.y == 0) cr.white_kingside  = false;
                } else {
                    if (target.x == 0 && target.y == 7) cr.black_queenside = false;
                    if (target.x == 7 && target.y == 7) cr.black_kingside  = false;
                }
            }
            board.set_castling(cr);

            // Erase captured piece and adjust mover index if needed
            bool adjust_from = (to_idx < from_idx);
            board.delete_piece(to_idx);
            if (adjust_from) --from_idx;
        }

        // Promote pawn
        if (is_attempted_promotion()) {
            if (!promote_to.has_value()) {
                throw std::runtime_error("Internal error: promotion piece kind not set");
            }
            board.change_pawn_kind(from_idx, promote_to.value());
        }

        // Move the mover (if castling, we already moved the rook)
        board.teletransport_piece(from_idx, toX, toY);

        // Update en-passant rights: set only when a pawn moves two squares; otherwise clear
        if (is_attempted_initial_two_square_pawn_move()) {
            int direction = mover.white ? 1 : -1;
            board.set_en_passant(fromX, fromY + direction); // square passed over
        } else {
            board.clear_en_passant();
        }

        // Update castling rights if mover was a king or rook (or rook moved away from original square)
        CastlingRights cr = board.get_castling_rights();
        if (mover.kind == PieceKind::King) {
            if (mover.white) {
                cr.white_kingside = false;
                cr.white_queenside = false;
            } else {
                cr.black_kingside = false;
                cr.black_queenside = false;
            }
        } else if (mover.kind == PieceKind::Rook) {
            // if rook moved from its initial corner, revoke corresponding right
            if (mover.white) {
                if (fromX == 0 && fromY == 0) cr.white_queenside = false;
                if (fromX == 7 && fromY == 0) cr.white_kingside  = false;
            } else {
                if (fromX == 0 && fromY == 7) cr.black_queenside = false;
                if (fromX == 7 && fromY == 7) cr.black_kingside  = false;
            }
        }
        board.set_castling(cr);
        
        // Increase or reset halfmove clock
        if (should_increase_halfmove_clock) board.increase_halfmove_clock();
        else board.reset_halfmove_clock();

        // Toggle player to move
        board.toggle_white_to_move();

        // Avoid using this instance of `Move` after it has moved.
        tried_to_move = true;  
    } 

};

SCRATCHWORK ENDS HERE

*/
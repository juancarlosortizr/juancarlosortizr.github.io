#include <vector> 
#include <cstdio>
#include <cmath>
#include <iostream>
#include <string>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <system_error>
#include <exception>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif
#include "raylib.h"
#include "piece.h"
#include "board.h"
#include "move.h"
#include "lawyer.h"
#include "game.h"
#include "en_passant.h"
#include "castling.h"
#include "square_utils.h"
#include "algebraic_notation.h"
#include "material.h"
#include "lost_pieces.h"
#include "dfs.h"

namespace fs = std::filesystem;

static fs::path canonical_path(const fs::path& path) {
    std::error_code ec;
    fs::path resolved = fs::weakly_canonical(path, ec);
    if (ec) {
        resolved = path;
    }
    return resolved;
}

static bool assets_present(const fs::path& dir) {
    if (dir.empty()) {
        return false;
    }
    std::error_code ec;
    if (!fs::exists(dir, ec) || ec) {
        return false;
    }
    ec.clear();
    if (!fs::exists(dir / "sprites", ec) || ec) {
        return false;
    }
    ec.clear();
    if (!fs::exists(dir / "sounds_that_cant_be_made", ec) || ec) {
        return false;
    }
    return true;
}

static fs::path executable_directory(const char* argv0) {
#if defined(__APPLE__)
    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) {
        return canonical_path(buffer).parent_path();
    }
#elif defined(_WIN32)
    char buffer[MAX_PATH];
    DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        return canonical_path(buffer).parent_path();
    }
#else
    char buffer[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length > 0) {
        buffer[length] = '\0';
        return canonical_path(buffer).parent_path();
    }
#endif
    if (argv0 && argv0[0] != '\0') {
        fs::path fallback(argv0);
        if (!fallback.is_absolute()) {
            std::error_code ec;
            fs::path cwd = fs::current_path(ec);
            if (!ec) {
                fallback = cwd / fallback;
            }
        }
        fallback = canonical_path(fallback);
        if (fallback.empty()) {
            return {};
        }
        std::error_code ec;
        if (fs::is_directory(fallback, ec) && !ec) {
            return fallback;
        }
        return fallback.parent_path();
    }
    return {};
}

static bool try_resolve_from(fs::path dir) {
    if (dir.empty()) {
        return false;
    }
    std::error_code ec;
    if (!fs::is_directory(dir, ec) || ec) {
        dir = dir.parent_path();
    }
    fs::path previous;
    while (!dir.empty() && dir != previous) {
        if (assets_present(dir)) {
            std::error_code setEc;
            fs::current_path(dir, setEc);
            if (!setEc) {
                return true;
            }
        }
        previous = dir;
        dir = dir.parent_path();
    }
    return false;
}

static void ensure_asset_directory(const char* argv0) {
    std::error_code ec;
    fs::path current = fs::current_path(ec);
    if (!ec && assets_present(current)) {
        return;
    }
    if (try_resolve_from(executable_directory(argv0))) {
        return;
    }
    try_resolve_from(current);
}

static const char* status_to_string(GameStatus status) {
    switch (status) {
        case GameStatus::Checkmate:           return "Checkmate";
        case GameStatus::Stalemate:           return "Stalemate";
        case GameStatus::FiftyMoveRule:       return "50-Move Rule";
        case GameStatus::Ongoing:             return "Ongoing";
        case GameStatus::ThreefoldRepetition: return "3-Fold Repetition";
        default:                              throw std::runtime_error("Unrecognized GameStatus");
    }
}

static const char* winner_to_string(GameWinner winner) {
    switch (winner) {
        case GameWinner::White: return "White";
        case GameWinner::Black: return "Black";
        case GameWinner::Draw:  return "Draw";
        case GameWinner::TBD:   return "TBD";
        default:                throw std::runtime_error("Unrecognized GameWinner");
    }
}

// Board/graphics constants
static const int BOARD_SIZE = 8;
static const int BOARD_PIXEL_SIZE = 600;
static const int SIDEBAR_WIDTH = 180;
static const float BOARD_MARGIN_LEFT = 60.0f;
static const float BOARD_MARGIN_RIGHT = 30.0f;
static const float BOARD_MARGIN_TOP = 80.0f;
static const float BOARD_MARGIN_BOTTOM = 60.0f;

static const Color DARK_GREEN = Color{86, 125, 70, 255};
static const Color LIGHT_GREEN = Color{174, 205, 127, 255};
static constexpr int PIECE_KIND_COUNT = 6;

struct PieceTextures {
    Texture2D white[PIECE_KIND_COUNT];
    Texture2D black[PIECE_KIND_COUNT];
};

static int piece_kind_index(PieceKind kind) {
    switch (kind) {
        case PieceKind::King:   return 0;
        case PieceKind::Queen:  return 1;
        case PieceKind::Rook:   return 2;
        case PieceKind::Bishop: return 3;
        case PieceKind::Knight: return 4;
        case PieceKind::Pawn:   return 5;
        default:                throw std::runtime_error("Piece kind not recognized");
    }
}

static const char* texture_name_for_index(int idx) {
    static const char* names[PIECE_KIND_COUNT] = {
        "king", "queen", "rook", "bishop", "knight", "pawn"
    };
    return names[idx];
}

static Texture2D load_texture_for(const std::string& color, const std::string& name) {
    const std::string path = "sprites/" + color + "/" + name + ".svg.png";
    return LoadTexture(path.c_str());
}

static PieceTextures load_piece_textures() {
    PieceTextures textures{};
    for (int i = 0; i < PIECE_KIND_COUNT; ++i) {
        const char* name = texture_name_for_index(i);
        textures.white[i] = load_texture_for("white", name);
        textures.black[i] = load_texture_for("black", name);
    }
    return textures;
}

static void unload_piece_textures(PieceTextures& textures) {
    for (int i = 0; i < PIECE_KIND_COUNT; ++i) {
        UnloadTexture(textures.white[i]);
        UnloadTexture(textures.black[i]);
    }
}

// Clamp a cell coordinate to [0, BOARD_SIZE-1]
static inline int clamp_cell(int v) {
    if (v < 0) return 0;
    if (v > BOARD_SIZE - 1) return BOARD_SIZE - 1;
    return v;
}

// Convert screen cell Y (0 at top) to board Y (0 = white back rank at bottom)
// screen_row is integer cell index computed as my / cellSize
static inline int screen_col_to_board_col(int screen_col, bool flipped) {
    screen_col = clamp_cell(screen_col);
    return flipped ? (BOARD_SIZE - 1 - screen_col) : screen_col;
}

static inline int screen_row_to_board_row(int screen_row, bool flipped) {
    screen_row = clamp_cell(screen_row);
    return flipped ? screen_row : (BOARD_SIZE - 1 - screen_row);
}

static inline int board_col_to_screen_col(int col, bool flipped) {
    return flipped ? (BOARD_SIZE - 1 - col) : col;
}

static inline int board_row_to_screen_row(int row, bool flipped) {
    return flipped ? row : (BOARD_SIZE - 1 - row);
}

// Convert board row (0 = white back rank at bottom) to screen y coordinate (pixels)
static inline float board_row_to_screen_y(int board_row, float cellSize, float boardOffsetY, bool flipped) {
    int screen_row = board_row_to_screen_row(board_row, flipped);
    return boardOffsetY + screen_row * cellSize + cellSize * 0.5f;
}

// Convert board column to screen x coordinate (pixels)
static inline float board_col_to_screen_x(int col, float cellSize, float boardOffsetX, bool flipped) {
    int screen_col = board_col_to_screen_col(col, flipped);
    return boardOffsetX + screen_col * cellSize + cellSize * 0.5f;
}

// Draw pieces (handles dragging visual as well)
static void draw_pieces(const Board& board, float cellSize,
                        bool dragging,
                        int selected, float dragX, float dragY,
                        int origX, int origY,
                        bool promotion_popup,         // are we in a promotion popup state?
                        bool pending_promotion_white, // ignored if not promotion_popup
                        const PieceTextures& textures,
                        float boardOffsetX,
                        float boardOffsetY,
                        bool flipped
)
{
    if (promotion_popup && dragging) {
        throw std::runtime_error("Cannot drag during promotion popup");
    }

    // highlight source square while dragging (origX/origY are board coords)
    if (dragging && selected >= 0) {
        const int screen_col = board_col_to_screen_col(origX, flipped);
        const int screen_row = board_row_to_screen_row(origY, flipped);
        float sx = boardOffsetX + screen_col * cellSize;
        float sy = boardOffsetY + screen_row * cellSize;
        DrawRectangleLines((int)sx, (int)sy, (int)cellSize, (int)cellSize, GREEN);
    }

    // Draw all pieces
    for (int i = 0; i < board.get_piece_count(); ++i) {
        float cx, cy;
        const Piece& pc = board.get_piece(i);
        bool isDraggingThis = dragging && i == selected;
        if (isDraggingThis) {
            cx = dragX;
            cy = dragY;
        } else {
            cx = board_col_to_screen_x(pc.x, cellSize, boardOffsetX, flipped);
            cy = board_row_to_screen_y(pc.y, cellSize, boardOffsetY, flipped);
        }

        // optional rounded background (transparent here)
        float pad = cellSize * 0.1f;
        DrawRectangleRounded({cx - cellSize/2 + pad, cy - cellSize/2 + pad, cellSize - pad*2, cellSize - pad*2}, 0.05f, 4, Fade(LIGHTGRAY, 0.0f));
        const int texIdx = piece_kind_index(pc.kind);
        const Texture2D& tex = pc.white ? textures.white[texIdx] : textures.black[texIdx];
        Rectangle src{0.0f, 0.0f, (float)tex.width, (float)tex.height};
        Rectangle dst{cx - cellSize * 0.5f, cy - cellSize * 0.5f, cellSize, cellSize};
        DrawTexturePro(tex, src, dst, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
    }

    // Draw promotion popup
    if (promotion_popup) {
        const int screenW = GetScreenWidth();
        const int screenH = GetScreenHeight();
        const float popup_w = cellSize * PROMO_OPTIONS;
        const float popup_h = cellSize;
        const float px = screenW * 0.5f - popup_w * 0.5f;
        const float py = screenH * 0.5f - popup_h * 0.5f;

        const Color overlayCol = Fade(BLACK, 0.45f);
        DrawRectangle(0, 0, screenW, screenH, overlayCol);

        const float pad = cellSize * 0.15f;
        Rectangle popupRect{px - pad, py - pad, popup_w + pad * 2.0f, popup_h + pad * 2.0f};
        const Color windowCol = Fade(RAYWHITE, 0.95f);
        const Color frameCol = Fade(BLACK, 0.6f);
        DrawRectangleRounded(popupRect, 0.12f, 6, windowCol);
        DrawRectangleRoundedLines(popupRect, 0.12f, 6, frameCol);

        const Color lightCell = pending_promotion_white ? Fade(LIGHTGRAY, 0.9f) : Fade(DARKGRAY, 0.85f);
        const Color darkCell = pending_promotion_white ? Fade(GRAY, 0.9f) : Fade(GRAY, 0.7f);
        const Color borderCol = Fade(BLACK, 0.5f);
        const float iconPad = cellSize * 0.15f;

        for (int i = 0; i < PROMO_OPTIONS; ++i) {
            const float cellX = px + i * cellSize;
            Rectangle cellRect{cellX, py, cellSize, popup_h};
            DrawRectangleRec(cellRect, (i & 1) ? darkCell : lightCell);
            DrawRectangleLinesEx(cellRect, 2.0f, borderCol);

            const int idx = piece_kind_index(promoKinds[i]);
            const Texture2D& tex = pending_promotion_white ? textures.white[idx] : textures.black[idx];
            Rectangle src{0.0f, 0.0f, (float)tex.width, (float)tex.height};
            Rectangle dst{cellRect.x + iconPad, cellRect.y + iconPad,
                          cellRect.width - iconPad * 2.0f, cellRect.height - iconPad * 2.0f};
            DrawTexturePro(tex, src, dst, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
        }
    }
}

static bool status_allows_resume(GameStatus status) {
    switch (status) {
        case GameStatus::Checkmate:
        case GameStatus::Stalemate:
        case GameStatus::FiftyMoveRule:
        case GameStatus::ThreefoldRepetition:
            return true;
        default:
            return false;
    }
}

static void compute_game_over_popup_geometry(float cellSize,
                                             bool includeUndoButton,
                                             Rectangle& window,
                                             Rectangle& playAgainButton,
                                             Rectangle& undoButton) {
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();
    const float popup_w = std::min((float)screenW - cellSize * 0.8f, cellSize * 6.8f);
    const float popup_h = std::min((float)screenH - cellSize * 0.8f, cellSize * 4.5f);
    const float px = screenW * 0.5f - popup_w * 0.5f;
    const float py = screenH * 0.5f - popup_h * 0.5f;
    window = Rectangle{px, py, popup_w, popup_h};
    const float button_w = includeUndoButton ? popup_w * 0.65f : popup_w * 0.5f;
    const float button_h = cellSize * 0.9f;
    const float button_x = screenW * 0.5f - button_w * 0.5f;
    const float spacing = includeUndoButton ? cellSize * 0.2f : 0.0f;
    const float bottom_y = py + popup_h - button_h - cellSize * 0.5f;
    undoButton = includeUndoButton ? Rectangle{button_x, bottom_y, button_w, button_h}
                                   : Rectangle{0.0f, 0.0f, 0.0f, 0.0f};
    const float playBtnY = includeUndoButton ? (bottom_y - button_h - spacing) : bottom_y;
    playAgainButton = Rectangle{button_x, playBtnY, button_w, button_h};
}

static void draw_game_over_popup(float cellSize, GameStatus status, GameWinner winner) {
    Rectangle window{};
    Rectangle playAgainButton{};
    Rectangle undoButton{};
    const bool showUndoButton = status_allows_resume(status);
    compute_game_over_popup_geometry(cellSize, showUndoButton, window, playAgainButton, undoButton);
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.55f));
    DrawRectangleRounded(window, 0.12f, 6, Fade(RAYWHITE, 0.97f));
    DrawRectangleRoundedLines(window, 0.12f, 6, Fade(BLACK, 0.5f));

    const std::string resultLine = std::string("Result: ") + status_to_string(status);
    const std::string winnerLine = std::string("Winner: ") + winner_to_string(winner);
    const float marginX = cellSize * 0.4f;
    const float availableWidth = window.width - marginX * 2.0f;
    auto fitted_font = [&](const std::string& text, int start) {
        int font = start;
        while (font > 14 && MeasureText(text.c_str(), font) > availableWidth) {
            font -= 2;
        }
        return font;
    };
    const int baseFont = std::min(42, std::max(18, (int)(cellSize * 0.55f)));
    const int resultFont = fitted_font(resultLine, baseFont);
    const int winnerFont = fitted_font(winnerLine, baseFont - 4);
    const int resultWidth = MeasureText(resultLine.c_str(), resultFont);
    const int winnerWidth = MeasureText(winnerLine.c_str(), winnerFont);
    const float resultX = window.x + (window.width - resultWidth) * 0.5f;
    const float winnerX = window.x + (window.width - winnerWidth) * 0.5f;
    const float resultY = window.y + cellSize * 0.5f;
    const float winnerY = resultY + resultFont + cellSize * 0.4f;
    DrawText(resultLine.c_str(), (int)resultX, (int)resultY, resultFont, DARKGRAY);
    DrawText(winnerLine.c_str(), (int)winnerX, (int)winnerY, winnerFont, DARKGRAY);

    const Vector2 mousePoint{(float)GetMouseX(), (float)GetMouseY()};
    auto draw_popup_button = [&](const Rectangle& rect, const char* label) {
        if (rect.width <= 0.0f || rect.height <= 0.0f) {
            return;
        }
        const bool hover = CheckCollisionPointRec(mousePoint, rect);
        const Color btnColor = hover ? Fade(GREEN, 0.8f) : Fade(LIGHTGRAY, 0.85f);
        DrawRectangleRounded(rect, 0.2f, 6, btnColor);
        DrawRectangleRoundedLines(rect, 0.2f, 6, Fade(BLACK, 0.4f));
        const int btnFont = std::max(18, (int)(cellSize * 0.5f));
        const int textWidth = MeasureText(label, btnFont);
        DrawText(label,
                 (int)(rect.x + rect.width * 0.5f - textWidth * 0.5f),
                 (int)(rect.y + rect.height * 0.5f - btnFont * 0.5f),
                 btnFont,
                 BLACK);
    };

    draw_popup_button(playAgainButton, "Play Again");
    if (showUndoButton) {
        draw_popup_button(undoButton, "Undo Last Move");
    }
}

int main(int argc, char** argv) {
    ensure_asset_directory((argc > 0) ? argv[0] : nullptr);
    const float sidebarWidth = (float)SIDEBAR_WIDTH;
    const float boardMarginLeft = BOARD_MARGIN_LEFT;
    const float boardMarginRight = BOARD_MARGIN_RIGHT;
    const float boardMarginTop = BOARD_MARGIN_TOP;
    const float boardMarginBottom = BOARD_MARGIN_BOTTOM;
    const int screenHeight = (int)(BOARD_PIXEL_SIZE + boardMarginTop + boardMarginBottom);
    const int screenWidth = (int)(SIDEBAR_WIDTH + BOARD_PIXEL_SIZE + boardMarginLeft + boardMarginRight);
    const float boardOffsetX = sidebarWidth + boardMarginLeft;
    const float boardOffsetY = boardMarginTop;
    const float boardPixelSize = (float)BOARD_PIXEL_SIZE;

    InitWindow(screenWidth, screenHeight, "Chess!");
    InitAudioDevice();
    SetTargetFPS(60);

    const float cellSize = boardPixelSize / (float)BOARD_SIZE;

    // Initialize game
    Game game;
    constexpr bool AI_PLAYS_WHITE = false;
    constexpr bool HUMAN_PLAYS_WHITE = !AI_PLAYS_WHITE;
    DFS dfs_agent(make_material_oracle(), AI_PLAYS_WHITE);
    DFS::MAX_DEPTH = 2;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if ((arg == "--max-depth" || arg == "-d") && i + 1 < argc) {
                try {
                    DFS::MAX_DEPTH = std::max(1, std::stoi(argv[++i]));
                } catch (const std::exception&) {
                    std::cerr << "Invalid depth value; using default " << DFS::MAX_DEPTH << "\n";
                }
            }
        }
    }
    bool ai_pending_move = false;

    // Load piece textures
    PieceTextures piece_textures = load_piece_textures();

    // Drag and UI state
    bool dragging = false;
    int selected = -1;            // index into pieces or -1
    float dragX = 0.f, dragY = 0.f;
    int origX = -1, origY = -1;   // original cell before drag (board coords)
    bool view_flipped = false;

    bool promotion_popup = false;
    bool pending_promotion_white = true;   // colour of piece to promote (for drawing)
    int pending_selected = -1;             // selected piece index
    int pending_origX = -1, pending_origY = -1;
    Move* pending_move = nullptr;
    std::vector<std::string> notation_undo_stack;
    std::vector<std::string> notation_redo_stack;
    std::string last_white_move;
    std::string last_black_move;
    std::vector<PieceKind> lost_white_pieces;
    std::vector<PieceKind> lost_black_pieces;

    bool game_over_popup = false;
    GameStatus game_over_status = GameStatus::Ongoing;
    GameWinner game_over_winner = GameWinner::TBD;

    const float buttonWidth = sidebarWidth * 0.75f;
    const float buttonHeight = 60.0f;
    const float buttonSpacing = 18.0f;
    const int buttonCount = 4;
    const float buttonsTotalHeight = buttonHeight * buttonCount + buttonSpacing * (buttonCount - 1);
    const float buttonStartY = (screenHeight - buttonsTotalHeight) * 0.5f;
    const float buttonX = (sidebarWidth - buttonWidth) * 0.5f;
    const Rectangle undoButtonArea{buttonX, buttonStartY, buttonWidth, buttonHeight};
    const Rectangle redoButtonArea{buttonX, buttonStartY + buttonHeight + buttonSpacing, buttonWidth, buttonHeight};
    const Rectangle resetButtonArea{buttonX, buttonStartY + (buttonHeight + buttonSpacing) * 2.0f, buttonWidth, buttonHeight};
    const Rectangle flipButtonArea{buttonX, buttonStartY + (buttonHeight + buttonSpacing) * 3.0f, buttonWidth, buttonHeight};

    auto refresh_move_display = [&]() {
        last_white_move.clear();
        last_black_move.clear();
        for (int i = (int)notation_undo_stack.size() - 1; i >= 0; --i) {
            if ((i & 1) == 0) {  // white moves at even indices
                last_white_move = notation_undo_stack[i];
                break;
            }
        }
        for (int i = (int)notation_undo_stack.size() - 1; i >= 0; --i) {
            if ((i & 1) == 1) {  // black moves at odd indices
                last_black_move = notation_undo_stack[i];
                break;
            }
        }
    };

    auto clear_interaction_state = [&]() {
        dragging = false;
        selected = -1;
        dragX = dragY = 0.f;
        origX = origY = -1;
        promotion_popup = false;
        pending_promotion_white = true;
        pending_selected = -1;
        pending_origX = pending_origY = -1;
        if (pending_move) {
            delete pending_move;
            pending_move = nullptr;
        }
    };

    auto reset_game_state = [&]() {
        game.reset();
        clear_interaction_state();
        game_over_status = GameStatus::Ongoing;
        game_over_winner = GameWinner::TBD;
        game_over_popup = false;
        notation_undo_stack.clear();
        notation_redo_stack.clear();
        refresh_move_display();
        ai_pending_move = false;
    };

    auto apply_status_ui_state = [&]() {
        game_over_status = game.status();
        game_over_winner = game.winner();
        if (game_over_status == GameStatus::Ongoing) {
            game_over_popup = false;
        } else {
            game_over_popup = true;
        }
    };

    auto undo_last_move = [&]() -> bool {
        const int moves_to_rewind = 2;
        if ((int)notation_undo_stack.size() < moves_to_rewind) {
            return false;
        }
        int undone = 0;
        while (undone < moves_to_rewind) {
            if (!game.undo()) {
                break;
            }
            if (!notation_undo_stack.empty()) {
                notation_redo_stack.push_back(notation_undo_stack.back());
                notation_undo_stack.pop_back();
            }
            ++undone;
        }
        if (undone == moves_to_rewind) {
            clear_interaction_state();
            apply_status_ui_state();
            refresh_move_display();
            ai_pending_move = false;
            return true;
        }
        while (undone > 0) {
            if (game.redo()) {
                if (!notation_redo_stack.empty()) {
                    notation_undo_stack.push_back(notation_redo_stack.back());
                    notation_redo_stack.pop_back();
                }
            }
            --undone;
        }
        return false;
    };
    auto redo_last_move = [&]() -> bool {
        const int moves_to_apply = 2;
        if ((int)notation_redo_stack.size() < moves_to_apply) {
            return false;
        }
        int redone = 0;
        while (redone < moves_to_apply) {
            if (!game.redo()) {
                break;
            }
            if (!notation_redo_stack.empty()) {
                notation_undo_stack.push_back(notation_redo_stack.back());
                notation_redo_stack.pop_back();
            }
            ++redone;
        }
        if (redone == moves_to_apply) {
            clear_interaction_state();
            apply_status_ui_state();
            refresh_move_display();
            ai_pending_move = false;
            return true;
        }
        while (redone > 0) {
            if (game.undo()) {
                if (!notation_undo_stack.empty()) {
                    notation_redo_stack.push_back(notation_undo_stack.back());
                    notation_undo_stack.pop_back();
                }
            }
            --redone;
        }
        return false;
    };
    auto mouse_to_board = [&](int mx, int my, int& cellX, int& cellY) -> bool {
        float relX = (float)mx - boardOffsetX;
        float relY = (float)my - boardOffsetY;
        if (relX < 0.0f || relX >= boardPixelSize) return false;
        if (relY < 0.0f || relY >= boardPixelSize) return false;
        int screenCol = clamp_cell((int)(relX / cellSize));
        int screenRow = clamp_cell((int)(relY / cellSize));
        cellX = screen_col_to_board_col(screenCol, view_flipped);
        cellY = screen_row_to_board_row(screenRow, view_flipped);
        return true;
    };

    bool audioReady = IsAudioDeviceReady();
    Sound moveSound = audioReady ? LoadSound("sounds_that_cant_be_made/move.wav") : Sound{};
    auto play_move_sound = [&]() {
        if (audioReady) {
            PlaySound(moveSound);
        }
    };
    Sound checkSound = audioReady ? LoadSound("sounds_that_cant_be_made/check.wav") : Sound{};
    auto play_check_sound = [&]() {
        if (audioReady) {
            PlaySound(checkSound);
        }
    };
    Sound checkmateSound = audioReady ? LoadSound("sounds_that_cant_be_made/checkmate.mp3") : Sound{};
    auto play_checkmate_sound = [&]() {
        if (audioReady) {
            PlaySound(checkmateSound);
        }
    };
    Sound stalemateSound = audioReady ? LoadSound("sounds_that_cant_be_made/stalemate.mp3") : Sound{};
    auto play_stalemate_sound = [&]() {
        if (audioReady) {
            PlaySound(stalemateSound);
        }
    };
    Sound captureSound = audioReady ? LoadSound("sounds_that_cant_be_made/capture.wav") : Sound{};
    auto play_capture_sound = [&]() {
        if (audioReady) {
            PlaySound(captureSound);
        }
    };
    Sound bounceSound = audioReady ? LoadSound("sounds_that_cant_be_made/bounce.mp3") : Sound{};
    auto play_bounce_sound = [&]() {
        if (audioReady) {
            PlaySound(bounceSound);
        }
    };
    Sound undoSound = audioReady ? LoadSound("sounds_that_cant_be_made/undo.wav") : Sound{};
    auto play_undo_sound = [&]() {
        if (audioReady) {
            PlaySound(undoSound);
        }
    };
    Sound redoSound = audioReady ? LoadSound("sounds_that_cant_be_made/redo.wav") : Sound{};
    auto play_redo_sound = [&]() {
        if (audioReady) {
            PlaySound(redoSound);
        }
    };
    Sound fiftyMoveRuleSound = audioReady ? LoadSound("sounds_that_cant_be_made/fifty_move_rule.mp3") : Sound{};
    auto play_fifty_move_rule_sound = [&]() {
        if (audioReady) {
            PlaySound(fiftyMoveRuleSound);
        }
    };
    Sound threefoldRepetitionSound = audioReady ? LoadSound("sounds_that_cant_be_made/3_fold_repetition.wav") : Sound{};
    auto play_3_fold_repetition_sound = [&]() {
        if (audioReady) {
            PlaySound(threefoldRepetitionSound);
        }
    };
    Sound resetSound = audioReady ? LoadSound("sounds_that_cant_be_made/reset.wav") : Sound{};
    auto play_reset_sound = [&]() {
        if (audioReady) {
            PlaySound(resetSound);
        }
    };
    Sound flipSound = audioReady ? LoadSound("sounds_that_cant_be_made/flip.wav") : Sound{};
    auto play_flip_sound = [&]() {
        if (audioReady) {
            PlaySound(flipSound);
        }
    };
    Sound mateInOneSound = audioReady ? LoadSound("sounds_that_cant_be_made/john_cena.mp3") : Sound{};
    auto play_mate_in_one_sound = [&]() {
        if (audioReady) {
            PlaySound(mateInOneSound);
        }
    };
    auto stop_all_sounds = [&]() {
        if (!audioReady) return;
        StopSound(moveSound);
        StopSound(checkSound);
        StopSound(checkmateSound);
        StopSound(stalemateSound);
        StopSound(captureSound);
        StopSound(bounceSound);
        StopSound(undoSound);
        StopSound(redoSound);
        StopSound(fiftyMoveRuleSound);
        StopSound(threefoldRepetitionSound);
        StopSound(resetSound);
        StopSound(flipSound);
        StopSound(mateInOneSound);
    };
    auto trigger_game_over_popup = [&]() {
        if (game.status() == GameStatus::Ongoing) {
            return;
        }
        game_over_popup = true;
        game_over_status = game.status();
        game_over_winner = game.winner();
    };
    auto make_move_and_play_sound = [&](const Move& move) {
        // This function assumes the move has already been verified.
        const bool mover_white = game.board().is_white_to_move();
        const std::string notation = to_algebraic_notation(move, game.board());
        const bool is_capture = move.is_attempted_capture();
        int ok = game.verify_and_move(move);  // ignore -2 (invalid), -1 (illegal), or 0 (OK)
        if (ok != 0) {
            return ok;
        }
        notation_undo_stack.push_back(notation);
        notation_redo_stack.clear();
        refresh_move_display();
        bool is_check = game.board().is_player_in_check(game.board().is_white_to_move());
        const GameStatus status = game.status();
        switch (status) {
            case GameStatus::Checkmate:
                play_checkmate_sound();
                break;
            case GameStatus::Stalemate:
                play_stalemate_sound();
                break;
            case GameStatus::FiftyMoveRule:
                play_fifty_move_rule_sound();
                break;
            case GameStatus::ThreefoldRepetition:
                play_3_fold_repetition_sound();
                break;
            case GameStatus::Ongoing:
                if (is_check) {
                    play_check_sound();
                } else if (is_capture) {
                    play_capture_sound();
                } else {
                    play_move_sound();
                }
                break;
            default:
                break;
        }
        if (status != GameStatus::Ongoing) {
            trigger_game_over_popup();
        }
        const bool mate_in_one_available = Lawyer::instance().has_mate_in_one(game.board());
        if (mate_in_one_available) {
            play_mate_in_one_sound();
        }
        return ok;
    };

    while (!WindowShouldClose()) {

        int mx = GetMouseX();
        int my = GetMouseY();

        Vector2 mousePoint{(float)mx, (float)my};
        bool mouseClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        const bool canUseHistoryKeys = !dragging && !promotion_popup && !game_over_popup;
        const bool leftPressed = IsKeyPressed(KEY_LEFT);
        const bool rightPressed = IsKeyPressed(KEY_RIGHT);

        if (leftPressed && canUseHistoryKeys) {
            stop_all_sounds();
            if (undo_last_move()) {
                play_undo_sound();
            } else {
                play_bounce_sound();
            }
        }
        if (rightPressed && canUseHistoryKeys) {
            stop_all_sounds();
            if (redo_last_move()) {
                play_redo_sound();
            } else {
                play_bounce_sound();
            }
        }

        if (mouseClicked) {
            if (CheckCollisionPointRec(mousePoint, undoButtonArea)) {
                stop_all_sounds();
                if (undo_last_move()) {
                    play_undo_sound();
                } else {
                    play_bounce_sound();
                }
            } else if (CheckCollisionPointRec(mousePoint, redoButtonArea)) {
                stop_all_sounds();
                if (redo_last_move()) {
                    play_redo_sound();
                } else {
                    play_bounce_sound();
                }
            } else if (CheckCollisionPointRec(mousePoint, resetButtonArea)) {
                stop_all_sounds();
                reset_game_state();
                play_reset_sound();
            } else if (CheckCollisionPointRec(mousePoint, flipButtonArea)) {
                stop_all_sounds();
                view_flipped = !view_flipped;
                clear_interaction_state();
                play_flip_sound();
            } else if (game_over_popup) {
                Rectangle window{}, playAgainButton{}, undoButton{};
                const bool showUndoButton = status_allows_resume(game_over_status);
                compute_game_over_popup_geometry(cellSize, showUndoButton, window, playAgainButton, undoButton);
                if (CheckCollisionPointRec(mousePoint, playAgainButton)) {
                    stop_all_sounds();
                    reset_game_state();
                    play_reset_sound();
                } else if (showUndoButton && CheckCollisionPointRec(mousePoint, undoButton)) {
                    stop_all_sounds();
                    if (undo_last_move()) {
                        play_undo_sound();
                    } else {
                        play_bounce_sound();
                    }
                }
            } else {
                int clickedCellX = 0;
                int clickedCellY = 0;
                bool inside_board = mouse_to_board(mx, my, clickedCellX, clickedCellY);

                if (promotion_popup) {
                    float popup_w = cellSize * PROMO_OPTIONS;
                    float popup_h = cellSize;
                    float px = screenWidth * 0.5f - popup_w * 0.5f;
                    float py = screenHeight * 0.5f - popup_h * 0.5f;
                    if (mx >= px && mx < px + popup_w && my >= py && my < py + popup_h) {
                        int choice = (int)((mx - px) / cellSize);
                        pending_move->set_promotion(promoKinds[choice]);
                        promotion_popup = false;
                        dragging = false;
                        selected = -1;
                        origX = origY = -1;
                        pending_selected = -1;
                        const int promo_result = make_move_and_play_sound(*pending_move);
                        if (promo_result != 0) {
                            throw std::runtime_error("Promotion moves should always be legal");
                        }
                        ai_pending_move = (game.board().is_white_to_move() == AI_PLAYS_WHITE);
                        delete pending_move;
                        pending_move = nullptr;
                    }
                } else if (!dragging) {
                    if (inside_board) {
                        int idx = game.board().find_piece_at(clickedCellX, clickedCellY);
                        if (idx != -1) {
                            const Piece& p = game.board().get_piece(idx);
                            if (game.board().is_white_to_move() != HUMAN_PLAYS_WHITE || p.white != HUMAN_PLAYS_WHITE) {
                                play_bounce_sound();
                            } else {
                                dragging = true;
                                selected = idx;
                                dragX = (float)mx;
                                dragY = (float)my;
                                origX = p.x;
                                origY = p.y;
                            }
                        }
                    }
                } else {
                    if (!inside_board) {
                        play_bounce_sound();
                        dragging = false;
                        selected = -1;
                        origX = origY = -1;
                    } else {
                        int occupant = game.board().find_piece_at(clickedCellX, clickedCellY);

                        if (occupant != selected) {
                            const Piece& selectedPiece = game.board().get_piece(selected);
                            Move move{selectedPiece.x, selectedPiece.y,
                                      clickedCellX, clickedCellY, game.board()};

                            if (move.is_attempted_promotion()) {
                                Lawyer& lawyer = Lawyer::instance();
                                bool attempted_promotion_would_be_legal = lawyer.attempted_promotion_would_be_legal(game.board(), move);
                                if (!attempted_promotion_would_be_legal) {
                                    play_bounce_sound();
                                } else {
                                    promotion_popup = true;
                                    pending_move = new Move{selectedPiece.x, selectedPiece.y,
                                                            clickedCellX, clickedCellY, game.board()};
                                    pending_promotion_white = selectedPiece.white;
                                    pending_selected = selected;
                                    pending_origX = origX;
                                    pending_origY = origY;
                                    dragging = false;
                                }
                            } else {
                                const int move_result = make_move_and_play_sound(move);
                                if (move_result != 0) {
                                    play_bounce_sound();
                                } else {
                                    ai_pending_move = (game.board().is_white_to_move() == AI_PLAYS_WHITE);
                                }
                            }
                        }

                        if (!promotion_popup) {
                            dragging = false;
                            selected = -1;
                            origX = origY = -1;
                        }
                    }
                }
            }
        }

        // while dragging, follow mouse
        if (dragging && selected >= 0) {
            dragX = (float)mx;
            dragY = (float)my;
        }

        if (ai_pending_move && !dragging && !promotion_popup && !game_over_popup) {
            if (game.status() != GameStatus::Ongoing) {
                ai_pending_move = false;
            } else if (game.board().is_white_to_move() != AI_PLAYS_WHITE) {
                ai_pending_move = false;
            } else {
                try {
                    Move ai_move = dfs_agent.explore(game.board());
                    const int ai_result = make_move_and_play_sound(ai_move);
                    if (ai_result != 0) {
                        ai_pending_move = false;
                    } else {
                        ai_pending_move = (game.board().is_white_to_move() == AI_PLAYS_WHITE);
                    }
                } catch (const std::exception& ex) {
                    std::cerr << "AI move failed: " << ex.what() << std::endl;
                    ai_pending_move = false;
                }
            }
        }
 
        piece_loss::compute_lost_pieces(game.board(), lost_white_pieces, lost_black_pieces);

        BeginDrawing();
        ClearBackground(DARK_GREEN);

        // Sidebar background
        const Color sidebarBg = Color{58, 83, 52, 255};
        DrawRectangle(0, 0, (int)sidebarWidth, screenHeight, sidebarBg);

        auto draw_sidebar_button = [&](const Rectangle& area, const char* text, bool hover) {
            const Color base = hover ? Fade(LIME, 0.9f) : Fade(RAYWHITE, 0.85f);
            DrawRectangleRounded(area, 0.2f, 6, base);
            DrawRectangleRoundedLines(area, 0.2f, 6, Fade(BLACK, 0.4f));
            const int fontSize = 24;
            const int textWidth = MeasureText(text, fontSize);
            DrawText(text,
                     (int)(area.x + area.width * 0.5f - textWidth * 0.5f),
                     (int)(area.y + area.height * 0.5f - fontSize * 0.55f),
                     fontSize,
                     BLACK);
        };

        draw_sidebar_button(undoButtonArea, "Undo", CheckCollisionPointRec(mousePoint, undoButtonArea));
        draw_sidebar_button(redoButtonArea, "Redo", CheckCollisionPointRec(mousePoint, redoButtonArea));
        draw_sidebar_button(resetButtonArea, "Reset", CheckCollisionPointRec(mousePoint, resetButtonArea));
        draw_sidebar_button(flipButtonArea, "Flip", CheckCollisionPointRec(mousePoint, flipButtonArea));

        const int materialScore = material::balance(game.board());
        char materialBuffer[16];
        if (materialScore > 0) {
            std::snprintf(materialBuffer, sizeof(materialBuffer), "+%d", materialScore);
        } else {
            std::snprintf(materialBuffer, sizeof(materialBuffer), "%d", materialScore);
        }
        const std::string materialText = std::string("Material: ") + materialBuffer;
        const Color materialColor = materialScore > 0 ? WHITE : (materialScore < 0 ? BLACK : DARKGRAY);
        const Color materialBg =
            materialScore > 0 ? Fade(DARK_GREEN, 0.7f)
                              : (materialScore < 0 ? Fade(RAYWHITE, 0.9f) : Fade(LIGHTGRAY, 0.8f));
        const Color materialBorder = materialScore > 0 ? Fade(LIME, 0.5f) : Fade(BLACK, 0.4f);
        const float materialWidth = 150.0f;
        const float materialHeight = 46.0f;
        float materialY = boardOffsetY - materialHeight - 8.0f;
        if (materialY < 8.0f) {
            materialY = 8.0f;
        }
        const float materialX = boardOffsetX + boardPixelSize - materialWidth;
        Rectangle materialRect{materialX, materialY, materialWidth, materialHeight};
        DrawRectangleRounded(materialRect, 0.2f, 6, materialBg);
        DrawRectangleRoundedLines(materialRect, 0.2f, 6, materialBorder);
        const int materialFont = 20;
        const int materialWidthText = MeasureText(materialText.c_str(), materialFont);
        DrawText(materialText.c_str(),
                 (int)(materialRect.x + materialRect.width * 0.5f - materialWidthText * 0.5f),
                 (int)(materialRect.y + materialRect.height * 0.5f - materialFont * 0.5f),
                 materialFont,
                 materialColor);

        const bool whiteToMove = game.board().is_white_to_move();
        const float moveAreaMargin = 12.0f;
        const float moveAreaTop = 30.0f;
        const float moveAreaBottom = buttonStartY - moveAreaMargin;
        if (moveAreaBottom > moveAreaTop + moveAreaMargin) {
            Rectangle movesRect{
                moveAreaMargin,
                moveAreaTop,
                sidebarWidth - moveAreaMargin * 2.0f,
                moveAreaBottom - moveAreaTop
            };
            DrawRectangleRounded(movesRect, 0.15f, 6, Fade(RAYWHITE, 0.3f));
            DrawRectangleRoundedLines(movesRect, 0.15f, 6, Fade(BLACK, 0.3f));

            const float innerPad = 10.0f;
            const float columnSpacing = 12.0f;
            const char* whiteLabel = "White";
            const char* blackLabel = "Black";
            const std::string whiteMoveText = last_white_move.empty() ? "--" : last_white_move;
            const std::string blackMoveText = last_black_move.empty() ? "--" : last_black_move;
            const int columnCount = whiteToMove ? 2 : 1;
            const float usableWidth = movesRect.width - innerPad * 2.0f - columnSpacing * (columnCount - 1);
            const float columnWidth = (columnCount > 0) ? (usableWidth / columnCount) : usableWidth;
            float columnX = movesRect.x + innerPad;

            auto draw_move_column = [&](float x, const char* label, const std::string& value) {
                const int labelFont = 16;
                const int moveFont = 20;
                const int labelWidth = MeasureText(label, labelFont);
                DrawText(label,
                         (int)(x + columnWidth * 0.5f - labelWidth * 0.5f),
                         (int)(movesRect.y + innerPad),
                         labelFont,
                         RAYWHITE);
                const int moveWidth = MeasureText(value.c_str(), moveFont);
                float moveY = movesRect.y + innerPad + labelFont + 6.0f;
                DrawText(value.c_str(),
                         (int)(x + columnWidth * 0.5f - moveWidth * 0.5f),
                         (int)moveY,
                         moveFont,
                         WHITE);
            };

            draw_move_column(columnX, whiteLabel, whiteMoveText);
            if (whiteToMove) {
                columnX += columnWidth + columnSpacing;
                draw_move_column(columnX, blackLabel, blackMoveText);
            }
        }

        const int halfmoveClock = game.board().get_halfmove_clock();
        const double halfmoveDisplay = halfmoveClock * 0.5;
        char clockBuffer[32];
        std::snprintf(clockBuffer, sizeof(clockBuffer), "50-move: %.1f", halfmoveDisplay);
        const std::string clockText(clockBuffer);
        const int clockFont = 20;
        const int clockWidth = MeasureText(clockText.c_str(), clockFont);
        const float clockX = sidebarWidth * 0.5f - clockWidth * 0.5f;
        const float clockY = flipButtonArea.y + flipButtonArea.height + 30.0f;
        DrawText(clockText.c_str(), (int)clockX, (int)clockY, clockFont, RAYWHITE);

        const float infoMargin = 12.0f;
        const float infoTop = clockY + clockFont + 20.0f;
        const float infoBottom = (float)screenHeight - infoMargin;
        if (infoBottom > infoTop) {
            Rectangle infoRect{
                infoMargin,
                infoTop,
                sidebarWidth - infoMargin * 2.0f,
                infoBottom - infoTop
            };
            DrawRectangleRounded(infoRect, 0.1f, 6, Fade(RAYWHITE, 0.25f));
            DrawRectangleRoundedLines(infoRect, 0.1f, 6, Fade(BLACK, 0.3f));

            const CastlingRights cr = game.board().get_castling_rights();
            std::ostringstream castlingStream;
            castlingStream << cr;
            const std::string castlingText = "C: " + castlingStream.str();

            std::string enPassantText;
            if (!game.board().has_en_passant()) {
                enPassantText = "EP: -";
            } else {
                const EnPassant ep = game.board().get_en_passant();
                const char* player = ep.white_vulnerable() ? "White" : "Black";
                enPassantText = std::string("EP: ") + player + " "
                    + square_utils::square_to_string(ep.get_x(), ep.get_y());
            }

            const int infoFont = 18;
            const float lineSpacing = 8.0f;
            const float totalTextHeight = infoFont * 2 + lineSpacing;
            float textY = infoRect.y + (infoRect.height - totalTextHeight) * 0.5f;

            auto draw_centered_text = [&](const std::string& text) {
                const int width = MeasureText(text.c_str(), infoFont);
                const float textX = infoRect.x + (infoRect.width - width) * 0.5f;
                DrawText(text.c_str(), (int)textX, (int)textY, infoFont, WHITE);
                textY += infoFont + lineSpacing;
            };

            draw_centered_text(castlingText);
            draw_centered_text(enPassantText);
        }

        // Chessboard background
        for (int row = 0; row < BOARD_SIZE; ++row) {
            for (int col = 0; col < BOARD_SIZE; ++col) {
                const bool dark = ((row + col) & 1) != 0;
                const float x = boardOffsetX + col * cellSize;
                const float y = boardOffsetY + row * cellSize;
                DrawRectangle((int)x, (int)y, (int)cellSize + 1, (int)cellSize + 1,
                              dark ? DARK_GREEN : LIGHT_GREEN);
            }
        }

        // --- GRID ---
        for (int i = 0; i <= BOARD_SIZE; i++) {
            float lineX = boardOffsetX + i * cellSize;
            float lineY = boardOffsetY + i * cellSize;
            DrawLine((int)lineX, (int)boardOffsetY, (int)lineX, (int)(boardOffsetY + boardPixelSize), Fade(BLACK, 0.15f));
            DrawLine((int)boardOffsetX, (int)lineY, (int)(boardOffsetX + boardPixelSize), (int)lineY, Fade(BLACK, 0.15f));
        }

        // draw coordinate indicators (files at bottom, ranks at left)
        const int coordinateFont = 18;
        const float fileLabelY = boardOffsetY + boardPixelSize + 8.0f;
        const char* files = "abcdefgh";
        for (int file = 0; file < BOARD_SIZE; ++file) {
            const int screenCol = board_col_to_screen_col(file, view_flipped);
            const float centerX = boardOffsetX + screenCol * cellSize + cellSize * 0.5f;
            const std::string label(1, files[file]);
            const int width = MeasureText(label.c_str(), coordinateFont);
            DrawText(label.c_str(),
                     (int)(centerX - width * 0.5f),
                     (int)fileLabelY,
                     coordinateFont,
                     RAYWHITE);
        }
        const float rankLabelX = boardOffsetX - cellSize * 0.35f;
        const char* ranks = "12345678";
        for (int rank = 0; rank < BOARD_SIZE; ++rank) {
            const int screenRow = board_row_to_screen_row(rank, view_flipped);
            const float centerY = boardOffsetY + screenRow * cellSize + cellSize * 0.5f;
            const std::string label(1, ranks[rank]);
            const int width = MeasureText(label.c_str(), coordinateFont);
            const int height = coordinateFont;
            DrawText(label.c_str(),
                     (int)(rankLabelX - width * 0.5f),
                     (int)(centerY - height * 0.5f),
                     coordinateFont,
                     RAYWHITE);
        }

        auto draw_lost_piece_icons = [&](const std::vector<PieceKind>& lostPieces,
                                         bool drawWhite,
                                         bool leftAnchor) {
            if (lostPieces.empty()) {
                return;
            }
            const float iconSize = cellSize * 0.3f;
            const float overlapStep = iconSize * 0.65f;
            const float availableWidth = boardPixelSize * 0.5f;
            const int perRow = std::max(1, (int)std::floor((availableWidth - iconSize) / overlapStep) + 1);
            float x = leftAnchor ? boardOffsetX : (boardOffsetX + boardPixelSize - iconSize);
            const float startX = x;
            float y = boardOffsetY + boardPixelSize + coordinateFont + 16.0f;
            const float maxY = (float)screenHeight - iconSize - 8.0f;
            if (y > maxY) {
                y = maxY;
            }
            for (size_t i = 0; i < lostPieces.size(); ++i) {
                const int idx = piece_kind_index(lostPieces[i]);
                const Texture2D& tex = drawWhite ? piece_textures.white[idx] : piece_textures.black[idx];
                Rectangle src{0.0f, 0.0f, (float)tex.width, (float)tex.height};
                Rectangle dst{x, y, iconSize, iconSize};
                DrawTexturePro(tex, src, dst, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
                if (leftAnchor) {
                    x += overlapStep;
                    if (((i + 1) % perRow) == 0) {
                        x = startX;
                        y += iconSize * 0.9f;
                        if (y > maxY) {
                            y = maxY;
                        }
                    }
                } else {
                    x -= overlapStep;
                    if (((i + 1) % perRow) == 0) {
                        x = boardOffsetX + boardPixelSize - iconSize;
                        y += iconSize * 0.9f;
                        if (y > maxY) {
                            y = maxY;
                        }
                    }
                }
            }
        };

        draw_lost_piece_icons(lost_white_pieces, true, true);
        draw_lost_piece_icons(lost_black_pieces, false, false);

        // draw pieces (handles dragging highlight and visuals)
        draw_pieces(game.board(), cellSize, dragging, selected, dragX, dragY, origX, origY,
            promotion_popup, pending_promotion_white, piece_textures, boardOffsetX, boardOffsetY, view_flipped);

        if (game_over_popup) {
            draw_game_over_popup(cellSize, game_over_status, game_over_winner);
        }

        EndDrawing();
    }

    if (pending_move) {
        delete pending_move;
        pending_move = nullptr;
    }

    unload_piece_textures(piece_textures);
    if (audioReady) {
        UnloadSound(moveSound);
        UnloadSound(checkSound);
        UnloadSound(checkmateSound);
        UnloadSound(stalemateSound);
        UnloadSound(captureSound);
        UnloadSound(bounceSound);
        UnloadSound(undoSound);
        UnloadSound(redoSound);
        UnloadSound(fiftyMoveRuleSound);
        UnloadSound(threefoldRepetitionSound);
        UnloadSound(resetSound);
        UnloadSound(flipSound);
        UnloadSound(mateInOneSound);
    }
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

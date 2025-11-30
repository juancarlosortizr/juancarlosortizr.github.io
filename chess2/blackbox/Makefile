CXX ?= g++
OPTIMIZE ?= -O3
CXXSTD ?= -std=c++17
CXXFLAGS ?= $(CXXSTD) -g $(OPTIMIZE)
INCLUDES = -I/opt/homebrew/include

GUI_LIBS = -L/opt/homebrew/lib -lraylib \
           -framework OpenGL -framework Cocoa \
           -framework IOKit -framework CoreVideo
CMDLINE_LIBS = -L/opt/homebrew/lib

GUI_SOURCES = chess_gui.cpp
CMDLINE_SOURCES = chess_cmdline.cpp

TARGETS = gui cmdline_chess

all: $(TARGETS)

gui: $(GUI_SOURCES) board.h castling.h en_passant.h piece.h move.h game.h lawyer.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GUI_SOURCES) -o $@ $(GUI_LIBS)

cmdline_chess: $(CMDLINE_SOURCES) board.h castling.h en_passant.h piece.h move.h game.h lawyer.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(CMDLINE_SOURCES) -o $@ $(CMDLINE_LIBS)

.PHONY: clean
clean:
	$(RM) $(TARGETS)

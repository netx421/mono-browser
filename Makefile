# ---------------------------------------------
# COLOSSUS Network Access Node Makefile
# ---------------------------------------------

CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -g

PKG_GTK := gtk+-3.0

# Try webkit2gtk-4.1 (Arch), fall back to 4.0 (Ubuntu/Mint)
WEBKIT_PKG := $(shell pkg-config --exists webkit2gtk-4.1 && echo webkit2gtk-4.1 || echo webkit2gtk-4.0)

PKG     := $(PKG_GTK) $(WEBKIT_PKG)

INCLUDES := $(shell pkg-config --cflags $(PKG))
LIBS     := $(shell pkg-config --libs $(PKG))

TARGET   := COLOSSUS-NAN
SRC      := main.cpp browser.cpp
OBJ      := $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) $(LIBS) -o $(TARGET)

%.o: %.cpp browser.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: all
	./$(TARGET)

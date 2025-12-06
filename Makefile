ifeq (,$(shell which ccache 2>/dev/null))
	CXX = clang++
else
	CXX = ccache clang++
endif

EXE = cleo
SRC_DIR = ./src
OBJ_DIR = ./obj
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
CXXSTD = -std=c++23
CXXFLAGS = $(CXXSTD)
CXXFLAGS += -Wall -Wextra -Wpedantic -Wformat -Weffc++ -Wconversion -Wunused-function
LDFLAGS = `pkg-config --libs sfml-audio readline`
MAKEFLAGS += --no-builtin-rules
DEBUG = 0
ifeq ($(DEBUG), 1)
	CXXFLAGS += -ggdb -DNDEBUG -fsanitize=address
else
	CXXFLAGS += -UNDEBUG -O2
endif

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(SRC_DIR)/%.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE) $(OBJ_DIR)
	@echo All sources built successfully

$(OBJ_DIR):
	mkdir -p ./obj

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

clean:
	$(RM) $(EXE) $(OBJ_DIR)/*

format:
	clang-format $(SOURCES) -i

.PHONY: clean format
.SUFFIXES:

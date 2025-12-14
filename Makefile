ifeq (,$(shell which ccache 2>/dev/null))
	CXX = clang++
else
	CXX = ccache clang++
endif

EXE = cleo
DBG_EXE = cleo-dbg
SRC_DIR = ./src
OBJ_DIR = ./obj
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
DBG_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%-dbg.o, $(SOURCES))
CXXSTD = -std=c++23
CXXFLAGS = $(CXXSTD)
CXXFLAGS += -Wall -Wextra -Wpedantic -Wformat -Weffc++ -Wconversion -Wunused-function
DBGFLAGS = -ggdb -UNDEBUG -fsanitize=address
RELFLAGS = -DNDEBUG -O1
# Change to O1 is necessary because a bug arises concerning the input thread if O2 or O3 is used
LDFLAGS = `pkg-config --libs sfml-audio readline`
MAKEFLAGS += --no-builtin-rules

$(OBJ_DIR)/%-dbg.o: $(SRC_DIR)/%.cpp $(SRC_DIR)/%.hpp
	$(CXX) $(CXXFLAGS) $(DBGFLAGS) -c -o $@ $<

$(OBJ_DIR)/%-dbg.o: $(SRC_DIR)/%.cpp 
	$(CXX) $(CXXFLAGS) $(DBGFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(SRC_DIR)/%.hpp
	$(CXX) $(CXXFLAGS) $(RELFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(RELFLAGS) -c -o $@ $<

all: $(EXE) $(OBJ_DIR)
	@echo Finished release build

debug: $(DBG_EXE) $(OBJ_DIR)
	@echo Finished debug build

$(OBJ_DIR):
	mkdir -p ./obj

$(DBG_EXE): $(DBG_OBJS)
	$(CXX) -o $@ $^ $(DBGFLAGS) $(LDFLAGS)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(RELFLAGS) $(LDFLAGS)

clean:
	$(RM) $(EXE) $(DBG_EXE) $(OBJ_DIR)/*

format:
	clang-format $(SOURCES) -i

.PHONY: clean format
.SUFFIXES:

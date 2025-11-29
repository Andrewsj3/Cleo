CXX = clang++

EXE = cleo
SRC_DIR = ./src
OBJ_DIR = ./obj
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
# OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
CXXSTD = -std=c++23
CXXFLAGS = $(CXXSTD)
CXXFLAGS += -Wall -Wextra -Wpedantic -Wformat -Weffc++ -Wconversion -Wunused-function
LDFLAGS = `pkg-config --libs sfml-audio readline`
MAKEFLAGS += --no-builtin-rules --no-builtin-variables
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
	rm -f $(EXE) $(OBJS)

format:
	clang-format $(SOURCES) -i

.PHONY: clean format
.SUFFIXES:

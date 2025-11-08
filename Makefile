CXX = clang++

EXE = cleo
SRC_DIR = ./src
OBJ_DIR = ./obj
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
# OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
CXXSTD = -std=c++23
CXXFLAGS = $(CXXSTD)
CXXFLAGS += -Wall -Wextra -Wpedantic -Wformat -Wextra -Weffc++ -Wconversion
LDFLAGS = `pkg-config --libs sfml-audio readline`
MAKEFLAGS += --no-builtin-rules --no-builtin-variables
DEBUG = 0
ifeq ($(DEBUG), 1)
	CXXFLAGS += -ggdb -UNDEBUG
else
	CXXFLAGS += -DNDEBUG
endif

$(OBJ_DIR)/%.o:$(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE) $(OBJ_DIR)
	@echo All sources built successfully

$(OBJ_DIR):
	mkdir -p ./obj

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f $(EXE) $(OBJS)

tidy:
	clang-tidy $(SOURCES)

.PHONY: clean tidy
.SUFFIXES:

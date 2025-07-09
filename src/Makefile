CXX = clang++

EXE = ../cleo
SOURCES = $(wildcard ./*.cpp)
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
CXXSTD = -std=c++23
CXXFLAGS = $(CXXSTD)
CXXFLAGS += -Wall -Wextra -Wpedantic -Wformat -Wextra -Weffc++ -Wconversion
LDFLAGS = `pkg-config --libs sfml-audio readline`
MAKEFLAGS += --no-builtin-rules --no-builtin-variables -j6
DEBUG = 0
ifeq ($(DEBUG), 1)
	CXXFLAGS += -ggdb -UNDEBUG
else
	CXXFLAGS += -DNDEBUG
endif

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo All sources built successfully

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f $(EXE) $(OBJS)

tidy:
	clang-tidy $(SOURCES)

.PHONY: clean tidy
.SUFFIXES:

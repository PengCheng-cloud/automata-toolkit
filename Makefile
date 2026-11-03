CXX      ?= clang++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic
CPPFLAGS += -Iinclude

LIB_SRC  := src/dfa.cpp src/nfa.cpp src/algorithms.cpp src/opacity.cpp src/timed.cpp
TEST_SRC := $(wildcard tests/*.cpp)
HEADERS  := $(wildcard include/automata/*.hpp) tests/check.hpp

.PHONY: all test clean

all: build/automata

build:
	mkdir -p build

build/automata: src/main.cpp $(LIB_SRC) $(HEADERS) | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/main.cpp $(LIB_SRC) -o $@

build/tests: $(TEST_SRC) $(LIB_SRC) $(HEADERS) | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TEST_SRC) $(LIB_SRC) -o $@

test: build/tests
	./build/tests

clean:
	rm -rf build

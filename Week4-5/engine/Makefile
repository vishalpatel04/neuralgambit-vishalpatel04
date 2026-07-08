CXX = clang++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Iinclude

all: bin/neuralgambit

bin:
	mkdir -p bin

bin/neuralgambit: src/main.cpp include/search.hpp include/evaluation.hpp include/transposition_table.hpp include/uci_utils.hpp include/chess.hpp | bin
	$(CXX) $(CXXFLAGS) src/main.cpp -o bin/neuralgambit

clean:
	rm -rf bin

.PHONY: all clean

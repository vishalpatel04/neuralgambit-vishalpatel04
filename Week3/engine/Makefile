CXX = clang++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Iinclude

all: bin/mate_solver bin/puzzle_runner

bin:
	mkdir -p bin

bin/mate_solver: src/main.cpp include/mate_solver.hpp include/chess.hpp | bin
	$(CXX) $(CXXFLAGS) src/main.cpp -o bin/mate_solver

bin/puzzle_runner: src/puzzle_runner.cpp include/mate_solver.hpp include/tiny_json.hpp include/chess.hpp | bin
	$(CXX) $(CXXFLAGS) src/puzzle_runner.cpp -o bin/puzzle_runner

clean:
	rm -rf bin

.PHONY: all clean

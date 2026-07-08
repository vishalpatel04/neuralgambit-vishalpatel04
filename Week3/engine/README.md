# Mate-in-N Solver

An OOP `MateSolver` class (`include/mate_solver.hpp`) that finds forced
checkmate sequences using the [Disservin chess-library](https://github.com/Disservin/chess-library)
(vendored at `include/chess.hpp`).

## Approach

`MateSolver::findMate(n)` performs an exhaustive game-tree search: a move for
the side to move only counts toward a forced mate if either it delivers
immediate checkmate, or *every* legal defensive reply still allows the
attacker to force mate within the remaining moves. Captures are tried first
as a move-ordering heuristic to find the winning line faster.

## Build

```sh
make
```

Produces `bin/mate_solver` and `bin/puzzle_runner`.

## Usage

```sh
./bin/mate_solver "<FEN>" <N>
# e.g.
./bin/mate_solver "4r1rk/5K1b/7R/R7/8/8/8/8 w - - 0 1" 2
# Mate in 2: h6h7 h8h7 a5h5
```

Moves are printed in UCI notation (matching the engine's UCI protocol).

## Verifying against the puzzle sets

```sh
./bin/puzzle_runner ../mate_in_2.json 2
./bin/puzzle_runner ../mate_in_3.json 3
./bin/puzzle_runner ../mate_in_4.json 4
```

Results:

| Puzzle set | Solved | Time |
|---|---|---|
| mate-in-2 (351 puzzles, 3-ply) | 351/351 | 0.04s |
| mate-in-3 (489 puzzles, 5-ply) | 489/489 | 2.3s |
| mate-in-4 (462 puzzles, 7-ply) | 455/462 (7 timed out) | 183s |

The 7 mate-in-4 timeouts (`findMateBounded`, 5s/puzzle cap) are positions
where the unpruned exhaustive search's branching factor is too large to
finish in time — a natural next step would be alpha-beta-style pruning or a
transposition table, rather than a correctness issue (every solved puzzle's
line was checked to actually end in checkmate).

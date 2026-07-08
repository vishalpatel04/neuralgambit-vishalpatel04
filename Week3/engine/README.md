# Mate-in-N Solver

An OOP `MateSolver` class (`include/mate_solver.hpp`) that finds forced
checkmate sequences using the [Disservin chess-library](https://github.com/Disservin/chess-library)
(vendored at `include/chess.hpp`).

## Approach

`MateSolver::findMate(n)` performs an exhaustive game-tree search: a move for
the side to move only counts toward a forced mate if either it delivers
immediate checkmate, or *every* legal defensive reply still allows the
attacker to force mate within the remaining moves. Captures are tried first
(for both the attacker's candidate moves and the defender's replies) as a
move-ordering heuristic to find the winning line, or the losing reply, faster.

A transposition table (`provenNoMateWithin_`, keyed by the position's Zobrist
hash) caches "no forced mate within N moves from this exact position"
results. The same position is frequently reached by different move orders —
especially once a puzzle's mating line includes a non-check "quiet" move —
so this prunes a large fraction of the redundant re-search that otherwise
makes deep (mate-in-4) searches slow.

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

To see the board itself change move by move, use `./bin/demo "<FEN>" <N>`
(defaults to the mate-in-2 example above if no arguments are given).

## Verifying against the puzzle sets

```sh
./bin/puzzle_runner ../mate_in_2.json 2
./bin/puzzle_runner ../mate_in_3.json 3
./bin/puzzle_runner ../mate_in_4.json 4
```

Results (with the transposition table, `findMateBounded`'s cap at 20s/puzzle):

| Puzzle set | Solved | Time |
|---|---|---|
| mate-in-2 (351 puzzles, 3-ply) | 351/351 | 0.05s |
| mate-in-3 (489 puzzles, 5-ply) | 489/489 | 1.9s |
| mate-in-4 (462 puzzles, 7-ply) | 462/462 | 95s |

Before the transposition table, mate-in-4 solved 455/462 within a 5s/puzzle
cap (7 timed out on positions whose branching factor was too large for the
unpruned search). Caching "no forced mate within N moves" per Zobrist hash
eliminated the redundant re-search caused by transposing move orders, and
all 7 stragglers now solve individually in under 14s each — full 462/462.

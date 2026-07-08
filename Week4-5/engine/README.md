# NeuralGambit UCI Engine

A from-scratch chess engine speaking the [UCI protocol](https://backscattering.de/chess/uci/), built on the
[Disservin chess-library](https://github.com/Disservin/chess-library) (vendored at `include/chess.hpp`, same
copy used by the Week3 mate solver).

## Architecture

- **`include/evaluation.hpp`** — static evaluation: material + piece-square tables (Tomasz Michniewski's
  simplified values), returned relative to the side to move.
- **`include/transposition_table.hpp`** — fixed-size hash table keyed by Zobrist hash (`board.hash()`),
  depth-preferred replacement. Same idea as the Week3 mate solver's transposition table, generalized to store
  a score, bound type (exact/lower/upper), and best move instead of just "no mate here."
- **`include/search.hpp`** — `Searcher`: negamax with alpha-beta pruning, using the transposition table for
  cutoffs and move ordering (TT move first, then captures), plus **quiescence search** at leaf nodes (extends
  through captures — and all evasions if in check — so the static eval is never taken mid-exchange).
  `searchWithTimeLimit()` drives **iterative deepening**: depth 1, 2, 3, ... each refining the last, stopping
  as soon as the time budget runs out and returning the last *fully completed* depth's move (a partially
  searched depth is discarded, since it can be misleadingly bad).
- **`include/uci_utils.hpp`** — move ↔ UCI string conversion (`e2e4`, `e7e8q`, ...).
- **`src/main.cpp`** — the UCI protocol loop itself (`uci`, `isready`, `position`, `go`, `stop`, `quit`).

## Build

```sh
make
```

Produces `bin/neuralgambit`.

## Using it

Any UCI-speaking GUI (CuteChess, Arena, ...) can drive it directly — point the GUI at `bin/neuralgambit` as
an engine. It also works from a raw terminal by typing UCI commands directly:

```sh
./bin/neuralgambit
uci
isready
position startpos
go movetime 1000
```

Time control support in `go`: `movetime <ms>` (search exactly this long), `wtime/btime/winc/binc [movestogo]`
(computes a per-move budget from remaining clock time), `depth <n>` (fixed-depth, not time-limited), or
`infinite`.

## Known limitation

`stop` isn't a true async interrupt — this is a single-threaded engine, so `go` blocks until it returns
`bestmove`, and any `stop` sent mid-search is only read *after* the search that's already running completes.
In practice this is fine since the engine self-limits to its computed time budget; it just means `stop` can't
cut a search short mid-flight. A future improvement would run the search on a separate thread and poll stdin
concurrently.

## Verification

Hand-tested via raw UCI commands (no GUI available in this environment) across: the `uci`/`isready` handshake,
`position startpos`/`position fen ... moves ...` parsing, `go movetime`/`go depth`, mate-in-1 detection (scored
correctly as a mate score, not just a high centipawn value), and — more subtly — two endgame positions where a
naive material-only engine would misjudge the result: capturing into a King+Knight vs King draw (insufficient
material) and a King+Pawn vs King position where the defending king actually reaches the pawn in time. In both
cases the engine's score correctly converged to the true drawn result (0) as search depth increased, rather
than staying fooled by the material count — a sign the search, quiescence, and insufficient-material handling
are working together correctly rather than just "material go up = good."

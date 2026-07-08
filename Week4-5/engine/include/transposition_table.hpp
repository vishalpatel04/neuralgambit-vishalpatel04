#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "chess.hpp"

namespace engine {

enum class Bound : std::uint8_t { EXACT, LOWER, UPPER };

struct TTEntry {
    std::uint64_t key = 0;
    int depth = -1;
    int score = 0;
    Bound bound = Bound::EXACT;
    chess::Move bestMove = chess::Move::NO_MOVE;
};

// Fixed-size hash table keyed by Zobrist hash (board.hash()), indexed via
// modulo. Depth-preferred replacement: a slot is only overwritten if the new
// entry searched at least as deep as whatever's already there, so shallow
// re-searches don't evict a deeper, more valuable result.
class TranspositionTable {
   public:
    explicit TranspositionTable(std::size_t sizeMb = 64) {
        std::size_t numEntries = std::max<std::size_t>(1, (sizeMb * 1024 * 1024) / sizeof(TTEntry));
        table_.resize(numEntries);
    }

    void clear() { std::fill(table_.begin(), table_.end(), TTEntry{}); }

    const TTEntry* probe(std::uint64_t key) const {
        const TTEntry& entry = table_[key % table_.size()];
        if (entry.depth >= 0 && entry.key == key) return &entry;
        return nullptr;
    }

    void store(std::uint64_t key, int depth, int score, Bound bound, chess::Move bestMove) {
        TTEntry& slot = table_[key % table_.size()];
        if (slot.key != key || depth >= slot.depth) {
            slot = TTEntry{key, depth, score, bound, bestMove};
        }
    }

   private:
    std::vector<TTEntry> table_;
};

}  // namespace engine

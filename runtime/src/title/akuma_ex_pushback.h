#pragma once
// Bounded, reversible EX Hadoken second-hit pushback pointers (Phase 80E09).
// Source and target are existing native reaction rows from the user's own
// generated moveset. This header contains no assets, addresses or dumps.
#include <array>
#include <cstdint>

namespace cafe::title::akuma_ex_pushback {
enum class Restore { nothing, restored, conflict };
class Overlay {
public:
    bool active() const { return row_ != 0; }
    uint32_t row() const { return row_; }

    template<class Read32, class Read16, class Write32>
    bool apply(uint32_t reaction, Read32 read32, Read16 read16, Write32 write32) {
        if (active() || reaction < 0x30001000u || reaction > 0x3fffffa0u)
            return false;
        constexpr uint32_t gap = 5u * 0x50u;
        const uint32_t source = reaction - gap;
        if (read16(source + 0x34u) != 2516u ||
            read16(source + 0x3cu) != 2517u ||
            read16(source + 0x40u) != 2518u ||
            read16(source + 0x44u) != 2519u)
            return false;
        const auto ptr = [](uint32_t p) { return p >= 0x30000000u && p < 0x3fffffe0u; };
        const uint32_t original = read32(reaction);
        const uint32_t front = read32(source);
        const uint32_t downed = read32(source + 0x14u);
        const uint32_t blocked = read32(source + 0x18u);
        if (!ptr(original) || !ptr(front) || !ptr(downed) || !ptr(blocked) ||
            read16(original) != 0u || read16(original + 2u) != 0u ||
            read16(front) != 48u || read16(front + 2u) != 30u ||
            read16(downed) != 32u || read16(downed + 2u) != 50u ||
            read32(downed + 4u) != 8u ||
            read16(blocked) != 0u || read16(blocked + 2u) != 0u)
            return false;
        for (unsigned i = 0; i != 7; ++i) {
            before_[i] = read32(reaction + 4u*i);
            replacement_[i] = read32(source + 4u*i);
            if (!ptr(before_[i]) || !ptr(replacement_[i])) return false;
        }
        row_ = reaction; // acquire lease before writes to permit partial rollback
        for (unsigned i = 0; i != 7; ++i)
            write32(reaction + 4u*i, replacement_[i]);
        for (unsigned i = 0; i != 7; ++i) {
            if (read32(reaction + 4u*i) != replacement_[i]) {
                restore(read32, write32);
                return false;
            }
        }
        return true;
    }

    template<class Read32, class Write32>
    Restore restore(Read32 read32, Write32 write32) {
        if (!active()) return Restore::nothing;
        // Unknown writes cause a conflict; never restore into a reused row.
        for (unsigned i = 0; i != 7; ++i) {
            const uint32_t now = read32(row_ + 4u*i);
            if (now != replacement_[i] && now != before_[i]) return Restore::conflict;
        }
        for (unsigned i = 0; i != 7; ++i)
            if (read32(row_ + 4u*i) == replacement_[i])
                write32(row_ + 4u*i, before_[i]);
        for (unsigned i = 0; i != 7; ++i)
            if (read32(row_ + 4u*i) != before_[i]) return Restore::conflict;
        row_ = 0;
        return Restore::restored;
    }
private:
    uint32_t row_ = 0;
    std::array<uint32_t,7> before_{};
    std::array<uint32_t,7> replacement_{};
};
} // namespace cafe::title::akuma_ex_pushback

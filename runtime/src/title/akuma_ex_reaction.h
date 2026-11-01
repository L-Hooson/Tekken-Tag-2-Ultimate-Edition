#pragma once
// EX Hadoken first-stagger -> second-hit knockdown overlay from Phase 80E.
// Operates on an injected memory accessor, not on global guest memory.
// Callers must validate the T7-derived reaction identity and fighter lifetime.
#include <array>
#include <cstdint>

namespace cafe::title::akuma_ex_reaction {
inline constexpr std::array<uint32_t, 4> offsets{0x34, 0x3c, 0x40, 0x44};
inline constexpr std::array<uint16_t, 4> stagger{2504, 2505, 2506, 2507};
inline constexpr std::array<uint16_t, 4> knockdown{2532, 2533, 2534, 2535};

enum class Restore { nothing, restored, conflict };

class Overlay {
public:
    bool active() const { return row_ != 0; }
    uint32_t row() const { return row_; }

    // The caller supplies read16(address) and write16(address, value).
    // No write is attempted unless *all four* original reactions match.
    template <class Read, class Write>
    bool apply(uint32_t reaction, Read read16, Write write16) {
        if (active() || reaction < 0x30000000u || reaction > 0x3fffffb0u)
            return false;
        for (unsigned i = 0; i < offsets.size(); ++i)
            if (read16(reaction + offsets[i]) != stagger[i]) return false;
        row_ = reaction; // record before the first write so failures can unwind
        for (unsigned i = 0; i < offsets.size(); ++i)
            write16(reaction + offsets[i], knockdown[i]);
        for (unsigned i = 0; i < offsets.size(); ++i)
            if (read16(reaction + offsets[i]) != knockdown[i]) {
                restore(read16, write16); // best effort; retain conflict if altered
                return false;
            }
        return true;
    }

    // Restore only if the *entire* overlay is still ours. An unknown writer
    // is never overwritten; the unresolved lease stays marked active.
    template <class Read, class Write>
    Restore restore(Read read16, Write write16) {
        if (!active()) return Restore::nothing;
        for (unsigned i = 0; i < offsets.size(); ++i) {
            const auto now = read16(row_ + offsets[i]);
            if (now != knockdown[i] && now != stagger[i]) return Restore::conflict;
        }
        for (unsigned i = 0; i < offsets.size(); ++i)
            if (read16(row_ + offsets[i]) == knockdown[i])
                write16(row_ + offsets[i], stagger[i]);
        for (unsigned i = 0; i < offsets.size(); ++i)
            if (read16(row_ + offsets[i]) != stagger[i]) return Restore::conflict;
        row_ = 0;
        return Restore::restored;
    }

private:
    uint32_t row_ = 0;
};
} // namespace cafe::title::akuma_ex_reaction

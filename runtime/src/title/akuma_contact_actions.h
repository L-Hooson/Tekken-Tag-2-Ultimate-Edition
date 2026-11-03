#pragma once
#include <array>
#include <cstdint>

namespace cafe::title::akuma_contact {
// Private imported requirement, emitted only by Akuma's special-buffer latch.
inline constexpr uint32_t special_contact = 0x7000;
constexpr bool hit_or_block(uint32_t victim_events) {
    return (victim_events & (0x00800000u | 0x20000000u)) != 0;
}

constexpr bool latch_contact(uint32_t victim_events, uint32_t before_move,
                             uint32_t move, uint32_t frame, uint32_t start, uint32_t end) {
    return before_move && before_move == move && start <= frame && frame <= end &&
           hit_or_block(victim_events);
}

// Native requirement 46 (04053E7C) first tests the attacker's +1152
// contact bit, then the victim's +1148 damage record. The victim's event bits
// used by the HUD are not the same phase of the cancel interpreter. Omitting
// native 46's "not blocked" clause permits both hit and block, not whiff.
constexpr bool cancel_contact(uint32_t attacker_events, uint32_t victim_damage) {
    return (attacker_events & 0x00200000u) != 0 && victim_damage != 0;
}

// A frozen animation can revisit its requirement actions. Do not restart the
// same stop indefinitely. New moves, loops and fighter generations re-arm it.
class Actions {
public:
    void reset() { *this = {}; }
    void begin(uint64_t generation, uint32_t move, uint32_t frame) {
        if (generation != generation_ || move != move_ || frame < frame_) count_ = 0;
        generation_ = generation;
        move_ = move;
        frame_ = frame;
    }
    bool accept(uint32_t row) {
        if (!generation_ || !move_ || !row) return false;
        for (unsigned i = 0; i < count_; ++i) if (rows_[i] == row) return false;
        if (count_ == rows_.size()) return false;
        rows_[count_++] = row;
        return true;
    }
private:
    uint64_t generation_ = 0;
    uint32_t move_ = 0, frame_ = 0;
    unsigned count_ = 0;
    std::array<uint32_t, 128> rows_{};
};
}

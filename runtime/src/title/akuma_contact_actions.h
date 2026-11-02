#pragma once
#include <array>
#include <cstdint>

namespace cafe::title::akuma_contact {
// Private imported requirement, emitted only by Akuma's special-buffer latch.
inline constexpr uint32_t special_contact = 0x7000;
constexpr bool hit_or_block(uint32_t victim_events) {
    return (victim_events & (0x00800000u | 0x20000000u)) != 0;
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

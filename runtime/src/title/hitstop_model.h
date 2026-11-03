#pragma once
// Per-fighter 60 Hz battle-tick hitstop scheduler for imported Tekken 7 moves.
// This header models the frame timing only. Native input collection, collision
// dispatch, and the decision to opt into hitstop remain in the runtime adapter.
// No game assets or precompiled gameplay code are included here.

#include <array>
#include <cstdint>
#include <stdexcept>

namespace cafe::title::hitstop {

struct Timing {
    unsigned delay = 0;
    unsigned duration = 0;
};

// T7 0x8292/0x8293 event: upper byte is the delay; next byte is frames.
// The following byte belongs to another field and is not hitstop duration.
constexpr Timing decode(uint32_t raw) {
    return {raw >> 24, (raw >> 16) & 0xff};
}

class Model {
public:
    static constexpr unsigned actors = 4;
    static constexpr unsigned horizon = 512;

    void reset() { *this = Model{}; }

    // A character swap must not inherit a previous fighter's frozen state.
    void bind(unsigned slot, uint64_t generation) {
        check(slot);
        if (generation_[slot] == generation) return;
        generation_[slot] = generation;
        stopped_[slot].fill(false);
    }

    bool frozen(unsigned slot) const {
        check(slot);
        return stopped_[slot][cursor_];
    }

    // An action replaces future freeze ticks for this fighter, rather than
    // stacking extra frames. It never retroactively freezes the current tick.
    void set(unsigned slot, Timing timing) {
        check(slot);
        if (timing.delay >= horizon || timing.duration >= horizon - timing.delay)
            throw std::invalid_argument("hitstop interval exceeds scheduler horizon");
        const bool current = stopped_[slot][cursor_];
        stopped_[slot].fill(false);
        stopped_[slot][cursor_] = current;
        for (unsigned n = 0; n < timing.duration; ++n)
            stopped_[slot][(cursor_ + 1 + timing.delay + n) % horizon] = true;
    }

    // Guarantee a short immediate contact pause without deleting a longer
    // source action or adding time after that action ends.
    void ensure(unsigned slot, unsigned duration) {
        check(slot);
        if (duration >= horizon) throw std::invalid_argument("hitstop interval exceeds scheduler horizon");
        for (unsigned n = 0; n < duration; ++n)
            stopped_[slot][(cursor_ + 1 + n) % horizon] = true;
    }

    // The adapter runs this once per battle simulation tick, for both actors.
    void advance() {
        for (auto& slot : stopped_) slot[cursor_] = false;
        cursor_ = (cursor_ + 1) % horizon;
    }

private:
    static void check(unsigned slot) {
        if (slot >= actors) throw std::out_of_range("hitstop fighter slot");
    }

    unsigned cursor_ = 0;
    std::array<uint64_t, actors> generation_{};
    std::array<std::array<bool, horizon>, actors> stopped_{};
};

} // namespace cafe::title::hitstop

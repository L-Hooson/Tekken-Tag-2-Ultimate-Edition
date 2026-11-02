#pragma once
// Pure multi-contact timing rules verified in the Akuma Phase 81D experiment.
// No guest memory access, projectile table mutation or character pack data here.
// The adapter must validate native projectile layout, restore temporary native
// gates on scene/slot changes, and call accept() *after* native damage dispatch.
#include <array>
#include <cstdint>

namespace cafe::title::akuma_projectile {

enum class Kind : uint8_t { ex_hadoken, shinku_hadoken };
struct Policy {
    unsigned hits, interval;
    std::array<unsigned, 5> freeze;
};
constexpr Policy policy(Kind k) {
    return k == Kind::ex_hadoken ? Policy{2, 9, {8, 5, 0, 0, 0}}
                                 : Policy{5, 7, {6, 6, 6, 6, 5}};
}
struct Contact {
    uint64_t tick;
    uintptr_t owner, victim, slot, row;
    unsigned remaining_after_native;
};
struct Decision {
    bool accepted = false;
    bool gate_next = false;
    bool finished = false;
    unsigned hit = 0, freeze = 0;
};

class Sequence {
public:
    explicit Sequence(Kind kind) : policy_(policy(kind)) {}
    void reset() { state_ = {}; }
    bool active() const { return state_.active; }
    unsigned accepted() const { return state_.accepted; }
    bool gate_active() const { return state_.gate; }
    uint64_t release_at() const { return state_.last_tick + policy_.interval; }
    // Release the native gate only when the entire previous stop interval ends.
    bool release_due(uint64_t tick) {
        if (!state_.active || !state_.gate || tick < release_at()) return false;
        state_.gate = false;
        return true;
    }
    Decision accept(Contact c) {
        if (!c.owner || !c.victim || !c.slot || !c.row || c.owner == c.victim)
            return {};
        // A completed sequence must allow the next projectile's first hit.
        // Same physical slot and row may be reused by native TTT2. Only
        // re-arm on a fresh native full-capacity first-hit counter, beyond
        // the completed sequence's contact window (never on a duplicate).
        if (state_.active && state_.accepted == policy_.hits && !state_.gate &&
            c.tick > state_.last_tick &&
            c.tick - state_.last_tick >= policy_.interval &&
            c.remaining_after_native == policy_.hits - 1)
            reset();
        if (!state_.active) {
            if (c.remaining_after_native != policy_.hits - 1) return {};
            state_.active = true;
            state_.owner = c.owner;
            state_.victim = c.victim;
            state_.slot = c.slot;
            state_.row = c.row;
        } else if (c.owner != state_.owner || c.victim != state_.victim ||
                   c.slot != state_.slot || c.row != state_.row) {
            // Never inherit state across fighter swaps or recycled slots.
            reset();
            return {};
        }
        if (state_.accepted >= policy_.hits || state_.gate ||
            (state_.accepted && (c.tick <= state_.last_tick ||
                                 c.tick - state_.last_tick < policy_.interval)))
            return {};
        const unsigned expected = policy_.hits - state_.accepted - 1;
        if (c.remaining_after_native != expected) {
            reset(); // native counter mismatch: unsafe to continue gating
            return {};
        }
        ++state_.accepted;
        state_.last_tick = c.tick;
        const bool finished = state_.accepted == policy_.hits;
        state_.gate = !finished;
        return {true, !finished, finished, state_.accepted,
                policy_.freeze[state_.accepted - 1]};
    }
private:
    Policy policy_;
    struct State {
        uintptr_t owner = 0, victim = 0, slot = 0, row = 0;
        uint64_t last_tick = 0;
        unsigned accepted = 0;
        bool active = false, gate = false;
    } state_;
};
} // namespace cafe::title::akuma_projectile

#pragma once
// A temporary per-projectile row. Shared moveset data is never modified.
// The caller owns a 136-byte scratch row until release, and releases before
// the native fighter/projectile storage is freed at a situation transition.
#include <cstdint>

namespace cafe::title::akuma_projectile {
class ReversibleGate {
public:
    bool active() const { return active_; }
    uint32_t slot() const { return slot_; }
    uint32_t row() const { return scratch_; }

    template<class Read, class Write>
    bool arm(uint32_t slot, uint32_t row, uint32_t scratch,
             uint32_t expected_speed, uint32_t expected_enemy, Read read, Write write) {
        if (active_ || !slot || !row || !scratch || scratch == row ||
            !expected_speed || !expected_enemy || read(slot + 60) != row ||
            read(slot + 32) == 0xffffffffu || read(row + 8) != expected_speed ||
            read(row + 88) != expected_enemy) return false;
        for (unsigned i = 0; i != 34; ++i) write(scratch + 4*i, read(row + 4*i));
        write(scratch + 8, 0);
        write(scratch + 88, 0);
        for (unsigned i = 0; i != 34; ++i) {
            const uint32_t expected = i == 2 || i == 22 ? 0 : read(row + 4*i);
            if (read(scratch + 4*i) != expected) return false;
        }
        write(slot + 60, scratch);
        if (read(slot + 60) != scratch) return false;
        slot_ = slot;
        original_ = row;
        scratch_ = scratch;
        for (unsigned i = 0; i != 3; ++i) xyz_[i] = read(slot + 4*i);
        active_ = true;
        return true;
    }

    template<class Read, class Write>
    bool hold(Read read, Write write) const {
        if (!active_ || read(slot_ + 60) != scratch_ ||
            read(slot_ + 32) == 0xffffffffu) return false;
        for (unsigned i = 0; i != 3; ++i) write(slot_ + i*4, xyz_[i]);
        return true;
    }

    template<class Read, class Write>
    bool release(Read read, Write write) {
        if (!active_) return false;
        // A retired slot can still point at our copy. A reused slot belongs
        // to another shot and must not be overwritten. Neither case requires
        // writing the shared row, even if another system has changed it.
        if (read(slot_ + 60) == scratch_) {
            write(slot_ + 60, original_);
            if (read(slot_ + 60) != original_) return false;
        }
        slot_ = original_ = scratch_ = 0;
        active_ = false;
        return true;
    }
private:
    uint32_t slot_ = 0, original_ = 0, scratch_ = 0;
    uint32_t xyz_[3]{};
    bool active_ = false;
};
} // namespace cafe::title::akuma_projectile

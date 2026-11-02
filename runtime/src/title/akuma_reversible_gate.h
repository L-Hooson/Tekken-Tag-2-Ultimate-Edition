#pragma once
// Reversible native-projectile gate. A safety boundary, not an always-on patch.
// The caller checks Akuma ownership, attack identity and native hit acceptance.
// All offsets are relative to a verified TTT2 projectile slot/row; the caller
// owns the memory accessors. Never touch a row whose state changed underneath us.
#include <cstdint>

namespace cafe::title::akuma_projectile {
class ReversibleGate {
public:
    bool active() const { return active_; }
    uint32_t slot() const { return slot_; }
    uint32_t row() const { return row_; }

    template<class Read, class Write>
    bool arm(uint32_t slot, uint32_t row, uint32_t expected_speed,
             uint32_t expected_enemy, Read read, Write write) {
        if (active_ || !slot || !row || !expected_speed || !expected_enemy ||
            read(slot + 60) != row || read(slot + 32) == 0xffffffffu ||
            read(row + 8) != expected_speed || read(row + 88) != expected_enemy)
            return false;
        uint32_t xyz[3]{};
        for (unsigned i = 0; i != 3; ++i) xyz[i] = read(slot + 4 * i);
        // Set the native targeting gate first; freeze the projectile's speed
        // only after the targeting flag has been checked.
        write(row + 88, 0);
        if (read(row + 88) != 0) return false;
        write(row + 8, 0);
        if (read(row + 8) != 0) {
            // A failed write must not leave a half-gated projectile.
            if (read(row + 88) == 0) write(row + 88, expected_enemy);
            return false;
        }
        slot_ = slot;
        row_ = row;
        speed_ = expected_speed;
        enemy_ = expected_enemy;
        for (unsigned i = 0; i != 3; ++i) xyz_[i] = xyz[i];
        active_ = true;
        return true;
    }

    template<class Read, class Write>
    bool hold(Read read, Write write) const {
        if (!valid(read)) return false;
        for (unsigned i = 0; i != 3; ++i) write(slot_ + i * 4, xyz_[i]);
        return true;
    }

    // Called on release tick, slot loss, fighter swap and shutdown.
    // Never overwrite a row if another system has already changed either
    // field. Clear our own state even if safe restoration was impossible.
    template<class Read, class Write>
    bool release(Read read, Write write) {
        if (!active_) return false;
        const bool can_restore = valid(read);
        if (can_restore) {
            write(row_ + 8, speed_);
            write(row_ + 88, enemy_);
        }
        clear();
        return can_restore;
    }

private:
    template<class Read>
    bool valid(Read read) const {
        return active_ && read(slot_ + 60) == row_ &&
               read(slot_ + 32) != 0xffffffffu &&
               read(row_ + 8) == 0 && read(row_ + 88) == 0;
    }
    void clear() { slot_ = row_ = speed_ = enemy_ = 0; active_ = false; }
    uint32_t slot_ = 0, row_ = 0, speed_ = 0, enemy_ = 0;
    uint32_t xyz_[3]{};
    bool active_ = false;
};
} // namespace cafe::title::akuma_projectile

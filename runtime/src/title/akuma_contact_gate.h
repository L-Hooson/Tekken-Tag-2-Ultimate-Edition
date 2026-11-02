#pragma once
// Read-only classification of native TTT2 projectile damage callbacks.
// This is a diagnostic boundary, NOT permission to patch projectile memory.
// Native struct offsets and move identity are checked by the caller.
#include <cstdint>
#include <string_view>

namespace cafe::title::akuma_contact {
enum class Kind : uint8_t { none, ex_hadoken, shinku_hadoken };

struct Snapshot {
    bool akuma_owner = false;
    bool separate_fighters = false;
    bool owner_targets_victim = false;
    std::string_view owner_move;
    uint32_t native_kind = 0, flags = 0;
    uint32_t projectile_capacity = 0, remaining_after_hit = 0;
    uint32_t projectile_life = 0, speed = 0, enemy_mask = 0;
};

constexpr Kind classify(const Snapshot& s) {
    // Verified Phase 81D collision signature; never infer hits from the
    // launch animation alone. TTT2's native dispatch must already have run.
    if (!s.akuma_owner || !s.separate_fighters || !s.owner_targets_victim ||
        s.native_kind != 3 || !(s.flags & 0x02000000u) ||
        (s.flags & 0x80000000u) || !s.projectile_life ||
        !s.projectile_capacity || s.remaining_after_hit >= s.projectile_capacity)
        return Kind::none;
    // Phase E11 observed native EX projectile: speed 100, enemy mask 1.
    // Require both before permitting a live EX gate (not just a move name).
    if (s.owner_move == "Ry_EX_hadoh" && s.projectile_capacity == 2 &&
        s.speed == 100 && s.enemy_mask == 1)
        return Kind::ex_hadoken;
    if (s.owner_move == "Ry_shinhadouken" && s.projectile_capacity == 5 &&
        s.speed == 120 && s.enemy_mask == 1)
        return Kind::shinku_hadoken;
    return Kind::none;
}
} // namespace cafe::title::akuma_contact

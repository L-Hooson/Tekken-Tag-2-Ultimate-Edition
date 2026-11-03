#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace cafe::title::akuma_contact {
// Private imported requirement, emitted only by Akuma's special-buffer latch.
inline constexpr uint32_t special_contact = 0x7000;
constexpr bool cancel_normal(std::string_view name) {
    return name == "Mx_lp00" || name == "Mx_lplp" || name == "Mx_4lp00" ||
           name == "Mx_rp00" || name == "Co_R_rp00L" || name == "Co_R_lp00R" ||
           name == "Co_syalp00" || name == "Co_syarp03" || name == "Co_syarp00" ||
           name == "Co_R_crp00L" || name == "Co_R_clp00R" || name == "Mx_syalk00" ||
           name == "Co_t_slp00" || name == "Co_t_srp00" || name == "Co_t_slk00" ||
           name == "Co_s_tlp00" || name == "Co_s_trp00" || name == "Mx_s_tlk00" ||
           name == "Mx_6lrk";
}
constexpr bool block_stop_condition(uint32_t kind, uint32_t next_kind, uint32_t victim_events) {
    // The converted T7 block-only stop is [47, 0x8292/0x8293]. Native TTT2
    // condition 47 includes a Tag-input test, so it cannot implement this row.
    return kind == 47 && (next_kind == 0x8292 || next_kind == 0x8293) &&
           (victim_events & 0x20000000u) != 0;
}
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

struct StopActions {
    uint32_t actor_row = 0, actor_value = 0, victim_row = 0, victim_value = 0;
    bool valid = false;
};

// A retained source requirement list is read without executing its actions.
// The adapter supplies native evaluation for additional ordinary conditions.
// Native damage flags are available before the defender's HUD events update.
template<class Read, class Check>
StopActions read_stops(uint32_t req, uint32_t flags, Read read, Check check) {
    StopActions stops;
    if (!req) return stops;
    const uint32_t contact = read(req);
    if (read(req + 4) != 0) return stops;
    const bool blocked = (flags & 0x80000000u) != 0;
    if (!((contact == 46 && !blocked && (flags & 0x02000000u)) ||
          (contact == 47 && blocked) ||
          (contact == 49 && (flags & 0x40000000u)))) return stops;
    for (unsigned i = 1; i < 64; ++i) {
        const uint32_t row = req + 8*i, kind = read(row);
        if (kind == 690) {
            stops.valid = stops.actor_row || stops.victim_row;
            return stops;
        }
        if (kind == 0x8292) {
            stops.actor_row = row; stops.actor_value = read(row + 4);
        } else if (kind == 0x8293) {
            stops.victim_row = row; stops.victim_value = read(row + 4);
        } else if (kind < 0x8000 && !check(row)) {
            return {};
        }
        // Native 0404FF30 treats bit-15 entries as actions, not predicates.
        // Do not replay their unrelated effects while extracting stop timing.
    }
    return {}; // unterminated/unknown list is not an actionable contact
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

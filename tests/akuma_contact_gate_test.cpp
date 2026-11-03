// Character-only, read-only collision classification; no game assets.
#include "title/akuma_contact_gate.h"
#include <cassert>
using namespace cafe::title::akuma_contact;

int main() {
    static_assert(classify(Snapshot{}) == Kind::none);
    Snapshot ex{true, true, true, "Ry_EX_hadoh", 3, 0x02000000, 2, 1, 12, 100, 1};
    assert(classify(ex) == Kind::ex_hadoken);
    ex.speed = 90;
    assert(classify(ex) == Kind::none); // rejected non-E11 native speed
    ex.speed = 100;
    ex.enemy_mask = 0;
    assert(classify(ex) == Kind::none); // rejected unknown targeting mask
    ex.enemy_mask = 1;
    ex.remaining_after_hit = 0;
    assert(classify(ex) == Kind::ex_hadoken);
    ex.remaining_after_hit = 2;
    assert(classify(ex) == Kind::none);
    ex.remaining_after_hit = 1;
    ex.flags |= 0x80000000u;
    assert(classify(ex) == Kind::ex_hadoken);
    assert(blocked(ex.flags));
    ex.flags = 0x80000000u;
    assert(classify(ex) == Kind::ex_hadoken); // block without a hit bit
    ex.flags = 0x04000000u;
    assert(classify(ex) == Kind::none); // chip eligibility alone is not contact
    ex.flags = 0x02000000;
    ex.akuma_owner = false;
    assert(classify(ex) == Kind::none);
    ex.akuma_owner = true;
    ex.owner_targets_victim = false;
    assert(classify(ex) == Kind::none);
    ex.owner_targets_victim = true;
    ex.native_kind = 4;
    assert(classify(ex) == Kind::none);

    Snapshot shinku{true, true, true, "Ry_shinhadouken", 3, 0x02000000, 5, 4, 25, 120, 1};
    for (uint32_t remaining = 4; remaining <= 4; --remaining) {
        shinku.remaining_after_hit = remaining;
        assert(classify(shinku) == Kind::shinku_hadoken);
        shinku.flags = 0x80000000u;
        assert(classify(shinku) == Kind::shinku_hadoken);
        shinku.flags = 0x02000000u;
        if (!remaining) break;
    }
    shinku.speed = 0;
    assert(classify(shinku) == Kind::none);
    shinku.speed = 120;
    shinku.enemy_mask = 0;
    assert(classify(shinku) == Kind::none);
    shinku.enemy_mask = 1;
    shinku.owner_move = "Ry_EX_hadoh";
    assert(classify(shinku) == Kind::none);
    return 0;
}

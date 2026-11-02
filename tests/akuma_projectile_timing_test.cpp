// Synthetic Phase 81D multi-hit timing tests: no game files required.
#include "title/akuma_projectile_timing.h"
#include <cassert>
using namespace cafe::title::akuma_projectile;
static_assert(policy(Kind::shinku_hadoken).freeze[4] == 5);
static_assert(policy(Kind::ex_hadoken).interval == 9);
int main() {
    Sequence ex(Kind::ex_hadoken);
    auto a = ex.accept({10, 1, 2, 3, 4, 1});
    assert(a.accepted && a.hit == 1 && a.freeze == 8 && a.gate_next);
    assert(!ex.accept({11, 1, 2, 3, 4, 0}).accepted);
    assert(!ex.release_due(18));
    assert(ex.release_due(19));
    a = ex.accept({19, 1, 2, 3, 4, 0});
    assert(a.accepted && a.finished && !a.gate_next && a.freeze == 5);
    assert(!ex.accept({20, 1, 2, 3, 4, 0}).accepted);
    // A different EX Hadoken in the same round may reuse the same native slot.
    assert(!ex.accept({27, 1, 2, 3, 4, 0}).accepted);
    auto next_ex = ex.accept({28, 1, 2, 3, 4, 1});
    assert(next_ex.accepted && next_ex.hit == 1 && next_ex.freeze == 8);
    ex.reset();
    assert(!ex.accept({0, 1, 2, 3, 4, 0}).accepted);
    assert(!ex.active());

    Sequence super(Kind::shinku_hadoken);
    for (unsigned i = 0; i < 5; ++i) {
        auto c = Contact{100 + 7ull*i, 1, 2, 3, 4, 4-i};
        auto result = super.accept(c);
        assert(result.accepted && result.hit == i+1);
        assert(result.freeze == (i == 4 ? 5u : 6u));
        assert(result.finished == (i == 4));
        if (i < 4) {
            assert(!super.accept({c.tick+1, 1, 2, 3, 4, 3-i}).accepted);
            assert(!super.release_due(c.tick+6));
            assert(super.release_due(c.tick+7));
        }
    }
    Sequence wrong(Kind::shinku_hadoken);
    assert(!wrong.accept({12, 1, 2, 3, 4, 3}).accepted);
    assert(wrong.accept({12, 1, 2, 3, 4, 4}).accepted);
    assert(!wrong.accept({19, 1, 7, 3, 4, 3}).accepted);
    assert(!wrong.active());
    assert(wrong.accept({20, 1, 2, 3, 4, 4}).accepted);
    assert(wrong.release_due(27));
    assert(!wrong.accept({28, 1, 2, 3, 4, 0}).accepted);
    assert(!wrong.active());
    // No fighter swap should be needed before firing the next Super.
    assert(!super.accept({129, 1, 2, 3, 4, 4}).accepted);
    const auto repeat_super = super.accept({135, 1, 2, 3, 4, 4});
    assert(repeat_super.accepted && repeat_super.hit == 1 &&
           repeat_super.freeze == 6 && repeat_super.gate_next);
}

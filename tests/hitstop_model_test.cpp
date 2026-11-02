// Standalone hitstop timing test; no game assets required.
// c++ -std=c++20 -Wall -Wextra -Werror -I runtime/src tests/hitstop_model_test.cpp -o /tmp/hitstop_model_test
#include "title/hitstop_model.h"
#include "title/akuma_projectile_timing.h"
#include "title/air_timing.h"
#include "title/akuma_contact_actions.h"

#include <cassert>
#include <stdexcept>

using cafe::title::hitstop::Model;
using cafe::title::hitstop::Timing;
using cafe::title::hitstop::decode;

int main() {
    using namespace cafe::title;
    static_assert(air_timing::landing_frame(12, 20) == 21);
    static_assert(air_timing::landing_frame(20, 20) == 21);
    static_assert(air_timing::landing_frame(25, 20) == 25);
    static_assert(air_timing::landing_frame(12, 0) == 12);
    assert(akuma_contact::hit_or_block(0x00800000));
    assert(akuma_contact::hit_or_block(0x20000000));
    assert(!akuma_contact::hit_or_block(0));
    akuma_contact::Actions actions;
    actions.begin(1, 20, 10);
    assert(actions.accept(200));
    for (unsigned tick = 0; tick < 20; ++tick) {
        actions.begin(1, 20, 10);
        assert(!actions.accept(200)); // a held pose must eventually unfreeze
    }
    assert(actions.accept(208)); // independent attacker/defender action rows
    actions.begin(1, 20, 1);
    assert(actions.accept(200)); // repeated move
    actions.begin(2, 20, 1);
    assert(actions.accept(200)); // reused fighter address
    static_assert(decode(0x00061400u).duration == 6);
    static_assert(decode(0x01051400u).delay == 1);
    static_assert(decode(0x01051400u).duration == 5);
    Model m;
    m.bind(0, 42);
    m.bind(1, 99);
    m.set(0, Timing{0, 6});
    assert(!m.frozen(0));
    for (unsigned i = 0; i < 6; ++i) {
        m.advance();
        assert(m.frozen(0));
        assert(!m.frozen(1));
    }
    m.advance();
    assert(!m.frozen(0));

    m.set(1, Timing{1, 3});
    m.advance();
    assert(!m.frozen(1));
    for (unsigned i = 0; i < 3; ++i) {
        m.advance();
        assert(m.frozen(1));
    }
    m.advance();
    assert(!m.frozen(1));

    m.set(0, Timing{0, 5});
    m.advance();
    assert(m.frozen(0));
    m.set(0, Timing{0, 2}); // replaces remaining future freeze
    assert(m.frozen(0));    // current frame stays frozen
    m.advance(); assert(m.frozen(0));
    m.advance(); assert(m.frozen(0));
    m.advance(); assert(!m.frozen(0));

    m.set(0, Timing{0, 20});
    m.bind(0, 43); // tag swap clears previous fighter's state
    m.advance();
    assert(!m.frozen(0));
    m.set(1, Timing{0, 2});
    m.reset();
    m.advance();
    assert(!m.frozen(1));

    bool invalid = false;
    try { m.set(0, Timing{511, 1}); }
    catch (const std::invalid_argument&) { invalid = true; }
    assert(invalid);
    invalid = false;
    try { m.frozen(4); } catch (const std::out_of_range&) { invalid = true; }
    assert(invalid);

    // Phase 81D: replay each of the five Super contact-stop lengths through
    // the actual battle-tick model. The first tick remains live, then one
    // native reaction dispatch tick, then exactly 6/6/6/6/5 frozen ticks.
    const auto super = cafe::title::akuma_projectile::policy(
        cafe::title::akuma_projectile::Kind::shinku_hadoken);
    for (unsigned hit = 0; hit != super.hits; ++hit) {
        Model stop;
        stop.bind(0, 11);
        stop.bind(1, 22);
        const unsigned freeze = super.freeze[hit];
        stop.set(1, Timing{1, freeze});
        assert(!stop.frozen(1));
        stop.advance();
        assert(!stop.frozen(1));
        for (unsigned i = 0; i < freeze; ++i) {
            stop.advance();
            assert(stop.frozen(1));
            assert(!stop.frozen(0));
        }
        stop.advance();
        assert(!stop.frozen(1));
    }
}

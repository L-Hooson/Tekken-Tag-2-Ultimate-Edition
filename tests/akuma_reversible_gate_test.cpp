// Synthetic guest memory only; no game files or executable are required.
#include "title/akuma_reversible_gate.h"
#include <cassert>
#include <cstdint>
#include <map>
using cafe::title::akuma_projectile::ReversibleGate;
int main() {
    constexpr uint32_t slot = 0x1100, row = 0x30000020;
    std::map<uint32_t, uint32_t> m = {{slot+60,row},{slot+32,12},{slot,55},
        {slot+4,77},{slot+8,99},{row+8,120},{row+88,1}};
    auto read = [&](uint32_t addr) { return m[addr]; };
    auto write = [&](uint32_t addr, uint32_t v) { m[addr] = v; };
    ReversibleGate g;
    assert(!g.arm(slot,row,119,1,read,write));
    assert(!g.active());
    assert(g.arm(slot,row,120,1,read,write));
    assert(g.active());
    assert(!g.arm(slot,row,120,1,read,write));
    assert(m[row+8] == 0 && m[row+88] == 0);
    m[slot] = 0;
    assert(g.hold(read,write) && m[slot] == 55);
    assert(g.release(read,write));
    assert(!g.active() && m[row+8] == 120 && m[row+88] == 1);
    assert(!g.release(read,write));
    // Recycled slot: never touch a different projectile row.
    assert(g.arm(slot,row,120,1,read,write));
    m[slot+60] = 0x30001000;
    assert(!g.hold(read,write));
    assert(!g.release(read,write));
    assert(!g.active() && m[row+8] == 0 && m[row+88] == 0);
    // Changed memory while gated: do not trample a third party's mutation.
    m[slot+60] = row; m[row+8] = 120; m[row+88] = 1;
    assert(g.arm(slot,row,120,1,read,write));
    m[row+8] = 20;
    assert(!g.release(read,write));
    assert(m[row+8] == 20 && m[row+88] == 0);
    return 0;
}

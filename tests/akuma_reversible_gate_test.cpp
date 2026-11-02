// Synthetic guest memory only; no game files required.
#include "title/akuma_reversible_gate.h"
#include <cassert>
#include <map>
using cafe::title::akuma_projectile::ReversibleGate;
int main() {
    constexpr uint32_t slot=0x1100, row=0x30000020, scratch=0x31000000;
    std::map<uint32_t,uint32_t> m{{slot+60,row},{slot+32,12},{slot,55},
        {slot+4,77},{slot+8,99},{row+8,120},{row+88,1},{row+76,0x30001200}};
    auto read=[&](uint32_t a){return m[a];};
    auto write=[&](uint32_t a,uint32_t v){m[a]=v;};
    ReversibleGate g;
    assert(!g.arm(slot,row,scratch,119,1,read,write));
    assert(g.arm(slot,row,scratch,120,1,read,write));
    assert(m[row+8]==120 && m[row+88]==1); // another shot still uses this row
    assert(m[slot+60]==scratch && m[scratch+8]==0 && m[scratch+88]==0);
    assert(m[scratch+76]==m[row+76]);
    m[slot]=0;
    assert(g.hold(read,write) && m[slot]==55);
    assert(g.release(read,write) && m[slot+60]==row && !g.active());
    assert(!g.release(read,write));
    // Native retirement must detach the copy without leaving shared data gated.
    assert(g.arm(slot,row,scratch,120,1,read,write));
    m[slot+32]=0xffffffffu;
    assert(!g.hold(read,write));
    assert(g.release(read,write) && m[slot+60]==row);
    // Slot reuse and third-party shared-row edits survive cleanup unchanged.
    m[slot+32]=12;
    assert(g.arm(slot,row,scratch,120,1,read,write));
    m[slot+60]=0x30001000; m[row+8]=20;
    assert(!g.hold(read,write));
    assert(g.release(read,write));
    assert(m[slot+60]==0x30001000 && m[row+8]==20 && m[row+88]==1);
    m[slot+60]=row; m[row+8]=120;
    // Failed scratch copy never installs a partial gate.
    auto fail=[&](uint32_t a,uint32_t v){if(a!=scratch+8)m[a]=v;};
    m[scratch+8]=99;
    assert(!g.arm(slot,row,scratch,120,1,read,fail));
    assert(!g.active() && m[slot+60]==row && m[row+8]==120 && m[row+88]==1);
}

#include "title/akuma_ex_reaction.h"
#include <cassert>
#include <cstdint>
#include <map>
using namespace cafe::title::akuma_ex_reaction;
int main() {
    static_assert(offsets[1] == 0x3c && knockdown[3] == 2535);
    constexpr uint32_t row = 0x30002000;
    std::map<uint32_t, uint16_t> mem;
    for (unsigned i=0; i<4; ++i) mem[row+offsets[i]] = stagger[i];
    const auto read=[&](uint32_t a){return mem[a];};
    const auto write=[&](uint32_t a,uint16_t v){mem[a]=v;};
    Overlay o;
    assert(!o.apply(0,read,write) && !o.active());
    assert(o.apply(row,read,write) && o.active() && o.row()==row);
    assert(!o.apply(row,read,write));
    for (unsigned i=0;i<4;++i) assert(read(row+offsets[i])==knockdown[i]);
    mem[row+offsets[2]]=9999; // unknown writer: never overwrite it
    assert(o.restore(read,write)==Restore::conflict && o.active());
    assert(mem[row+offsets[2]]==9999);
    mem[row+offsets[2]]=knockdown[2];
    assert(o.restore(read,write)==Restore::restored && !o.active());
    assert(o.restore(read,write)==Restore::nothing);
    for (unsigned i=0;i<4;++i) assert(read(row+offsets[i])==stagger[i]);
    mem[row+offsets[0]]=123;
    assert(!o.apply(row,read,write));
    assert(!o.active());
    mem[row+offsets[0]]=stagger[0];
    // Simulate one write being ignored: partial application must roll back.
    const auto flaky=[&](uint32_t a,uint16_t v){if (a!=row+offsets[1])mem[a]=v;};
    assert(!o.apply(row,read,flaky));
    assert(!o.active());
    for (unsigned i=0;i<4;++i) assert(read(row+offsets[i])==stagger[i]);
}

// Pure synthetic memory test; no extracted Tekken assets.
#include "title/akuma_ex_pushback.h"
#include <cassert>
#include <cstdint>
#include <map>
using namespace cafe::title::akuma_ex_pushback;
int main() {
    constexpr uint32_t reaction=0x30002000, source=reaction-5u*0x50u;
    std::map<uint32_t,uint16_t> half;
    std::map<uint32_t,uint32_t> word;
    for(unsigned i=0;i<7;i++) {
        word[reaction+4u*i]=0x30003100u+32u*i;
        word[source+4u*i]=0x30004100u+32u*i;
    }
    half[source+0x34]=2516; half[source+0x3c]=2517;
    half[source+0x40]=2518; half[source+0x44]=2519;
    auto r32=[&](uint32_t a){return word[a];};
    auto r16=[&](uint32_t a){return half[a];};
    auto w32=[&](uint32_t a,uint32_t v){word[a]=v;};
    const uint32_t orig=word[reaction],front=word[source];
    const uint32_t down=word[source+0x14],blocked=word[source+0x18];
    half[orig]=half[orig+2]=0;
    half[front]=48;half[front+2]=30;
    half[down]=32;half[down+2]=50;word[down+4]=8;
    half[blocked]=half[blocked+2]=0;
    Overlay o;
    assert(o.apply(reaction,r32,r16,w32));
    assert(o.active() && o.row()==reaction);
    assert(word[reaction]==front);
    assert(!o.apply(reaction,r32,r16,w32));
    word[reaction+8]=0x3000eeee;
    assert(o.restore(r32,w32)==Restore::conflict && o.active());
    assert(word[reaction+8]==0x3000eeee);
    word[reaction+8]=word[source+8];
    assert(o.restore(r32,w32)==Restore::restored && !o.active());
    for(unsigned i=0;i<7;i++)assert(word[reaction+4u*i]==0x30003100u+32u*i);
    assert(o.restore(r32,w32)==Restore::nothing);
    half[source+0x34]=1;
    assert(!o.apply(reaction,r32,r16,w32));
    half[source+0x34]=2516;
    // A failed write must restore all entries successfully written so far.
    auto broken=[&](uint32_t a,uint32_t v){if(a!=reaction+4u*3||v==0x30003100u+32u*3)word[a]=v;};
    assert(!o.apply(reaction,r32,r16,broken));
    assert(!o.active());
    for(unsigned i=0;i<7;i++)assert(word[reaction+4u*i]==0x30003100u+32u*i);
}

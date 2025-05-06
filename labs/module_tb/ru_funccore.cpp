/*********************
* Author : ABC at abc
* File   : ru_funccore.cpp
*********************/
#include "ru_funccore.hpp"
#include <cstring>

constexpr int  NUM_PORTS = 16;
constexpr int  NUM_LINES = 4;

/* ── helpers ────────────────────────────────────────────────────────────*/
inline uint16_t make_addr16(uint32_t r,uint32_t c)
{
    uint16_t bank   = ((r & 0xF) << 1) | (c >> 1);        // 0‑31
    uint16_t offset = (r>>4)*64 + ((c&1)?32:0);           // 0‑224
    return (bank<<11)|offset;                             // 5+11 bits
}
inline unsigned line_of(uint16_t addr16){ return (addr16>>11)>>3; } // bank>>3

/* ── constructor ───────────────────────────────────────────────────────*/
ru_funccore::ru_funccore(sc_core::sc_module_name n)
: sc_module(n)
, clk("clk"),reset("reset")
, i_npuc2mmu("i_npuc2mmu")
, i_mmu2npuc("i_mmu2npuc")
, i_mmu2ru  ("i_mmu2ru" ,NUM_PORTS)
, o_ru2tcm  ("o_ru2tcm" ,NUM_LINES)
, o_ru2mlsu ("o_ru2mlsu",NUM_LINES)
, i_reg_map ("i_reg_map")
{
    SC_CTHREAD(main_thread,clk.pos());
    reset_signal_is(reset,true);
}

void ru_funccore::set_Id(int v){ id=v; }

/* ── internal per‑port counter state ───────────────────────────────────*/
struct state { uint32_t row=0,col=0; };
state st[NUM_PORTS];

/* ── main behaviour ────────────────────────────────────────────────────*/
void ru_funccore::main_thread()
{
    bool fused=false;
    sc_bv<512> pack[NUM_LINES];   // 4×128b aggregator
    uint8_t    fill[NUM_LINES]={0};

    wait();
    while(true)
    {
        /* read SFR stream (non‑blocking) */
        if(i_reg_map.num_available()){
            auto sfr=i_reg_map.read();
            uint32_t w=*reinterpret_cast<uint32_t*>(sfr.get());
            fused=((w>>20)&1);
        }

        /* service all MMU ports (one pkt each) */
        for(int p=0;p<NUM_PORTS;++p)
        {
            if(!i_mmu2ru[p].num_available()) continue;
            mu2ru_PTR in=i_mmu2ru[p].read();

            uint32_t r=st[p].row;
            uint32_t c=st[p].col;
            uint16_t addr16=make_addr16(r,c);
            unsigned line=line_of(addr16);

            /* pack 16×8b → 128b at offset fill*128 */
            for(int b=0;b<16;++b)
                pack[line].range( fill[line]*128+8*b+7,
                                  fill[line]*128+8*b   )
                        = in->C_data[b];
            bool four_chunks=(++fill[line]==4);

            if(four_chunks){
                /* emit one 512‑bit packet */
                if(fused){
                    ru2mlsu_PTR o(new ru2mlsu);
                    o->data = pack[line];
                    o->done = in->done;
                    o_ru2mlsu[line].write(o);
                }else{
                    ru2tcm_PTR o(new ru2tcm);
                    o->data    = pack[line];
                    o->address = addr16;
                    o->done    = in->done;
                    o_ru2tcm[line].write(o);
                }
                fill[line]=0; pack[line]=0;
            }

            /* advance indices on last flag */
            if(in->done==1){
                st[p].col++;
                if(st[p].col==64){ st[p].col=0; st[p].row++; }
            }
        }
        wait();
    }
}

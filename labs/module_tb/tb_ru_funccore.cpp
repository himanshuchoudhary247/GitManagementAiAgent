#include "tb_ru_funccore.hpp"
#include <cassert>
#include <iostream>

/* helper to build 16‑bit address */
static uint16_t a16(uint32_t r,uint32_t c){
    uint16_t bank=((r&0xF)<<1)|(c>>1);
    uint16_t off =(r>>4)*64+((c&1)?32:0);
    return uint16_t((bank<<11)|off);
}

/* build one mmu2ru packet */
mmu2ru_PTR tb_ru_funccore::make_pkt(bool last)
{
    mmu2ru_PTR p(new mmu2ru);
    for(int i=0;i<16;i++) p->C_data[i]=sc_dt::sc_bv<8>(i);
    p->done = last;
    return p;
}

/* ── constructor ───────────────────────────────────────────────────────*/
tb_ru_funccore::tb_ru_funccore(sc_core::sc_module_name n)
: sc_module(n)
, clk("clk"), reset("reset")
, o_npuc2mmu("o_npuc2mmu"), o_mmu2npuc("o_mmu2npuc")
, o_mmu2ru("o_mmu2ru",16)
, i_ru2tcm("i_ru2tcm",4), i_ru2mlsu("i_ru2mlsu",4)
, o_reg_map("o_reg_map")
{
    SC_THREAD(handler);   sensitive << clk.pos();  dont_initialize();
    SC_THREAD(resp_tcm);  for(int i=0;i<4;i++)
                              sensitive << i_ru2tcm[i].data_written_event();
                          dont_initialize();
    SC_THREAD(resp_mlsu); for(int i=0;i<4;i++)
                              sensitive << i_ru2mlsu[i].data_written_event();
                          dont_initialize();
}

/* ── top‑level TB thread ───────────────────────────────────────────────*/
void tb_ru_funccore::handler()
{
    COMMON_REGISTERS cfg;
    tb_config::instance().get_cfg_registers(cfg);

    /* push full‑width SFR (non‑fused initially) */
    sfr_PTR map = std::make_shared<COMMON_REGISTERS>(cfg);
    map->MODE_TENSOR = 0;              // FUSED_OPERATION = 0
    o_reg_map.write(map);

    run_tc(*map);

    /* simulation ends */
    sc_core::sc_stop();
}

/* ── run single testcase (2 phases) ────────────────────────────────────*/
void tb_ru_funccore::run_tc(const COMMON_REGISTERS& cfg)
{
    uint32_t M = cfg.option_tensor_size_size_m * 8;
    uint32_t N = cfg.option_tensor_size_size_n * 32;

    /* phase‑1 : results go to TCM */
    for(uint32_t r=0;r<M;r++)
        for(uint32_t c=0;c<N;c++){
            bool last = (r==M-1 && c==N-1);
            o_mmu2ru[r & 0xF].write(make_pkt(last));
            gold_tcm_.push(a16(r,c));
        }

    /* phase‑2 : set FUSED_OPERATION bit → results to MLSU */
    wait(500, sc_core::SC_NS);

    sfr_PTR fusedSfr = std::make_shared<COMMON_REGISTERS>(cfg);
    fusedSfr->MODE_TENSOR = 1u << 20;          // bit‑20 = 1 (fused)
    o_reg_map.write(fusedSfr);

    for(uint32_t r=0;r<M;r++)
        for(uint32_t c=0;c<N;c++){
            bool last = (r==M-1 && c==N-1);
            o_mmu2ru[(r+1) & 0xF].write(make_pkt(last));
            ++gold_mlsu_;
        }
}

/* ── monitors ──────────────────────────────────────────────────────────*/
void tb_ru_funccore::resp_tcm()
{
    while(true){
        for(int l=0;l<4;l++)
            if(i_ru2tcm[l].num_available()){
                auto p = i_ru2tcm[l].read();
                assert(!gold_tcm_.empty());
                uint16_t exp = gold_tcm_.front(); gold_tcm_.pop();
                assert(p->address.to_uint() == exp);
            }
        wait(sc_core::SC_ZERO_TIME);
    }
}

void tb_ru_funccore::resp_mlsu()
{
    while(true){
        for(int l=0;l<4;l++)
            if(i_ru2mlsu[l].num_available()){
                i_ru2mlsu[l].read();      // no address to compare
                assert(gold_mlsu_ > 0); --gold_mlsu_;
            }
        wait(sc_core::SC_ZERO_TIME);
    }
}

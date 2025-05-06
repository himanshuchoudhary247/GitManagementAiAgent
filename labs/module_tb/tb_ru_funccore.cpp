#include "tb_ru_funccore.hpp"
#include "tb_config.hpp"
#include <cassert>

/* build 16-bit bank+offset address */
uint16_t tb_ru_funccore::addr16(uint32_t r,uint32_t c){
    uint16_t bank=((r&0xF)<<1)|(c>>1);
    uint16_t off =(r>>4)*64+((c&1)?32:0);
    return (bank<<11)|off;
}

/* make one mmu2ru packet */
mmu2ru_PTR tb_ru_funccore::make_pkt(bool last){
    auto p = std::make_shared<mmu2ru>();
    for(int i=0;i<16;i++) p->C_data[i]=i;
    p->done=last; return p;
}

tb_ru_funccore::tb_ru_funccore(sc_core::sc_module_name n)
: sc_core::sc_module(n)
, clk("clk"), reset("reset")
, o_npuc2mmu("o_npuc2mmu"), o_mmu2npuc("o_mmu2npuc")
, o_mmu2ru("o_mmu2ru",16), i_ru2tcm("i_ru2tcm",4)
, i_ru2mlsu("i_ru2mlsu",4), o_reg_map("o_reg_map")
{
    SC_THREAD(handler);     sensitive << clk.pos();
    SC_THREAD(resp_tcm);    for(int i=0;i<4;i++) sensitive<<i_ru2tcm[i].data_written_event();
    SC_THREAD(resp_mlsu);   for(int i=0;i<4;i++) sensitive<<i_ru2mlsu[i].data_written_event();
}

void tb_ru_funccore::handler(){
    /* load sizes from JSON */
    auto cfg = tb_config::instance().load("cfg_ru.json");
    uint32_t M = cfg.size_m*8, N = cfg.size_n*32;

    /* phase‐1: non‐fused */
    o_reg_map.write(std::make_shared<uint32_t>(0u));
    run_testcase(M,N);

    wait(500, SC_NS);

    /* phase‐2: fused */
    o_reg_map.write(std::make_shared<uint32_t>(1u<<20));
    run_testcase(M,N);

    sc_core::sc_stop();
}

void tb_ru_funccore::run_testcase(uint32_t M,uint32_t N){
    for(uint32_t r=0;r<M;r++)for(uint32_t c=0;c<N;c++){
        bool last=(r==M-1&&c==N-1);
        o_mmu2ru[r&0xF].write(make_pkt(last));
        gold_tcm_.push(addr16(r,c));
        gold_mlsu_++;
    }
}

void tb_ru_funccore::resp_tcm(){
    while(true){
        for(int l=0;l<4;l++){
            if(!i_ru2tcm[l].num_available()) continue;
            auto p = i_ru2tcm[l].read();
            assert(!gold_tcm_.empty());
            auto exp = gold_tcm_.front(); gold_tcm_.pop();
            assert(p->address.to_uint()==exp);
        }
        wait(SC_ZERO_TIME);
    }
}

void tb_ru_funccore::resp_mlsu(){
    while(true){
        for(int l=0;l<4;l++){
            if(!i_ru2mlsu[l].num_available()) continue;
            i_ru2mlsu[l].read();
            assert(gold_mlsu_>0); --gold_mlsu_;
        }
        wait(SC_ZERO_TIME);
    }
}

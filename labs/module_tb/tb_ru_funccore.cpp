#include "tb_ru_funccore.hpp"
#include <cassert>
#include <iostream>

/* helper */
static uint16_t addr16(uint32_t r,uint32_t c)
{
    uint16_t bank=((r&0xF)<<1)|(c>>1);
    uint16_t off =(r>>4)*64+((c&1)?32:0);
    return (bank<<11)|off;
}

mu2ru_PTR tb_ru_funccore::make_pkt(bool last)
{
    mu2ru_PTR p(new mu2ru);
    for(int i=0;i<16;i++) p->C_data[i]=sc_bv<8>(i);
    p->done=last;
    return p;
}

tb_ru_funccore::tb_ru_funccore(sc_core::sc_module_name n)
: sc_module(n),clk("clk"),reset("reset")
, o_npuc2mmu("o_npuc2mmu"),o_mmu2npuc("o_mmu2npuc")
, o_mmu2ru("o_mmu2ru",PORTS),i_ru2tcm("i_ru2tcm",LINES)
, i_ru2mlsu("i_ru2mlsu",LINES),o_reg_map("o_reg_map")
{
    SC_THREAD(handler);        sensitive<<clk.pos(); dont_initialize();
    SC_THREAD(resp_tcm);       for(int i=0;i<LINES;i++)
                                   sensitive<<i_ru2tcm[i].data_written();
                               dont_initialize();
    SC_THREAD(resp_mlsu);      for(int i=0;i<LINES;i++)
                                   sensitive<<i_ru2mlsu[i].data_written();
                               dont_initialize();
    SC_METHOD(receive_done);   sensitive<<o_mmu2npuc.data_written();
                               dont_initialize();
}

void tb_ru_funccore::handler()
{
    common_register_map cfg; tb_config::instance().get_cfg_registers(cfg);
    sfr_PTR map(new common_register_map()); *map=cfg; o_reg_map.write(map);
    run_testcase(cfg);
    sc_stop();
}

void tb_ru_funccore::run_testcase(const common_register_map& cfg)
{
    uint32_t M=cfg.option_tensor_size_size_m*8;
    uint32_t N=cfg.option_tensor_size_size_n*32;

    /* phase‑1 → TCM */
    uint32_t sfr0=0; o_reg_map.write(sfr_PTR(new uint32_t(sfr0)));
    for(uint32_t r=0;r<M;r++)
        for(uint32_t c=0;c<N;c++){
            bool last=(r==M-1&&c==N-1);
            o_mmu2ru[r&0xF].write(make_pkt(last));
            gold_tcm_.push(addr16(r,c));
        }

    /* phase‑2 → MLSU */
    wait(500,SC_NS);
    uint32_t sfr1=1u<<20; o_reg_map.write(sfr_PTR(new uint32_t(sfr1)));
    for(uint32_t r=0;r<M;r++)
        for(uint32_t c=0;c<N;c++){
            bool last=(r==M-1&&c==N-1);
            o_mmu2ru[(r+1)&0xF].write(make_pkt(last));
            gold_mlsu_++;
        }
}

/* monitors */
void tb_ru_funccore::resp_tcm()
{
    while(true){
        for(int l=0;l<LINES;l++)
            if(i_ru2tcm[l].num_available()){
                auto p=i_ru2tcm[l].read();
                assert(!gold_tcm_.empty());
                uint16_t g=gold_tcm_.front(); gold_tcm_.pop();
                assert(p->address.to_uint()==g);
            }
        wait(SC_ZERO_TIME);
    }
}

void tb_ru_funccore::resp_mlsu()
{
    while(true){
        for(int l=0;l<LINES;l++)
            if(i_ru2mlsu[l].num_available()){
                i_ru2mlsu[l].read();           // addressless compare
                assert(gold_mlsu_>0); --gold_mlsu_;
            }
        wait(SC_ZERO_TIME);
    }
}

void tb_ru_funccore::receive_done(){ done_flag=true; }

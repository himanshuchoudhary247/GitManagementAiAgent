#include "tb_ru_funccore.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

static inline uint32_t bank_of(uint32_t r,uint32_t c){
    return ((r&0xF)<<1)|(c>>1);
}
static inline uint32_t off_in_bank(uint32_t r,uint32_t c){
    return (r>>4)*64 + ((c&1)?32:0);
}
static uint64_t make_phys(uint32_t r,uint32_t c){
    return (uint64_t(bank_of(r,c))<<32) | off_in_bank(r,c);
}

tb_ru_funccore::tb_ru_funccore(sc_core::sc_module_name n)
: sc_module(n)
, clk("clk"), reset("reset")
, o_npuc2mmu("o_npuc2mmu")
, o_mmu2npuc("o_mmu2npuc")
, o_mmu2ru("o_mmu2ru",NUM_PORTS)
, i_ru2tcm("i_ru2tcm",NUM_LINES)
, i_ru2mlsu("i_ru2mlsu",NUM_LINES)
, o_reg_map("o_reg_map")
{
    SC_THREAD(driver_thread);  sensitive<<clk.pos();               dont_initialize();
    SC_THREAD(monitor_tcm);    for(int i=0;i<NUM_LINES;i++) sensitive<<i_ru2tcm[i].data_written(); dont_initialize();
    SC_THREAD(monitor_mlsu);   for(int i=0;i<NUM_LINES;i++) sensitive<<i_ru2mlsu[i].data_written();dont_initialize();
}

mmu2ru_PTR tb_ru_funccore::make_pkt(uint32_t r,uint32_t c,bool last){
    auto p=std::make_shared<mmu2ru_t>();
    p->row=r; p->col=c;
    std::memset(p->C_data.data(),0,sizeof(p->C_data));
    p->done=last;
    return p;
}

sfr_PTR tb_ru_funccore::make_sfr(bool fused){
    auto s=std::make_shared<common_register_map>(cfg_);
    s->mode_tensor.FUSED_OPERATION = fused?1:0;
    return s;
}

void tb_ru_funccore::driver_thread(){
    wait(); while(reset.read()) wait();
    tb_config::instance().get_cfg_registers(cfg_);
    o_reg_map.write(std::make_shared<common_register_map>(cfg_));
    fill_golden();
    uint32_t M=cfg_.option_tensor_size_size_m*8;
    uint32_t N=cfg_.option_tensor_size_size_n*32;

    // non-fused
    o_reg_map.write(make_sfr(false));
    for(uint32_t r=0;r<M;r++) for(uint32_t c=0;c<N;c++)
        o_mmu2ru[r&0xF].write(make_pkt(r,c, r+1==M&&c+1==N));

    // fused
    wait(200,SC_NS);
    o_reg_map.write(make_sfr(true));
    for(uint32_t r=0;r<M;r++) for(uint32_t c=0;c<N;c++)
        o_mmu2ru[(r+1)&0xF].write(make_pkt(r,c, r+1==M&&c+1==N));

    while(!golden_tcm_.empty()||!golden_mlsu_.empty()) wait();
    std::cout<<"[TB_RU] ✓ addresses verified @"<<sc_time_stamp()<<'\n';
    sc_stop();
}

void tb_ru_funccore::fill_golden(){
    uint32_t M=cfg_.option_tensor_size_size_m*8;
    uint32_t N=cfg_.option_tensor_size_size_n*32;
    for(uint32_t r=0;r<M;r++) for(uint32_t c=0;c<N;c++){
        uint64_t a=make_phys(r,c);
        golden_tcm_.push(a);
        golden_mlsu_.push(a);
    }
}

void tb_ru_funccore::monitor_tcm(){
    while(true){
        for(int l=0;l<NUM_LINES;l++){
            while(i_ru2tcm[l].num_available()){
                auto pkt=i_ru2tcm[l].read();
                assert(!golden_tcm_.empty());
                uint64_t e=golden_tcm_.front(); golden_tcm_.pop();
                assert(pkt->address[l]==uint16_t(e>>32));
            }
        }
        wait(SC_ZERO_TIME);
    }
}

void tb_ru_funccore::monitor_mlsu(){
    while(true){
        for(int l=0;l<NUM_LINES;l++){
            while(i_ru2mlsu[l].num_available()){
                i_ru2mlsu[l].read();
                assert(!golden_mlsu_.empty());
                golden_mlsu_.pop();
            }
        }
        wait(SC_ZERO_TIME);
    }
}

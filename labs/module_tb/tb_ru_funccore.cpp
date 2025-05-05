#include "tb_ru_funccore.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

/* address helpers */
static uint32_t bank_of(uint32_t r,uint32_t c){ return ((r&0xF)<<1)|(c>>1); }
static uint32_t off_in_bank(uint32_t r,uint32_t c){ return (r>>4)*64+((c&1)?32:0); }
static uint64_t make_addr(uint32_t r,uint32_t c,uint64_t hi)
{
    return (hi & ~0xFFFFFFFFULL) | (uint64_t(bank_of(r,c))<<32) | off_in_bank(r,c);
}

/*──────── constructor ────────*/
tb_ru_funccore::tb_ru_funccore(sc_core::sc_module_name n)
: sc_module(n)
, clk("clk"), reset("reset")
, o_mmu2ru("o_mmu2ru", NUM_PORTS)
, i_ru2tcm("i_ru2tcm", NUM_LINES)
, i_ru2mlsu("i_ru2mlsu", NUM_LINES)
, o_reg_map("o_reg_map")
{
    SC_THREAD(driver_thread);  sensitive<<clk.pos();  dont_initialize();
    SC_THREAD(monitor_tcm);    for(int i=0;i<NUM_LINES;i++)
                                   sensitive<<i_ru2tcm[i].data_written();
                               dont_initialize();
    SC_THREAD(monitor_mlsu);   for(int i=0;i<NUM_LINES;i++)
                                   sensitive<<i_ru2mlsu[i].data_written();
                               dont_initialize();
}

/*──────── helper builders ─────*/
mmu2ru_PTR tb_ru_funccore::make_pkt(uint32_t r,uint32_t c,bool last)
{
    mmu2ru_PTR p(new mmu2ru_t);
    p->row=r; p->col=c; p->done=last;
    std::memset(p->C_data,0,sizeof(p->C_data));
    return p;
}
sfr_PTR tb_ru_funccore::make_sfr(bool fused)
{
    sfr_PTR p(new common_register_map);
    p->MODE_TENSOR.FUSED_OPERATION = fused ? 1 : 0;   /* field name per spec */
    return p;
}

/*──────── driver thread ───────*/
void tb_ru_funccore::driver_thread()
{
    wait(); while(reset.read()) wait();

    common_register_map cfg;
    tb_config::instance().get_cfg_registers(cfg);

    /* push full map once */
    o_reg_map.write( sfr_PTR(new common_register_map(cfg)) );

    build_golden(cfg);

    uint32_t M = cfg.option_tensor_size_size_m*8;
    uint32_t N = cfg.option_tensor_size_size_n*32;

    /* phase‑1: non‑fused */
    o_reg_map.write( make_sfr(false) );
    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
            o_mmu2ru[r & 0xF].write( make_pkt(r,c) );

    /* phase‑2: fused */
    wait(200,SC_NS);
    o_reg_map.write( make_sfr(true) );
    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
            o_mmu2ru[(r+1)&0xF].write( make_pkt(r,c) );

    while(!golden_.empty()) wait();   // wait until monitors empty queue
    std::cout<<"[TB_RU] ✓ finished @"<<sc_time_stamp()<<std::endl;
    sc_stop();
}

/*──────── golden list ─────────*/
void tb_ru_funccore::build_golden(const common_register_map& cfg)
{
    uint32_t M = cfg.option_tensor_size_size_m*8;
    uint32_t N = cfg.option_tensor_size_size_n*32;
    uint64_t hi= cfg.addr_tensor_matrix_c_base_matrix_c_base_addr;

    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
        {
            golden_.push( std::make_pair(make_addr(r,c,hi), false) );
            golden_.push( std::make_pair(make_addr(r,c,hi), true ) );
        }
}

/*──────── monitor threads ─────*/
void tb_ru_funccore::monitor_tcm()
{
    while(true)
    {
        for(int l=0;l<NUM_LINES;l++)
            while(i_ru2tcm[l].num_available())
            {
                ru2tcm_PTR pkt=i_ru2tcm[l].read();
                std::pair<uint64_t,bool> g=golden_.front(); golden_.pop();
                assert(!g.second);
                assert(pkt->address[l]==uint16_t(g.first>>32));
#if DBG_TB
                std::cout<<sc_time_stamp()<<" TCM OK 0x"
                         <<std::hex<<g.first<<std::dec<<'\n';
#endif
            }
        wait(SC_ZERO_TIME);
    }
}
void tb_ru_funccore::monitor_mlsu()
{
    while(true)
    {
        for(int l=0;l<NUM_LINES;l++)
            while(i_ru2mlsu[l].num_available())
            {
                ru2mlsu_PTR pkt=i_ru2mlsu[l].read();
                std::pair<uint64_t,bool> g=golden_.front(); golden_.pop();
                assert(g.second);
#if DBG_TB
                std::cout<<sc_time_stamp()<<" MLSU OK 0x"
                         <<std::hex<<g.first<<std::dec<<'\n';
#endif
                (void)pkt;
            }
        wait(SC_ZERO_TIME);
    }
}

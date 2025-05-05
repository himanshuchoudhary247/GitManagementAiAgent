/*********************************************************************
*  tb_ru_funccore.cpp  –  Implementation
*********************************************************************/
#include "tb_ru_funccore.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

/* helper mapping (same as RTL) */
static inline uint32_t bank_of(uint32_t r,uint32_t c){return ((r&0xF)<<1)|(c>>1);}
static inline uint32_t off_in_bank(uint32_t r,uint32_t c){return (r>>4)*64 + ((c&1)?32:0);}
static uint64_t make_addr(uint32_t r,uint32_t c,uint64_t hi)
{
    return (hi & ~0xFFFFFFFFULL) | (uint64_t(bank_of(r,c))<<32) | off_in_bank(r,c);
}

/*──────────────────────── constructor ──────────────────────────────*/
tb_ru_funccore::tb_ru_funccore(sc_core::sc_module_name n)
: sc_module (n)
, clk("clk"), reset("reset")
, o_npuc2mmu("o_npuc2mmu")
, o_mmu2npuc("o_mmu2npuc")
, o_mmu2ru("o_mmu2ru", NUM_PORTS)
, i_ru2tcm("i_ru2tcm", NUM_LINES)
, i_ru2mlsu("i_ru2mlsu", NUM_LINES)
, o_reg_map("o_reg_map")
{
    SC_THREAD(driver_thread);  sensitive << clk.pos(); dont_initialize();
    SC_THREAD(monitor_tcm);    for(int i=0;i<NUM_LINES;i++)
                                   sensitive << i_ru2tcm[i].data_written();
                               dont_initialize();
    SC_THREAD(monitor_mlsu);   for(int i=0;i<NUM_LINES;i++)
                                   sensitive << i_ru2mlsu[i].data_written();
                               dont_initialize();
}

/*──────────────────────── packet builders ─────────────────────────*/
mmu2ru_PTR tb_ru_funccore::make_pkt(uint32_t r,uint32_t c,bool last)
{
    auto p = std::make_shared<mmu2ru_t>();
    p->row  = r;
    p->col  = c;
    std::memset(p->C_data, 0, sizeof(p->C_data));
    p->done = last;
    return p;
}
sfr_PTR tb_ru_funccore::make_sfr(bool fused)
{
    auto p = std::make_shared<common_register_map>();
    p->mode_tensor.FUSED_OPERATION = fused;
    return p;
}

/*──────────────────────── driver thread ───────────────────────────*/
void tb_ru_funccore::driver_thread()
{
    wait(); while(reset.read()) wait();          // wait for reset release

    /* Load register map from tb_config (singleton) */
    common_register_map cfg;
    tb_config::instance().get_cfg_registers(cfg);

    /* Push full map once (reset + tensor sizes) */
    o_reg_map.write( std::make_shared<common_register_map>(cfg) );

    /* Build golden expectation list */
    golden_fill(cfg);

    uint32_t M = cfg.option_tensor_size_size_m*8;
    uint32_t N = cfg.option_tensor_size_size_n*32;

    /* -------- Phase‑1 : non‑fused → TCM ------------------------- */
    o_reg_map.write( make_sfr(false) );
    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
            o_mmu2ru[r & 0xF].write( make_pkt(r,c) );

    /* -------- Phase‑2 : fused → MLSU ---------------------------- */
    wait(200,SC_NS);
    o_reg_map.write( make_sfr(true) );
    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
            o_mmu2ru[(r+1) & 0xF].write( make_pkt(r,c) );

    /* Wait until all golden entries verified */
    while(!golden_.empty()) wait();

    std::cout << "[TB_RU] ✓ simulation finished @" << sc_time_stamp() << '\n';
    sc_stop();
}

/*──────────────────────── golden fill ─────────────────────────────*/
void tb_ru_funccore::golden_fill(const common_register_map& cfg)
{
    uint32_t M = cfg.option_tensor_size_size_m*8;
    uint32_t N = cfg.option_tensor_size_size_n*32;
    uint64_t hi = cfg.addr_tensor_matrix_c_base_matrix_c_base_addr;

    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
        {
            golden_.push({ make_addr(r,c,hi), false });  // expect in TCM phase
            golden_.push({ make_addr(r,c,hi), true  });  // expect in MLSU phase
        }
}

/*──────────────────────── monitor threads ─────────────────────────*/
void tb_ru_funccore::monitor_tcm()
{
    while(true)
    {
        for(int l=0;l<NUM_LINES;l++)
            while(i_ru2tcm[l].num_available())
            {
                ru2tcm_PTR pkt = i_ru2tcm[l].read();
                auto [addr,fused] = golden_.front(); golden_.pop();
                assert(!fused);                         // should be non‑fused
                assert(pkt->address[l] == uint16_t(addr >> 32));
#if DBG_TB
                std::cout << sc_time_stamp() << " TCM OK 0x"
                          << std::hex << addr << std::dec << '\n';
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
                ru2mlsu_PTR pkt = i_ru2mlsu[l].read();
                auto [addr,fused] = golden_.front(); golden_.pop();
                assert(fused);                         // should be fused
#if DBG_TB
                std::cout << sc_time_stamp() << " MLSU OK 0x"
                          << std::hex << addr << std::dec << '\n';
#endif
                (void)pkt;   // payload not checked in this demo
            }
        wait(SC_ZERO_TIME);
    }
}

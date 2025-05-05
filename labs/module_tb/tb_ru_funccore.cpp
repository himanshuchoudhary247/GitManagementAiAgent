#include "tb_ru_funccore.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

/* ───────────────────────────────────────────────────────────── helpers */
inline uint32_t bank_of(uint32_t r,uint32_t c){ return ((r&0xF)<<1)|(c>>1); }
inline uint32_t off_in_bank(uint32_t r,uint32_t c){ return (r>>4)*64+((c&1)?32:0); }
static uint64_t make_addr(uint32_t r,uint32_t c,uint64_t base_hi)
{
    return (base_hi & ~0xFFFFFFFFULL) |
           (uint64_t(bank_of(r,c))<<32) | off_in_bank(r,c);
}

/* ───────────────────────────────────────────────────────── constructor */
tb_ru_funccore::tb_ru_funccore(sc_core::sc_module_name n)
: sc_module(n)
, clk("clk"), reset("reset")
, o_npuc2mmu("o_npuc2mmu")
, o_mmu2npuc("o_mmu2npuc")
, o_mmu2ru("o_mmu2ru", NUM_PORTS)
, i_ru2tcm("i_ru2tcm", NUM_LINES)
, i_ru2mlsu("i_ru2mlsu", NUM_LINES)
, o_reg_map("o_reg_map")
{
    SC_THREAD(main_thread); sensitive << clk.pos(); dont_initialize();
    SC_THREAD(resp_tcm);    for(int i=0;i<NUM_LINES;i++)
                                sensitive << i_ru2tcm[i].data_written();
                            dont_initialize();
    SC_THREAD(resp_mlsu);   for(int i=0;i<NUM_LINES;i++)
                                sensitive << i_ru2mlsu[i].data_written();
                            dont_initialize();
}

/* ───────────────────────────────────────────────────────── main thread */
void tb_ru_funccore::main_thread()
{
    /* reset phase */
    wait(); while(reset.read()) wait();

    /* load config once */
    common_register_map cfg;
    tb_config::instance().get_cfg_registers(cfg);

    /* send full register block to RU */
    sfr_PTR map(new common_register_map(cfg));
    o_reg_map.write(map);

    /* fill golden queue for both phases */
    golden_fill(cfg);

    /* ---------- Phase‑1 : non‑fused → TCM -------------------------- */
    o_reg_map.write( make_sfr(false) );

    uint32_t M = cfg.option_tensor_size_size_m*8;
    uint32_t N = cfg.option_tensor_size_size_n*32;
    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
            o_mmu2ru[r & 0xF].write( make_mmu_pkt(r,c) );

    /* ---------- Phase‑2 : fused → MLSU ----------------------------- */
    wait(500, SC_NS);
    o_reg_map.write( make_sfr(true) );

    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
            o_mmu2ru[(r+1) & 0xF].write( make_mmu_pkt(r,c) );

    /* wait until both resp threads empty the golden queue */
    while(!golden_q_.empty()) wait();

    std::cout << "[TB_RU] ✓ all packets verified @" << sc_time_stamp() << '\n';
    sc_stop();
}

/* ────────────────────────────────────────────────── golden generation */
void tb_ru_funccore::golden_fill(const common_register_map& cfg)
{
    uint32_t M = cfg.option_tensor_size_size_m * 8;
    uint32_t N = cfg.option_tensor_size_size_n * 32;
    uint64_t hi = cfg.addr_tensor_matrix_c_base_matrix_c_base_addr;

    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
        {
            golden_q_.emplace( make_addr(r,c,hi), false );  // to TCM
            golden_q_.emplace( make_addr(r,c,hi), true  );  // to MLSU
        }
}

/* ─────────────────────────────────────────────────── pkt constructors */
mmu2ru_PTR tb_ru_funccore::make_mmu_pkt(uint32_t r,uint32_t c)
{
    mmu2ru_PTR p(new mmu2ru_t);
    p->row  = r;
    p->col  = c;
    p->done = false;
    std::memset(p->C_data, 0, sizeof(p->C_data));
    return p;
}
sfr_PTR tb_ru_funccore::make_sfr(bool fused)
{
    sfr_PTR p(new common_register_map);
    p->mode_tensor.FUSED_OPERATION = fused;
    return p;
}

/* ────────────────────────────────────────────────────────── monitors */
void tb_ru_funccore::resp_tcm()
{
    while(true)
    {
        for(int l=0;l<NUM_LINES;l++)
            while(i_ru2tcm[l].num_available())
            {
                ru2tcm_PTR pkt = i_ru2tcm[l].read();
                auto [addr,fused] = golden_q_.front();
                assert(!fused);
                uint16_t addr_hi = uint16_t(addr>>32);
                assert(pkt->address[l] == addr_hi);
                golden_q_.pop();
            }
        wait(SC_ZERO_TIME);
    }
}
void tb_ru_funccore::resp_mlsu()
{
    while(true)
    {
        for(int l=0;l<NUM_LINES;l++)
            while(i_ru2mlsu[l].num_available())
            {
                ru2mlsu_PTR pkt = i_ru2mlsu[l].read();
                auto [addr,fused] = golden_q_.front();
                assert(fused);
                (void)pkt;  /* payload not checked in this demo */
                golden_q_.pop();
            }
        wait(SC_ZERO_TIME);
    }
}

#include "tb_ru_funccore.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

/*──────────────── helper: address mapping ───────────────────────────*/
static inline uint32_t bank_of(uint32_t r,uint32_t c)   { return ((r&0xF)<<1)|(c>>1); }
static inline uint32_t off_in_bank(uint32_t r,uint32_t c){ return (r>>4)*64 + ((c&1)?32:0); }
static inline uint64_t make_addr(uint32_t r,uint32_t c,uint64_t hi)
{
    return (hi & ~0xFFFFFFFFULL)
         | (uint64_t(bank_of(r,c))<<32)
         |  off_in_bank(r,c);
}

/*──────────────── constructor ──────────────────────────────────────*/
tb_ru_funccore::tb_ru_funccore(sc_core::sc_module_name n)
: sc_module(n)
, clk       ("clk")
, reset     ("reset")
, o_npuc2mmu("o_npuc2mmu")
, o_mmu2npuc("o_mmu2npuc")
, o_mmu2ru  ("o_mmu2ru",  NUM_PORTS)
, i_ru2tcm  ("i_ru2tcm",  NUM_LINES)
, i_ru2mlsu ("i_ru2mlsu", NUM_LINES)
, o_reg_map ("o_reg_map")
{
    SC_THREAD(driver_thread);      sensitive << clk.pos();    dont_initialize();
    SC_THREAD(monitor_tcm);        for(int i=0;i<NUM_LINES;i++) sensitive << i_ru2tcm[i].data_written(); dont_initialize();
    SC_THREAD(monitor_mlsu);       for(int i=0;i<NUM_LINES;i++) sensitive << i_ru2mlsu[i].data_written(); dont_initialize();
}

/*────────────────── setters ───────────────────────────────────────*/
void tb_ru_funccore::set_id(int id)                   { id_   = id; }
void tb_ru_funccore::set_args(int argc, char* argv[]) { argc_ = argc; argv_ = argv; }

/*────────────────── packet builders ─────────────────────────────────*/
mmu2ru_PTR tb_ru_funccore::make_pkt(uint32_t r,uint32_t c,bool last)
{
    mmu2ru_PTR p(new mmu2ru_t);
    p->row  = r;
    p->col  = c;
    //! Assuming C_data is an array of 8×32-bit elements:
    std::memset(p->C_data, 0, sizeof(p->C_data));
    p->done = last;
    return p;
}

sfr_PTR tb_ru_funccore::make_sfr(bool fused)
{
    sfr_PTR p(new common_register_map());
    p->mode_tensor.FUSED_OPERATION = fused ? 1 : 0;
    return p;
}

/*────────────────── driver thread ──────────────────────────────────*/
void tb_ru_funccore::driver_thread()
{
    wait(); while(reset.read()) wait();  // wait de-assert

    // 1) load register map
    common_register_map cfg;
    tb_config::instance().get_cfg_registers(cfg);

    // 2) push full map (initial)
    o_reg_map.write( sfr_PTR(new common_register_map(cfg)) );

    // 3) build golden expectations
    build_golden(cfg);

    uint32_t M = cfg.option_tensor_size_size_m * 8;
    uint32_t N = cfg.option_tensor_size_size_n * 32;

    // Phase-1: non-fused → TCM
    o_reg_map.write( make_sfr(false) );
    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
            o_mmu2ru[r & 0xF].write( make_pkt(r,c, r==M-1 && c==N-1) );

    // small delay
    wait(200, SC_NS);

    // Phase-2: fused → MLSU
    o_reg_map.write( make_sfr(true) );
    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
            o_mmu2ru[(r+1) & 0xF].write( make_pkt(r,c, r==M-1 && c==N-1) );

    // wait until all golden entries are consumed
    while(!golden_.empty()) wait();
    std::cout << "[TB_RU] ✓ All packets verified @" << sc_time_stamp() << "\n";
    sc_stop();
}

/*────────────────── build golden list ───────────────────────────────*/
void tb_ru_funccore::build_golden(const common_register_map& cfg)
{
    uint32_t M  = cfg.option_tensor_size_size_m * 8;
    uint32_t N  = cfg.option_tensor_size_size_n * 32;
    uint64_t hi = cfg.addr_tensor_matrix_c_base_matrix_c_base_addr;

    for(uint32_t r=0;r<M;++r)
    for(uint32_t c=0;c<N;++c)
    {
        uint64_t a = make_addr(r,c,hi);
        golden_.push(std::make_pair(a,false));
        golden_.push(std::make_pair(a,true ));
    }
}

/*────────────────── monitor TCM ────────────────────────────────────*/
void tb_ru_funccore::monitor_tcm()
{
    while(true)
    {
        for(int i=0;i<NUM_LINES;i++)
            while(i_ru2tcm[i].num_available())
            {
                ru2tcm_PTR pkt = i_ru2tcm[i].read();
                auto pr = golden_.front(); golden_.pop();
                assert(pr.second == false);
                assert(pkt->address[i] == uint16_t(pr.first >> 32));
                (void)pkt;
            }
        wait(SC_ZERO_TIME);
    }
}

/*────────────────── monitor MLSU ───────────────────────────────────*/
void tb_ru_funccore::monitor_mlsu()
{
    while(true)
    {
        for(int i=0;i<NUM_LINES;i++)
            while(i_ru2mlsu[i].num_available())
            {
                ru2mlsu_PTR pkt = i_ru2mlsu[i].read();
                auto pr = golden_.front(); golden_.pop();
                assert(pr.second == true);
                (void)pkt;
            }
        wait(SC_ZERO_TIME);
    }
}

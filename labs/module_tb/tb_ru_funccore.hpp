#pragma once
#include "npucommon.hpp"
#include "npudefine.hpp"
#include "tb_config.hpp"            // same helper class as in LU TB
#include <queue>
#include <tuple>

#define NUM_PORTS 16
#define NUM_LINES 4

class tb_ru_funccore : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(tb_ru_funccore);
    tb_ru_funccore(sc_core::sc_module_name name,
                   int              argc,
                   char*            argv[]);

    /*──── DUT-side ports (bound by tb_top_ru) ───────────────────────────*/
    sc_in <bool>               clk;
    sc_in <bool>               reset;

    sc_fifo_out<npuc2mmu_PTR>  o_npuc2mmu;
    sc_fifo_out<mmu2npuc_PTR>  o_mmu2npuc;
    sc_vector< sc_fifo_out<mmu2ru_PTR> >  o_mmu2ru;      // 16
    sc_vector< sc_fifo_in <ru2tcm_PTR> >  i_ru2tcm;      // 4
    sc_vector< sc_fifo_in <ru2mlsu_PTR> > i_ru2mlsu;     // 4
    sc_fifo_out<sfr_PTR>       o_reg_map;

    void set_id(int id_) { id = id_; }

private:
    /******** threads & methods *******************************************/
    void handlerTestMain();          // main testcase orchestrator
    void respru2tcm();               // monitor RU→TCM lines
    void respru2mlsu();              // monitor RU→MLSU lines
    void receive_done_micro();       // optional micro-done handshake

    /******** helper routines *********************************************/
    void  run_testcase();
    void  golden_addr_gen_matrix_c(uint32_t M,uint32_t N,
                                   uint32_t base,uint32_t rowStride,uint32_t colStride);
    void  compare_golden_output_tcm (ru2tcm_PTR  pkt);
    void  compare_golden_output_mlsu(ru2mlsu_PTR pkt);

    mmu2ru_PTR make_mmu_pkt(uint32_t row,uint32_t col);
    sfr_PTR    make_sfr(bool fused);

private:
    std::queue<std::tuple<std::uint64_t,bool>> golden_q_;   // ‹addr,fused?›
    int   id{0};
};

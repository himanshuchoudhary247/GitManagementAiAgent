#pragma once
#include "npucommon.hpp"
#include "npudefine.hpp"
#include "sfr/unique_registers.h"          // brings in common_register_map
#include <queue>
#include <tuple>

/* small loader that simply hands the one built‑in config to the TB */
class tb_config {
public:
    explicit tb_config(int argc, char* argv[]);
    bool     get_next_testcase(common_register_map& out);   // returns true once
private:
    bool delivered_{false};
    common_register_map reg_;
};

/* --------------------------------------------------------------------- */
#define NUM_PORTS 16
#define NUM_LINES 4
#define DEBUG_LOG_SEVERITY_TB 1

class tb_ru_funccore : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(tb_ru_funccore);
    tb_ru_funccore(sc_core::sc_module_name name,
                   int argc,
                   char* argv[]);

    /* ports identical to MALU TB style */
    sc_in <bool>              clk;
    sc_in <bool>              reset;
    sc_fifo_out<npuc2mmu_PTR> o_npuc2mmu;
    sc_fifo_out<mmu2npuc_PTR> o_mmu2npuc;
    sc_vector< sc_fifo_out<mmu2ru_PTR> >  o_mmu2ru;
    sc_vector< sc_fifo_in <ru2tcm_PTR> >  i_ru2tcm;
    sc_vector< sc_fifo_in <ru2mlsu_PTR> > i_ru2mlsu;
    sc_fifo_out<sfr_PTR>      o_reg_map;

    /* MALU‑style setter */
    void set_id(int v){ id_ = v; }

private:
    /* threads */
    void handlerTestMain();
    void respru2tcm();
    void respru2mlsu();
    void receive_done_micro();   /* dummy */

    /* helpers */
    void run_testcase(const common_register_map& cfg);
    void golden_addr_gen_matrix_c(uint32_t M,uint32_t N,
                                  uint32_t base,uint32_t rs,uint32_t cs);
    void compare_golden_output_tcm (ru2tcm_PTR  p);
    void compare_golden_output_mlsu(ru2mlsu_PTR p);
    mmu2ru_PTR make_mmu_pkt(uint32_t r,uint32_t c);
    sfr_PTR    make_sfr(bool fused);

private:
    tb_config cfg_loader_;
    std::queue<std::tuple<std::uint64_t,bool>> golden_q_;
    bool done_micro{false};
    int  id_{0};
};

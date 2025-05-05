#pragma once
#include "npucommon.hpp"
#include "npudefine.hpp"
#include "sfr/unique_registers.h"   // brings in common_register_map
#include "tb_config.hpp"

#include <queue>
#include <tuple>

/* ------------------------------------------------------------------ */
#define NUM_PORTS 16
#define NUM_LINES 4
#define DEBUG_LOG_SEVERITY_TB 1

class tb_ru_funccore : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(tb_ru_funccore);
    explicit tb_ru_funccore(sc_core::sc_module_name name);

    /* Ports identical to ru.hpp wiring (see tb_top_ru.cpp) */
    sc_in <bool>              clk;
    sc_in <bool>              reset;

    sc_fifo_out<npuc2mmu_PTR> o_npuc2mmu;   // start trigger (optional)
    sc_fifo_out<mmu2npuc_PTR> o_mmu2npuc;   // not used, but wired

    sc_vector< sc_fifo_out<mmu2ru_PTR> >  o_mmu2ru;   // 16 lanes → RU
    sc_vector< sc_fifo_in <ru2tcm_PTR> >  i_ru2tcm;   // 4 lanes ← RU
    sc_vector< sc_fifo_in <ru2mlsu_PTR> > i_ru2mlsu;  // 4 lanes ← RU (fused)

    sc_fifo_out<sfr_PTR>      o_reg_map;    // write full register map

    /* ----- public helpers ----- */
    void set_id(int v) { id_ = v; }

private:
    /* threads */
    void main_thread();                   // test‑sequence driver
    void resp_tcm();                      // monitor RU→TCM
    void resp_mlsu();                     // monitor RU→MLSU

    /* helpers */
    void golden_fill(const common_register_map& cfg);
    mmu2ru_PTR make_mmu_pkt(uint32_t row,uint32_t col);
    sfr_PTR    make_sfr(bool fused);

    /* queues */
    std::queue<std::tuple<uint64_t,bool>> golden_q_;  // <addr,fused?>

    /* state */
    int  id_{0};
};

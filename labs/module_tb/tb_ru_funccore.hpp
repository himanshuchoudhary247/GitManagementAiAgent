#pragma once
#include "npucommon.hpp"
#include "npudefine.hpp"
#include "mmu2ru.hpp"
#include "ru2tcm.hpp"
#include "ru2mlsu.hpp"
#include "sfr/common_registers.hpp"     // brings COMMON_REGISTERS + sfr_PTR
#include "tb_config.hpp"
#include <queue>

class tb_ru_funccore : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(tb_ru_funccore);
    explicit tb_ru_funccore(sc_core::sc_module_name);

    /* DUT‑side ports */
    sc_in <bool>              clk;
    sc_in <bool>              reset;
    sc_fifo_out<npuc2mmu_PTR> o_npuc2mmu;
    sc_fifo_out<mmu2npuc_PTR> o_mmu2npuc;
    sc_vector< sc_fifo_out<mmu2ru_PTR> >  o_mmu2ru;
    sc_vector< sc_fifo_in <ru2tcm_PTR> >  i_ru2tcm;
    sc_vector< sc_fifo_in <ru2mlsu_PTR> > i_ru2mlsu;
    sc_fifo_out<sfr_PTR>      o_reg_map;

private:
    void handler();   /* drives testcase */
    void resp_tcm();  /* compares RU→TCM */
    void resp_mlsu(); /* compares RU→MLSU */

    void run_tc(const COMMON_REGISTERS& cfg);
    mmu2ru_PTR make_pkt(bool last);

    std::queue<uint16_t> gold_tcm_;
    std::size_t          gold_mlsu_{0};
};

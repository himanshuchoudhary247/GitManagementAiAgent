#pragma once
/*********************************************************************
*  tb_ru_funccore.hpp  –  RU testbench interface
*********************************************************************/
#include "npucommon.hpp"
#include "npudefine.hpp"
#include "npuc2mmu.hpp"
#include "mmu2npuc.hpp"
#include "mmu2ru.hpp"
#include "ru2tcm.hpp"
#include "ru2mlsu.hpp"
#include "sfr/unique_registers.h"   // common_register_map, sfr_PTR
#include "tb_config.hpp"            // tb_config::instance()

#include <queue>
#include <utility>

#define NUM_PORTS 16
#define NUM_LINES 4

class tb_ru_funccore : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(tb_ru_funccore);
    explicit tb_ru_funccore(sc_core::sc_module_name name);

    sc_in<bool>                          clk;
    sc_in<bool>                          reset;

    sc_fifo_out<npuc2mmu_PTR>            o_npuc2mmu;
    sc_fifo_out<mmu2npuc_PTR>            o_mmu2npuc;

    sc_vector< sc_fifo_out<mmu2ru_PTR> > o_mmu2ru;     // 16 lanes → RU
    sc_vector< sc_fifo_in <ru2tcm_PTR> > i_ru2tcm;     //  4 lanes ← RU
    sc_vector< sc_fifo_in <ru2mlsu_PTR> >i_ru2mlsu;    //  4 lanes ← RU

    sc_fifo_out<sfr_PTR>                 o_reg_map;

    void set_id(int id);
    void set_args(int argc, char* argv[]);

private:
    void driver_thread();
    void monitor_tcm();
    void monitor_mlsu();

    mmu2ru_PTR make_pkt(uint32_t row, uint32_t col, bool last);
    sfr_PTR    make_sfr(bool fused);
    void       build_golden(const common_register_map& cfg);

    std::queue< std::pair<uint64_t,bool> > golden_;  // <addr, fused?>
    int id_{0}, argc_{0};
    char** argv_{nullptr};
};

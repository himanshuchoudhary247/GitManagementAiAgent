#pragma once
/*********************************************************************
*  tb_ru_funccore.hpp  –  Routing‑Unit test‑bench (full version)
*********************************************************************/
#include "npucommon.hpp"
#include "npudefine.hpp"
#include "sfr/unique_registers.h"   // common_register_map
#include "tb_config.hpp"            // singleton loader (header‑only)

#include <queue>
#include <tuple>

/*----------------------------------------------------------------------*/
#define NUM_PORTS 16
#define NUM_LINES 4
#define DBG_TB    1

class tb_ru_funccore : public sc_core::sc_module
{
public:
    SC_HAS_PROCESS(tb_ru_funccore);
    explicit tb_ru_funccore(sc_core::sc_module_name name);

    /* Clock / reset */
    sc_in<bool> clk;
    sc_in<bool> reset;

    /* Channel FIFOs (types exist in common/ports/) */
    sc_fifo_out<npuc2mmu_PTR>               o_npuc2mmu;   // (optional trigger)
    sc_fifo_out<mmu2npuc_PTR>               o_mmu2npuc;   // (unused)

    sc_vector< sc_fifo_out<mmu2ru_PTR> >    o_mmu2ru;     // 16 → RU
    sc_vector< sc_fifo_in <ru2tcm_PTR> >    i_ru2tcm;     // 4  ← RU
    sc_vector< sc_fifo_in <ru2mlsu_PTR> >   i_ru2mlsu;    // 4  ← RU

    sc_fifo_out<sfr_PTR>                    o_reg_map;    // register map

    /* Helpers */
    void set_id  (int id)                 { id_ = id; }
    void set_args(int argc, char* argv[]) { argc_ = argc; argv_ = argv; }

private:
    /* Threads */
    void driver_thread();      // stimuli
    void monitor_tcm();        // RU→TCM
    void monitor_mlsu();       // RU→MLSU

    /* Helpers */
    void golden_fill(const common_register_map& cfg);
    mmu2ru_PTR make_pkt(uint32_t row,uint32_t col,bool last=false);
    sfr_PTR    make_sfr(bool fused);

    /* State */
    std::queue<std::tuple<uint64_t,bool>> golden_;   // <addr , fused?>
    int   id_    {0};
    int   argc_  {0};
    char** argv_ {nullptr};
};

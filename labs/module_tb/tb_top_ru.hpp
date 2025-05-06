#pragma once
#include "npucommon.hpp"
#include "npudefine.hpp"
#include "ru.hpp"
#include "tb_ru_funccore.hpp"

class tb_top_ru : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(tb_top_ru);
    tb_top_ru(sc_core::sc_module_name name, int argc, char* argv[]);

    sc_in<bool> clk;
    sc_in<bool> reset;

    ru             dut;
    tb_ru_funccore tb;

private:
    sc_fifo<npuc2mmu_PTR>   npuc2mmu_fifo{2};
    sc_fifo<mmu2npuc_PTR>   mmu2npuc_fifo{2};
    sc_fifo<mmu2ru_PTR>     mmu2ru_fifo[NUM_PORTS];
    sc_fifo<ru2tcm_PTR>     ru2tcm_fifo[NUM_LINES];
    sc_fifo<ru2mlsu_PTR>    ru2mlsu_fifo[NUM_LINES];
    sc_fifo<sfr_PTR>        reg_map_fifo{2};
};

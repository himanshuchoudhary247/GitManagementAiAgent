#pragma once
#include "npucommon.hpp"
#include "npudefine.hpp"
#define DEBUG_LOG_SEVERITY 0

class ru_funccore: public sc_core::sc_module {
public:
SC_HAS_PROCESS(ru_funccore);
ru_funccore(sc_core::sc_module_name name);
    sc_in<bool> clk;
    sc_in<bool> reset;
    sc_fifo_in<npuc2mmu_PTR > i_npuc2mmu;
    sc_fifo_in<mmu2npuc_PTR > i_mmu2npuc;
    sc_vector< sc_fifo_in mmu2ru mmu2ru PTR > > i_mmu2ru;
    sc_vector< sc_fifo_out< ru2tcm PTR > > o_ru2tcm;
    sc_vector< sc_fifo_out< ru2mlsu PTR > > o_ru2mlsu;
    sc_fifo_ink sfr_PTR > i_reg map;

void set_Id(int set_id);
private:
    int id;
};

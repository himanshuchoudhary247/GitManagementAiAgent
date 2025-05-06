#pragma once
#include "npucommon.hpp"
#include "npudefine.hpp"

// RTL‐only: main_thread is the DUT’s process.
class ru_funccore : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(ru_funccore);
    explicit ru_funccore(sc_core::sc_module_name name);

    sc_in<bool> clk, reset;
    sc_fifo_in<npuc2mmu_PTR>  i_npuc2mmu;
    sc_fifo_in<mmu2npuc_PTR>  i_mmu2npuc;
    sc_vector< sc_fifo_in<mmu2ru_PTR> >  i_mmu2ru;
    sc_vector< sc_fifo_out<ru2tcm_PTR> > o_ru2tcm;
    sc_vector< sc_fifo_out<ru2mlsu_PTR> >o_ru2mlsu;
    sc_fifo_in<sfr_PTR> i_reg_map;

    void set_Id(int);

private:
    void main_thread();
    int id{0};
};

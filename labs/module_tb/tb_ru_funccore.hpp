#pragma once
#include "npucommon.hpp"
#include "npudefine.hpp"

class tb_ru_funccore: public sc_core::sc_module {
public:
    SC_HAS_PROCESS(tb_ru_funccore);
    sc_in<bool> clk;
    tb_ru_funccore(sc_core::sc_module_name name);
    sc_fifo_out<npuc2mmu_PTR > o_npuc2mmu; 
    sc_fifo_out<mmu2npuc_PTR > o_mmu2npuc;
    sc_in<bool> reset;
    sc_vector< sc_fifo_out< mmu2ru_PTR > > o_mmu2ru;
    sc_vector< sc_fifo_ink ru2tcm PTR > > 1_ru2tcm;
    sc_vector< sc_fifo_ink ružmisu PTR > > i_ru2mlsu;
    sc_core::sc_fifo_out< sfr_PTR > o_reg_map;
    void set_id(int id);
    void set_args(int argc, char* argv[]);
private:
    int id;
    int argc_;
    char** argv_;

};
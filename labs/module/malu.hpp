/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: malu.hpp
 * Description: The top-level MALU module wrapping the functional core.
 **********/
#pragma once
#include "npucommon.hpp"
#include "npudefine.hpp"
#include "malu_funccore.hpp"

class malu: public sc_core::sc_module {
public:
    SC_HAS_PROCESS(malu);
    malu(sc_core::sc_module_name name, int id);

    sc_in<bool> reset;
    sc_fifo_in<npuc2malu_PTR>  i_npuc2malu;
    sc_fifo_out<malu2npuc_PTR> o_malu2npuc;
    sc_vector< sc_fifo_in<mrf2malu_PTR> > i_mrf2malu;
    sc_fifo_out<malu2mrf_PTR>  o_malu2mrf;
    sc_fifo_in<sfr_PTR>        i_reg_map;

    void set_id(int set_id);

private:
    malu_funccore funccore;
    int id;
};

#ifndef MALU_HPP
#define MALU_HPP

#include <systemc.h>
#include "malu_funccore.hpp"

/**
 * Top-level MALU module wraps the functional core.
 */
struct malu : sc_core::sc_module {
    sc_in<bool>            clk;
    sc_in<bool>            reset;
    sc_in< sc_uint<32> >   i_npuc2malu;
    sc_out< sc_uint<32> >  o_malu2npuc;
    sc_in< sc_bv<2048> >   i_mrf2malu[2];
    sc_out< sc_bv<2048> >  o_malu2mrf;
    sc_in< sc_uint<32> >   i_reg_map;
    sc_in< sc_bv<512> >    i_tcm2malu[4];

    malu_funccore funccore;
    int id;

    SC_HAS_PROCESS(malu);
    malu(sc_core::sc_module_name name, int set_id);
    void set_id(int new_id);
    static void instantiate_MALU();
};

#endif

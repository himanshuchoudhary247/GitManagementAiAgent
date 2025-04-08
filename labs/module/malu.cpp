/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: malu.cpp
 * Description: Instantiates the malu_funccore, hooking up the required FIFOs and signals.
 **********/
#include "malu.hpp"

malu::malu(sc_core::sc_module_name name, int id)
    : sc_module(name)
    , reset("reset")
    , i_npuc2malu("i_npuc2malu")
    , o_malu2npuc("o_malu2npuc")
    , i_mrf2malu("i_mrf2malu", 2)
    , o_malu2mrf("o_malu2mrf")
    , i_reg_map("i_reg_map")
    , funccore("funccore")
    , id(id)
{
    // Create a local clock
    funccore.clk.bind(*new sc_clock("clk", 10, SC_NS));
    funccore.reset(reset);

    funccore.i_npuc2malu(i_npuc2malu);
    funccore.o_malu2npuc(o_malu2npuc);

    for(int i=0; i<2; i++){
        funccore.i_mrf2malu[i]( i_mrf2malu[i] );
    }
    funccore.o_malu2mrf(o_malu2mrf);

    funccore.i_reg_map(i_reg_map);

    funccore.set_Id(id);
}

void malu::set_id(int set_id)
{
    id=set_id;
    funccore.set_Id(set_id);
}

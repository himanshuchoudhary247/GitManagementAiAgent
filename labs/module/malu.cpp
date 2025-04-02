#include "malu.hpp"

malu::malu(sc_core::sc_module_name name, int set_id)
    : sc_module(name)
    , clk("clk")
    , reset("reset")
    , i_npuc2malu("i_npuc2malu")
    , o_malu2npuc("o_malu2npuc")
    , i_mrf2malu("i_mrf2malu",2)
    , o_malu2mrf("o_malu2mrf")
    , i_reg_map("i_reg_map")
    , i_tcm2malu("i_tcm2malu",4)
    , funccore("funccore")
    , id(set_id)
{
    // wiring
    funccore.clk(clk);
    funccore.reset(reset);
    funccore.i_npuc2malu(i_npuc2malu);
    funccore.o_malu2npuc(o_malu2npuc);

    for(int i=0; i<2; i++){
        funccore.i_mrf2malu[i]( i_mrf2malu[i] );
    }
    funccore.o_malu2mrf(o_malu2mrf);

    funccore.i_reg_map(i_reg_map);

    for(int j=0; j<4; j++){
        funccore.i_tcm2malu[j]( i_tcm2malu[j] );
    }

    funccore.set_Id(id);
}

void malu::set_id(int new_id)
{
    id=new_id;
    funccore.set_Id(id);
}

void malu::instantiate_MALU()
{
    malu iM("malu_inst",0);
}

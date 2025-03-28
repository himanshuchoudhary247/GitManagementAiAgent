#include "malu.hpp"
#include <systemc.h>
#include <iostream>

//------------------------------------------------------------------------------
// MALU constructor: wires up ports and instantiates the functional core
//------------------------------------------------------------------------------
malu::malu(sc_core::sc_module_name name, int set_id)
    : sc_module(name)
    , clk("clk")
    , reset("reset")
    , i_npuc2malu("i_npuc2malu")
    , o_malu2npuc("o_malu2npuc")
    , i_mrf2malu("i_mrf2malu", 2)
    , o_malu2mrf("o_malu2mrf")
    , i_reg_map("i_reg_map")
    , i_tcm2malu("i_tcm2malu", 4)  // 4 lines from TCM
    , funccore("funccore")
    , id(set_id)
{
    // 1) Bind clock, reset
    funccore.clk(clk);
    funccore.reset(reset);

    // 2) Bind MRF <-> MALU
    for(int i = 0; i < 2; i++) {
        funccore.i_mrf2malu[i](i_mrf2malu[i]);
    }
    funccore.o_malu2mrf(o_malu2mrf);

    // 3) Bind NPUC <-> MALU
    funccore.i_npuc2malu(i_npuc2malu);
    funccore.o_malu2npuc(o_malu2npuc);

    // 4) Bind register map
    funccore.i_reg_map(i_reg_map);

    // 5) Bind TCM <-> MALU (4 lines)
    for(int j=0; j<4; j++) {
        funccore.i_tcm2malu[j](i_tcm2malu[j]);
    }

    // 6) Set ID
    funccore.set_Id(id);
}

void malu::set_id(int new_id)
{
    id = new_id;
    funccore.set_Id(id);
}

//------------------------------------------------------------------------------
// Example top-level instantiation function
//   * In a larger design, you might do this in a dedicated testbench .cpp file
//------------------------------------------------------------------------------
void instantiate_MALU()
{
    // Create an instance of MALU with ID = 0
    malu i_MALU("MALU_Inst", 0);

    // Typically you'd declare signals here and connect them. We'll illustrate:
    static sc_signal<bool>             rst_sig("rst_sig");
    static sc_clock                    clk_sig("clk_sig", 10, SC_NS);
    static sc_signal< sc_uint<32> >    npuc2malu_sig("npuc2malu_sig");
    static sc_signal< sc_uint<32> >    malu2npuc_sig("malu2npuc_sig");
    static sc_signal< sc_uint<32> >    mrf2malu_sig[2];
    static sc_signal< sc_uint<32> >    malu2mrf_sig("malu2mrf_sig");
    static sc_signal< sc_uint<32> >    reg_map_sig("reg_map_sig");
    static sc_signal< sc_bv<512> >     tcm2malu_sig[4];

    // Bind signals
    i_MALU.reset(rst_sig);
    i_MALU.clk(clk_sig);

    i_MALU.i_npuc2malu(npuc2malu_sig);
    i_MALU.o_malu2npuc(malu2npuc_sig);

    for (int i = 0; i < 2; i++) {
        i_MALU.i_mrf2malu[i](mrf2malu_sig[i]);
    }
    i_MALU.o_malu2mrf(malu2mrf_sig);

    i_MALU.i_reg_map(reg_map_sig);
    for (int j = 0; j < 4; j++) {
        i_MALU.i_tcm2malu[j](tcm2malu_sig[j]);
    }

    // Possibly drive some test data
    // npuc2malu_sig = 0x00000000; // e.g., opcode for add
    // mrf2malu_sig[0] = ...
    // mrf2malu_sig[1] = ...

    // sc_start(...) can happen in sc_main
}

//------------------------------------------------------------------------------
// Minimal sc_main for a stand-alone test
//   * Typically kept in a separate file, but shown here for completeness
//------------------------------------------------------------------------------
int sc_main(int argc, char* argv[])
{
    // Instantiate the MALU at top-level
    malu i_MALU("MALU_Inst", 0);

    // Create signals
    sc_signal<bool>             rst_sig("rst_sig");
    sc_clock                    clk_sig("clk_sig", 10, SC_NS);
    sc_signal< sc_uint<32> >    npuc2malu_sig("npuc2malu_sig");
    sc_signal< sc_uint<32> >    malu2npuc_sig("malu2npuc_sig");
    sc_signal< sc_uint<32> >    mrf2malu_sig[2];
    sc_signal< sc_uint<32> >    malu2mrf_sig("malu2mrf_sig");
    sc_signal< sc_uint<32> >    reg_map_sig("reg_map_sig");
    sc_signal< sc_bv<512> >     tcm2malu_sig[4];

    // Bind signals
    i_MALU.reset(rst_sig);
    i_MALU.clk(clk_sig);

    i_MALU.i_npuc2malu(npuc2malu_sig);
    i_MALU.o_malu2npuc(malu2npuc_sig);

    for (int i = 0; i < 2; i++) {
        i_MALU.i_mrf2malu[i](mrf2malu_sig[i]);
    }
    i_MALU.o_malu2mrf(malu2mrf_sig);

    i_MALU.i_reg_map(reg_map_sig);
    for (int j = 0; j < 4; j++) {
        i_MALU.i_tcm2malu[j](tcm2malu_sig[j]);
    }

    // Initialize signals
    rst_sig.write(true);
    reg_map_sig.write(0);
    npuc2malu_sig.write(0);
    for (int i = 0; i < 2; i++) {
        mrf2malu_sig[i].write(0);
    }
    for (int j = 0; j < 4; j++) {
        tcm2malu_sig[j].write(0);
    }

    // Start simulation: first 3 cycles with reset=1
    sc_start(30, SC_NS);

    rst_sig.write(false); // De-assert reset
    std::cout << "=== De-asserting reset at t=" << sc_time_stamp() << " ===" << std::endl;

    // Provide some example command
    // e.g., opcode = 0x00 => ADD, with operands 1.5 and 2.5
    union { float f; uint32_t u; } cvt;
    cvt.f = 1.5f;
    mrf2malu_sig[0].write(cvt.u);
    cvt.f = 2.5f;
    mrf2malu_sig[1].write(cvt.u);

    sc_uint<8> opcode = 0x00; // OP_ADD
    sc_uint<32> cmd = 0;
    cmd.range(7,0) = opcode;
    npuc2malu_sig.write(cmd);

    sc_start(50, SC_NS);

    // Read out the result from MALU2MRF
    sc_uint<32> result_val = malu2mrf_sig.read();
    float result_float;
    std::memcpy(&result_float, &result_val, sizeof(result_float));
    std::cout << "[TB] MALU ADD result = " << result_float
              << " at t=" << sc_time_stamp() << std::endl;

    // End simulation
    sc_stop();
    return 0;
}

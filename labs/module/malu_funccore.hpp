#ifndef MALU_FUNCCORE_HPP
#define MALU_FUNCCORE_HPP

#include <systemc.h>

/**
 * MALU functional core:
 *  - Single-cycle floating ops for 64 lanes in parallel (FP32 or BF16).
 *  - No multi-state pipeline for the operation itself; it's purely combinational.
 *  - We do use a 2-state FSM (IDLE -> EX -> back to IDLE) to capture the result in registers.
 */

// We parse 2048 bits as 64 x 32 bits
static const int VECTOR_LEN = 64;

// FSM states
enum PipeState { ST_IDLE, ST_EX };

struct malu_funccore : sc_core::sc_module {
    // Ports
    sc_in<bool>            clk;
    sc_in<bool>            reset;

    sc_in< sc_uint<32> >   i_npuc2malu;    // opcode
    sc_out< sc_uint<32> >  o_malu2npuc;    // status
    sc_in< sc_bv<2048> >   i_mrf2malu[2];  // 2 vector inputs
    sc_out< sc_bv<2048> >  o_malu2mrf;     // vector output
    sc_in< sc_uint<32> >   i_reg_map;      // bit0 => 0=BF16,1=FP32
    sc_in< sc_bv<512> >    i_tcm2malu[4];   // not used here, but in your design

    int id;

    // decode signals
    sc_signal<bool> dec_add, dec_sub;

    // precision mode
    sc_signal<bool> use_fp32;

    // FSM
    sc_signal<PipeState> state_reg;

    SC_HAS_PROCESS(malu_funccore);
    malu_funccore(sc_core::sc_module_name name);

    // threads/methods
    void pipeline_thread();
    void opcode_decode_method();
    void reg_map_monitor_method();
    void lut_load_thread();  // if needed

    void set_Id(int set_id);
};

#endif

#ifndef MALU_FUNCCORE_HPP
#define MALU_FUNCCORE_HPP

#include <systemc.h>
#include "fp32_add_unit.hpp"
#include "bf16_add_unit.hpp"

/**
 * The MALU functional core that uses:
 *  - Two submodules: fp32_add_unit, bf16_add_unit
 *  - They run multi-cycle alignment, rounding, etc.
 *  - The main MALU pipeline just orchestrates:
 *    - read opcode
 *    - if FP32 add => fire fp32_add_unit
 *    - if BF16 add => fire bf16_add_unit
 *    - wait for done
 *    - write to MRF
 */

static const int VECTOR_LEN=64;

enum PipeState {
    ST_IDLE,
    ST_DECODE,
    ST_EX_ADD,   // simplified => if "fp32" => we drive fp32_add_unit, else bf16
    ST_WAIT_ADD,
    ST_WRITEBACK
};

struct malu_funccore : sc_core::sc_module {
    // Ports
    sc_in<bool> clk;
    sc_in<bool> reset;

    sc_in< sc_uint<32> >  i_npuc2malu;   // command
    sc_out< sc_uint<32> > o_malu2npuc;   // status
    sc_in< sc_bv<2048> >  i_mrf2malu[2];
    sc_out< sc_bv<2048> > o_malu2mrf;
    sc_in< sc_uint<32> >  i_reg_map;
    sc_in< sc_bv<512> >   i_tcm2malu[4];

    int id;

    // Submodules
    fp32_add_unit* fp32_addU;
    bf16_add_unit* bf16_addU;

    // Pipeline states
    sc_signal<PipeState> state_reg;

    // decode signals (no switch)
    sc_signal<bool> dec_add; // we demonstrate just add for brevity

    // BF16 or FP32 mode
    sc_signal<bool> use_fp32;

    // handshake
    sc_signal<bool> add_start;
    sc_signal< sc_uint<32> > addA, addB;
    sc_signal<bool> add_done;
    sc_signal< sc_uint<32> > add_result;

    // latched bus input
    sc_signal< sc_bv<2048> > lat_inA, lat_inB;

    SC_HAS_PROCESS(malu_funccore);
    malu_funccore(sc_core::sc_module_name name);

    void lut_load_thread();
    void pipeline_thread();
    void opcode_decode_method();
    void reg_map_monitor_method();
    void set_Id(int set_id);
};

#endif

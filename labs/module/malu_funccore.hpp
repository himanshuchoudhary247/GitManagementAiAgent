#ifndef MALU_FUNCCORE_HPP
#define MALU_FUNCCORE_HPP

#include <systemc.h>
#include "ops.hpp"
#include "typecast_ops.hpp"

/**
 * MALU functional core:
 * - Implements single-cycle vector operations (64 lanes)
 * - Supports FP32 and BF16 operations, as well as type-casting.
 * - Uses register bits from i_reg_map to control:
 *     use_fp32, enable_except, enable_clamp, enable_trunc, enable_subnorm.
 */
static const int VECTOR_LEN = 64;

enum PipeState { ST_IDLE, ST_EX };

struct malu_funccore : sc_core::sc_module {
    sc_in<bool>            clk;
    sc_in<bool>            reset;
    sc_in< sc_uint<32> >   i_npuc2malu;
    sc_out< sc_uint<32> >  o_malu2npuc;
    sc_in< sc_bv<2048> >   i_mrf2malu[2];
    sc_out< sc_bv<2048> >  o_malu2mrf;
    sc_in< sc_uint<32> >   i_reg_map;
    sc_in< sc_bv<512> >    i_tcm2malu[4];

    int id;

    // Decode signals for operation selection
    sc_signal<bool> dec_add, dec_sub, dec_mul, dec_typecast;

    // Register-controlled flags (from CSV HR_Table):
    sc_signal<bool> use_fp32;      // 1: FP32, 0: BF16
    sc_signal<bool> enable_except; // Exception handling enabled
    sc_signal<bool> enable_clamp;  // Clamp outputs if required
    sc_signal<bool> enable_trunc;  // 1: truncate; 0: round half-up
    sc_signal<bool> enable_subnorm;// Normalize subnormals if true

    // Type-cast formats
    sc_signal<NumFormat> srcFmt, dstFmt;

    sc_signal<PipeState> state_reg;

    SC_HAS_PROCESS(malu_funccore);
    malu_funccore(sc_core::sc_module_name name);

    void pipeline_thread();
    void opcode_decode_method();
    void reg_map_monitor_method();
    void lut_load_thread();
    void set_Id(int set_id);
};

#endif

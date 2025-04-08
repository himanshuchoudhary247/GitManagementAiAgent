/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: malu_funccore.hpp
 * Description: Declares the MALU functional core. It has three threads:
 *              1) pipeline_thread (executes the math ops),
 *              2) sfr_decoder (reads i_reg_map FIFO and decodes SFRs),
 *              3) lut_load_thread (dummy thread for future LUT usage).
 **********/
#pragma once
#include <systemc.h>
#include "npucommon.hpp"
#include "npudefine.hpp"
#include "npu2malu.hpp"
#include "malu2npuc.hpp"
#include "mrf2malu.hpp"
#include "malu2mrf.hpp"
#include "ops.hpp"
#include "typecast_ops.hpp"

/**
 * SFR decode struct: holds config from addresses like 0x64, 0x68, 0x6A, 0x80, 0xA0
 */
struct decoded_sfr_t {
    // Fields from 0x64
    sc_uint<1>  store_output;
    sc_uint<4>  reg_index_output;
    sc_uint<1>  load_input_1;
    sc_uint<4>  reg_index_input_1;
    sc_uint<1>  load_input_0;
    sc_uint<4>  reg_index_input_0;
    sc_uint<1>  load_lut_enable;
    sc_uint<6>  lut_size;
    sc_uint<15> lut_base_addr;

    // Fields from 0x68
    sc_uint<8>  scalar_index_output;
    sc_uint<8>  scalar_index_input_1;

    // Field from 0x6A
    sc_uint<32> immediate_value;

    // Field from 0x80
    sc_uint<1>  tensor_fused_op;
    sc_uint<3>  tensor_num_fmt;

    // Fields from 0xA0 (MODE_MATH)
    sc_uint<4>  operation;       // 0..12 => add, sub, mul, ...
    sc_uint<3>  input_format;    // 0 => FP32, 1 => BF16, 2 => INT8?
    sc_uint<3>  output_format;   // 0 => FP32, 1 => BF16, 2 => INT8?
    sc_uint<2>  operand_type;
    sc_uint<1>  fused_op;
    sc_uint<3>  rounding_mode;
    sc_uint<1>  saturation_enable;

    void printHumanReadable() const;
};

class malu_funccore: public sc_core::sc_module {
public:
    SC_HAS_PROCESS(malu_funccore);
    malu_funccore(sc_core::sc_module_name name);

    // Ports
    sc_in<bool> clk;
    sc_in<bool> reset;

    // FIFO ports for instructions, results, MRF data, etc.
    sc_fifo_in<npuc2malu_PTR>  i_npuc2malu;
    sc_fifo_out<malu2npuc_PTR> o_malu2npuc;
    sc_vector< sc_fifo_in<mrf2malu_PTR> > i_mrf2malu;
    sc_fifo_out<malu2mrf_PTR>  o_malu2mrf;
    sc_fifo_in<sfr_PTR>        i_reg_map;

    void set_Id(int set_id);

private:
    int id;
    decoded_sfr_t sfr_config; // store the SFR config

    // Threads
    void pipeline_thread();   // does the math ops
    void sfr_decoder();       // reads i_reg_map and sets sfr_config
    void lut_load_thread();   // dummy thread for future LUT usage
};

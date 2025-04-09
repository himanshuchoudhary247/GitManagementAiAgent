/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: malu_funccore.hpp
 * Description: Declares the MALU functional core module. Contains threads:
 *              1) pipeline_thread for math ops
 *              2) sfr_decoder for reading _COMMON_REGISTERS
 *              3) lut_load_thread as a dummy for future LUT usage
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

// new includes for the addresses + struct
#include "common_register_addr.hpp"
#include "common_register.hpp"

// A local struct storing the simplified fields we need
struct decoded_sfr_t {
    // from load_store
    sc_uint<1>  store_output;
    sc_uint<4>  reg_index_output;
    sc_uint<1>  load_input_1;
    sc_uint<4>  reg_index_input_1;
    sc_uint<1>  load_input_0;
    sc_uint<4>  reg_index_input_0;
    // from LUT
    sc_uint<1>  load_lut_enable;
    sc_uint<6>  lut_size;
    sc_uint<15> lut_base_addr;
    // from SCALAR
    sc_uint<8>  scalar_index_output;
    sc_uint<8>  scalar_index_input_1;
    // from IMMEDIATE
    sc_uint<32> immediate_value;
    // from MODE_MATH
    sc_uint<4>  operation;
    sc_uint<3>  input_format;
    sc_uint<3>  output_format;
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

    sc_in<bool> clk;
    sc_in<bool> reset;

    sc_fifo_in<npuc2malu_PTR>  i_npuc2malu;
    sc_fifo_out<malu2npuc_PTR> o_malu2npuc;
    sc_vector< sc_fifo_in<mrf2malu_PTR> > i_mrf2malu;
    sc_fifo_out<malu2mrf_PTR>  o_malu2mrf;

    // Now we read a pointer to _COMMON_REGISTERS from i_reg_map
    sc_fifo_in< sfr_PTR > i_reg_map;

    void set_Id(int set_id);

private:
    int id;
    decoded_sfr_t sfr_config; // local simplified copy

    // threads
    void pipeline_thread();
    void sfr_decoder();
    void lut_load_thread();
};

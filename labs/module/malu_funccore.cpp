/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: malu_funccore.cpp
 * Description: Implements the MALU functional core. It has three threads:
 *              1) pipeline_thread => single-cycle 64-lane math ops
 *              2) sfr_decoder => reads from i_reg_map (COMMON_REGISTERS) subfields
 *              3) lut_load_thread => dummy
 **********/
#include "malu_funccore.hpp"
#include <iostream>

// Debug printing for decoded SFR
void decoded_sfr_t::printHumanReadable() const
{
    std::cout << "[decoded_sfr_t]\n"
              << "  store_output=" << store_output << "\n"
              << "  reg_index_output=" << reg_index_output << "\n"
              << "  load_input_1=" << load_input_1 << "\n"
              << "  reg_index_input_1=" << reg_index_input_1 << "\n"
              << "  load_input_0=" << load_input_0 << "\n"
              << "  reg_index_input_0=" << reg_index_input_0 << "\n"
              << "  load_lut_enable=" << load_lut_enable << "\n"
              << "  lut_size=" << lut_size << "\n"
              << "  lut_base_addr=" << lut_base_addr << "\n"
              << "  scalar_index_output=" << scalar_index_output << "\n"
              << "  scalar_index_input_1=" << scalar_index_input_1 << "\n"
              << "  immediate_value=0x" << std::hex << immediate_value.to_uint() << std::dec << "\n"
              << "  operation=" << operation << "\n"
              << "  input_format=" << input_format << "\n"
              << "  output_format=" << output_format << "\n"
              << "  operand_type=" << operand_type << "\n"
              << "  fused_op=" << fused_op << "\n"
              << "  rounding_mode=" << rounding_mode << "\n"
              << "  saturation_enable=" << saturation_enable << std::endl;
}

// Constructor
malu_funccore::malu_funccore(sc_core::sc_module_name name)
  : sc_module(name)
  , clk("clk")
  , reset("reset")
  , i_npuc2malu("i_npuc2malu")
  , o_malu2npuc("o_malu2npuc")
  , i_mrf2malu("i_mrf2malu", 2)
  , o_malu2mrf("o_malu2mrf")
  , i_reg_map("i_reg_map")
  , id(-1)
{
    SC_CTHREAD(pipeline_thread, clk.pos());
    reset_signal_is(reset,true);

    SC_CTHREAD(sfr_decoder, clk.pos());
    reset_signal_is(reset,true);

    SC_CTHREAD(lut_load_thread, clk.pos());
    reset_signal_is(reset,true);
}

void malu_funccore::set_Id(int set_id)
{
    id= set_id;
}

// The dummy LUT load thread
void malu_funccore::lut_load_thread()
{
    wait();
    while(true){
        // For future LUT usage
        wait();
    }
}

// The SFR decoder
// We read from i_reg_map => a shared_ptr<_COMMON_REGISTERS> (sfr_PTR).
// Then we copy relevant subfields into sfr_config.
void malu_funccore::sfr_decoder()
{
    while(true){
        wait();

        // If there's an SFR struct available, parse it:
        if(i_reg_map.num_available() > 0) {
            auto sfr_ptr = i_reg_map.read();

            // We'll do a switch on *addresses* if you wish. 
            // But user code says each sub-struct in _COMMON_REGISTERS 
            // is already splitted. So let's do direct copying:

            // 1) For 0x64 => reg_parsed_option_math_load_store
            sfr_config.store_output      = sfr_ptr->reg_parsed_option_math_load_store.store_output;
            sfr_config.reg_index_output  = sfr_ptr->reg_parsed_option_math_load_store.reg_index_output;
            sfr_config.load_input_1      = sfr_ptr->reg_parsed_option_math_load_store.load_input_1;
            sfr_config.reg_index_input_1 = sfr_ptr->reg_parsed_option_math_load_store.reg_index_input_1;
            sfr_config.load_input_0      = sfr_ptr->reg_parsed_option_math_load_store.load_input_0;
            sfr_config.reg_index_input_0 = sfr_ptr->reg_parsed_option_math_load_store.reg_index_input_0;

            // 2) Also from 0x64 => reg_parsed_option_math_lut
            sfr_config.load_lut_enable   = sfr_ptr->reg_parsed_option_math_lut.load_lut_enable;
            sfr_config.lut_size          = sfr_ptr->reg_parsed_option_math_lut.lut_size;
            sfr_config.lut_base_addr     = sfr_ptr->reg_parsed_option_math_lut.lut_base_addr;

            // 3) 0x68 => reg_parsed_option_math_scalar
            sfr_config.scalar_index_output  = sfr_ptr->reg_parsed_option_math_scalar.scalar_index_output;
            sfr_config.scalar_index_input_1 = sfr_ptr->reg_parsed_option_math_scalar.scalar_index_input_1;

            // 4) 0x6A => reg_parsed_option_math_immediate
            sfr_config.immediate_value = sfr_ptr->reg_parsed_option_math_immediate.immediate_value;

            // 5) 0xA0 => reg_parsed_mode_math
            sfr_config.operation         = sfr_ptr->reg_parsed_mode_math.operation;
            sfr_config.input_format      = sfr_ptr->reg_parsed_mode_math.input_format;
            sfr_config.output_format     = sfr_ptr->reg_parsed_mode_math.output_format;
            sfr_config.operand_type      = sfr_ptr->reg_parsed_mode_math.operand_type;
            sfr_config.fused_op          = sfr_ptr->reg_parsed_mode_math.fused_op;
            sfr_config.rounding_mode     = sfr_ptr->reg_parsed_mode_math.rounding_mode;
            sfr_config.saturation_enable = sfr_ptr->reg_parsed_mode_math.saturation_enable;

#if DEBUG_LOG_SEVERITY>0
            std::cout << "[SFR Decoder] Copied sub-struct fields into sfr_config.\n";
            sfr_config.printHumanReadable();
#endif
            // Possibly set the global ops context from the mode fields
            bool excpt   = (sfr_config.operation[0]==1);
            bool clamp   = (sfr_config.saturation_enable==1);
            bool trunc   = (sfr_config.rounding_mode[0]==1);
            bool subnorm = true;
            setOpsContext(subnorm, trunc, clamp, excpt);
        }
    }
}

// The pipeline thread
// Reads instructions from i_npuc2malu and two lines from MRF, 
// uses sfr_config.operation to pick the math op, 
// does a 64-lane single-cycle pass, writes out results.
void malu_funccore::pipeline_thread()
{
    wait();
    while(true){
        if(i_npuc2malu.num_available()>0 &&
           i_mrf2malu[0].num_available()>0 &&
           i_mrf2malu[1].num_available()>0)
        {
            auto cmd_ptr = i_npuc2malu.read();

            if(cmd_ptr->start==1) {
                // read MRF lines
                auto inA_ptr= i_mrf2malu[0].read();
                auto inB_ptr= i_mrf2malu[1].read();

                sc_bv<2048> outBits=0;
                sc_bv<2048> aBits = inA_ptr->data;
                sc_bv<2048> bBits = inB_ptr->data;

                // interpret input_format => 0=FP32, 1=BF16, 2=INT8?
                bool use_fp32= (sfr_config.input_format==0);
                bool use_bf16= (sfr_config.input_format==1);

                for(int lane=0; lane<64; lane++){
                    sc_uint<32> valA=0, valB=0;

                    // Safely cast bits from sc_bv to bool before assigning to valA
                    for(int bit=0; bit<32; bit++){
                        bool aBitVal = (bool)aBits[lane*32 + bit];
                        bool bBitVal = (bool)bBits[lane*32 + bit];
                        valA[bit]= aBitVal;
                        valB[bit]= bBitVal;
                    }

                    sc_uint<32> result=0;
                    switch(sfr_config.operation.to_uint()) {
                        case 0: // add
                            if(use_fp32) result= fp32_add_1c(valA,valB);
                            else         result= bf16_add_1c(valA,valB);
                            break;
                        case 1: // sub
                            if(use_fp32) result= fp32_sub_1c(valA,valB);
                            else         result= bf16_sub_1c(valA,valB);
                            break;
                        case 2: // mul
                            if(use_fp32) result= fp32_mul_1c(valA,valB);
                            else         result= bf16_mul_1c(valA,valB);
                            break;
                        case 3: // max => dummy
                            // naive: compare bits
                            result= (valA>valB)? valA: valB;
                            break;
                        case 4: // sum => dummy => add
                            if(use_fp32) result= fp32_add_1c(valA,valB);
                            else         result= bf16_add_1c(valA,valB);
                            break;
                        case 5: // reciprocal => placeholder
                            result=0;
                            break;
                        case 6: // inv sqrt even => placeholder
                            result=0;
                            break;
                        case 7: // inv sqrt odd => placeholder
                            result=0;
                            break;
                        case 8: // log => placeholder
                            result=0;
                            break;
                        case 9: // exp => placeholder
                            result=0;
                            break;
                        case 10:// sin => placeholder
                            result=0;
                            break;
                        case 11:// cos => placeholder
                            result=0;
                            break;
                        case 12:// type cast
                        {
                            NumFormat sF= (use_fp32? FP32 : (use_bf16? BF16 : INT8));
                            NumFormat dF= (sfr_config.output_format==0)? FP32 :
                                          ((sfr_config.output_format==1)? BF16 : INT8);
                            result= typecast_single_cycle(valA, sF, dF);
                        }
                            break;
                        default:
                            result=0;
                            break;
                    }

                    // store bits => outBits
                    for(int bit=0; bit<32; bit++){
                        outBits[lane*32 + bit] = (bool) result[bit];
                    }
                }

                auto out_mrf= std::make_shared<malu2mrf>();
                out_mrf->data= outBits;
                out_mrf->done=1;
                o_malu2mrf.write(out_mrf);

                auto out_npu= std::make_shared<malu2npuc>();
                out_npu->done=1;
                o_malu2npuc.write(out_npu);
            }
        }
        wait();
    }
}

/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: malu_funccore.cpp
 * Description: Implements the MALU functional core. Has three threads:
 *              1) pipeline_thread (64-lane math ops),
 *              2) sfr_decoder (reads i_reg_map to decode SFR),
 *              3) lut_load_thread (dummy for future use).
 **********/
#include "malu_funccore.hpp"
#include <iostream>

//--------------------------------------
// Print function for debugging SFR
//--------------------------------------
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
              << "  tensor_fused_op=" << tensor_fused_op << "\n"
              << "  tensor_num_fmt=" << tensor_num_fmt << "\n"
              << "  operation=" << operation << "\n"
              << "  input_format=" << input_format << "\n"
              << "  output_format=" << output_format << "\n"
              << "  operand_type=" << operand_type << "\n"
              << "  fused_op=" << fused_op << "\n"
              << "  rounding_mode=" << rounding_mode << "\n"
              << "  saturation_enable=" << saturation_enable << std::endl;
}

//--------------------------------------
// Constructor
//--------------------------------------
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
    // pipeline thread
    SC_CTHREAD(pipeline_thread, clk.pos());
    reset_signal_is(reset,true);

    // SFR decode thread
    SC_CTHREAD(sfr_decoder, clk.pos());
    reset_signal_is(reset,true);

    // Dummy LUT thread
    SC_CTHREAD(lut_load_thread, clk.pos());
    reset_signal_is(reset,true);
}

//--------------------------------------
// set_Id: store ID
//--------------------------------------
void malu_funccore::set_Id(int set_id)
{
    id= set_id;
}

//--------------------------------------
// The dummy LUT load thread
// Waits forever for future usage
//--------------------------------------
void malu_funccore::lut_load_thread()
{
    wait();
    while(true) {
        // For future LUT-based interpolation
        wait();
    }
}

//--------------------------------------
// sfr_decoder: reads from i_reg_map FIFO,
// decodes addresses into sfr_config, prints
// for debugging, and calls setOpsContext(...) 
// for the math ops global context if needed.
//--------------------------------------
void malu_funccore::sfr_decoder()
{
    while(true){
        wait(); // each clock

        if(i_reg_map.num_available()>0) {
            auto sfr_ptr = i_reg_map.read();
            sc_uint<32> word = sfr_ptr->data;

            switch(sfr_ptr->addr) {
                case 0x64:
                    sfr_config.store_output      = word[23];
                    sfr_config.reg_index_output  = word.range(19,16);
                    sfr_config.load_input_1      = word[15];
                    sfr_config.reg_index_input_1 = word.range(11,8);
                    sfr_config.load_input_0      = word[7];
                    sfr_config.reg_index_input_0 = word.range(3,0);
                    sfr_config.load_lut_enable   = word[31];
                    sfr_config.lut_size          = word.range(29,24);
                    sfr_config.lut_base_addr     = word.range(15,1);
                    break;
                case 0x68:
                    sfr_config.scalar_index_output  = word.range(15,8);
                    sfr_config.scalar_index_input_1 = word.range(7,0);
                    break;
                case 0x6A:
                    sfr_config.immediate_value = word;
                    break;
                case 0x80:
                    sfr_config.tensor_fused_op = word[20];
                    sfr_config.tensor_num_fmt  = word.range(2,0);
                    break;
                case 0xA0:
                    sfr_config.operation         = word.range(3,0);
                    sfr_config.input_format      = word.range(6,4);
                    sfr_config.output_format     = word.range(10,8);
                    sfr_config.operand_type      = word.range(17,16);
                    sfr_config.fused_op          = word[20];
                    sfr_config.rounding_mode     = word.range(26,24);
                    sfr_config.saturation_enable = word[28];
                    break;
                default:
                    std::cerr << "[SFR] Unknown addr 0x" << std::hex << sfr_ptr->addr 
                              << " data=0x" << word << std::dec << std::endl;
                    break;
            }

#if DEBUG_LOG_SEVERITY > 0
            std::cout << "[SFR] Decoded addr 0x" << std::hex << sfr_ptr->addr 
                      << ", data=0x" << word << std::dec << std::endl;
            sfr_config.printHumanReadable();
#endif
            // Potentially set subnorm/trunc/clamp/except bits
            bool excpt  = (sfr_config.operation[0]==1);
            bool clamp  = (sfr_config.saturation_enable==1);
            bool trunc  = (sfr_config.rounding_mode[0]==1);
            bool subnorm= true; // always handle subnormal for demonstration
            setOpsContext(subnorm, trunc, clamp, excpt);
        }
    }
}

//--------------------------------------
// pipeline_thread: main math pipeline
// read an instruction + MRF data => 
// pick operation from sfr_config.operation => do 64-lane single-cycle => output
//--------------------------------------
void malu_funccore::pipeline_thread()
{
    wait(); // after reset
    while(true) {
        if(i_npuc2malu.num_available()>0 &&
           i_mrf2malu[0].num_available()>0 &&
           i_mrf2malu[1].num_available()>0)
        {
            // Read an NPU->MALU command
            auto cmd_ptr = i_npuc2malu.read();
            if(cmd_ptr->start==1) {
                // read MRF lines
                auto inA_ptr= i_mrf2malu[0].read();
                auto inB_ptr= i_mrf2malu[1].read();

                sc_bv<2048> outBits=0;
                sc_bv<2048> aBits= inA_ptr->data;
                sc_bv<2048> bBits= inB_ptr->data;

                // interpret input format
                bool use_fp32= (sfr_config.input_format==0);
                bool use_bf16= (sfr_config.input_format==1);

                // 64-lane loop
                for(int lane=0; lane<64; lane++){
                    sc_uint<32> valA=0, valB=0;
                    for(int bit=0; bit<32; bit++){
                        valA[bit]= aBits[lane*32+ bit];
                        valB[bit]= bBits[lane*32+ bit];
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
                        case 3: // max (dummy)
                        {
                            // compare bits directly or decode floats
                            result= (valA>valB)? valA: valB;
                        }
                            break;
                        case 4: // sum (dummy => add)
                            if(use_fp32) result= fp32_add_1c(valA,valB);
                            else         result= bf16_add_1c(valA,valB);
                            break;
                        case 5: // reciprocal => placeholder
                            result=0; 
                            break;
                        case 6: // inverse sqrt even => placeholder
                            result=0;
                            break;
                        case 7: // inverse sqrt odd => placeholder
                            result=0;
                            break;
                        case 8: // log => placeholder
                            result=0;
                            break;
                        case 9: // exp => placeholder
                            result=0;
                            break;
                        case 10: // sin => placeholder
                            result=0;
                            break;
                        case 11: // cos => placeholder
                            result=0;
                            break;
                        case 12: // type cast
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

                    // store in outBits
                    for(int bit=0; bit<32; bit++){
                        outBits[lane*32 + bit]= result[bit];
                    }
                }

                // write to MRF
                auto out_mrf= std::make_shared<malu2mrf>();
                out_mrf->data= outBits;
                out_mrf->done= 1;
                o_malu2mrf.write(out_mrf);

                // send done to NPU
                auto out_npu= std::make_shared<malu2npuc>();
                out_npu->done=1;
                o_malu2npuc.write(out_npu);
            }
        }
        wait();
    }
}

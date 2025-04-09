/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: main.cpp
 * Description: SystemC testbench for the MALU pipeline. This file:
 *  - Instantiates the MALU module
 *  - Sets up FIFOs for NPU <-> MALU and MRF <-> MALU communication
 *  - Writes an SFR config to execute a 64-lane FP32 ADD operation
 *  - Sends two MRF data lines: one with 1.0f, the other with 2.0f
 *  - Receives and prints the result line from the MALU
 **********/

 #include <systemc.h>
 #include <iomanip>
 #include "malu.hpp"
 #include "npu2malu.hpp"
 #include "malu2npuc.hpp"
 #include "mrf2malu.hpp"
 #include "malu2mrf.hpp"
 #include "common_register.hpp"
 #include "common_register_addr.hpp"
 
 int sc_main(int argc, char* argv[])
 {
     // -------------------------------------------------------------
     // 1. Signals and FIFOs
     // -------------------------------------------------------------
     sc_clock clk("clk", 10, SC_NS);
     sc_signal<bool> rst("rst");
 
     sc_fifo<npuc2malu_PTR>  npuc2malu_fifo("npuc2malu_fifo", 4);
     sc_fifo<malu2npuc_PTR>  malu2npuc_fifo("malu2npuc_fifo", 4);
     sc_vector<sc_fifo<mrf2malu_PTR>> mrf2malu_fifo("mrf2malu_fifo", 2);
     sc_fifo<malu2mrf_PTR>   malu2mrf_fifo("malu2mrf_fifo", 4);
     sc_fifo<sfr_PTR>        sfr_fifo("sfr_fifo", 4);
 
     // -------------------------------------------------------------
     // 2. MALU Instantiation and Binding
     // -------------------------------------------------------------
     malu top_malu("top_malu", 0);
     top_malu.clk(clk);
     top_malu.reset(rst);
     top_malu.i_npuc2malu(npuc2malu_fifo);
     top_malu.o_malu2npuc(malu2npuc_fifo);
     for (int i = 0; i < 2; ++i)
         top_malu.i_mrf2malu[i](mrf2malu_fifo[i]);
     top_malu.o_malu2mrf(malu2mrf_fifo);
     top_malu.i_reg_map(sfr_fifo);
 
     // -------------------------------------------------------------
     // 3. Reset Sequence
     // -------------------------------------------------------------
     rst.write(true);
     sc_start(40, SC_NS);  // simulate 4 cycles of reset
     rst.write(false);
     sc_start(40, SC_NS);  // allow system to settle
 
     // -------------------------------------------------------------
     // 4. Send SFR Configuration (FP32 ADD)
     // -------------------------------------------------------------
     {
         auto sfr_ptr = std::make_shared<_COMMON_REGISTERS>();
 
         // Set OPERATION = 0 (ADD), input_format = 0 (FP32), output_format = 0 (FP32)
         sfr_ptr->reg_parsed_mode_math.operation            = 0;
         sfr_ptr->reg_parsed_mode_math.input_number_format  = 0;
         sfr_ptr->reg_parsed_mode_math.output_number_format = 0;
         sfr_ptr->reg_parsed_mode_math.type_input_1         = 0;
         sfr_ptr->reg_parsed_mode_math.fused_operation      = 0;
         sfr_ptr->reg_parsed_mode_math.rounding             = 0;
         sfr_ptr->reg_parsed_mode_math.saturation           = 0;
 
         // Load/store enables (simulate realistic config)
         sfr_ptr->reg_parsed_option_math_load_store.load_input_0 = 1;
         sfr_ptr->reg_parsed_option_math_load_store.load_input_1 = 1;
         sfr_ptr->reg_parsed_option_math_load_store.store_output = 1;
 
         sfr_fifo.write(sfr_ptr);
     }
 
     // -------------------------------------------------------------
     // 5. Push Instruction (start = 1)
     // -------------------------------------------------------------
     {
         auto inst_ptr = std::make_shared<npuc2malu>();
         inst_ptr->start = 1;
         npuc2malu_fifo.write(inst_ptr);
     }
 
     // -------------------------------------------------------------
     // 6. Send MRF Data Lines (64 lanes each, values: 1.0f + 2.0f)
     // -------------------------------------------------------------
     auto mrf_a = std::make_shared<mrf2malu>();
     auto mrf_b = std::make_shared<mrf2malu>();
     sc_bv<2048> lineA = 0, lineB = 0;
 
     sc_uint<32> valA = 0x3F800000;  // 1.0f in IEEE-754
     sc_uint<32> valB = 0x40000000;  // 2.0f in IEEE-754
 
     for (int lane = 0; lane < 64; ++lane) {
         for (int bit = 0; bit < 32; ++bit) {
             // Convert sc_uint_bitref to bool before assigning to sc_bv bitref
             lineA[lane * 32 + bit] = (bool)valA[bit];
             lineB[lane * 32 + bit] = (bool)valB[bit];
         }
     }
 
     mrf_a->data = lineA;
     mrf_b->data = lineB;
     mrf_a->done = 1;
     mrf_b->done = 1;
 
     mrf2malu_fifo[0].write(mrf_a);
     mrf2malu_fifo[1].write(mrf_b);
 
     // -------------------------------------------------------------
     // 7. Run Simulation to Allow Processing
     // -------------------------------------------------------------
     sc_start(200, SC_NS);
 
     // -------------------------------------------------------------
     // 8. Retrieve and Print MALU Output
     // -------------------------------------------------------------
     if (malu2mrf_fifo.num_available() > 0) {
         auto result = malu2mrf_fifo.read();
         sc_bv<2048> out = result->data;
 
         std::cout << "\n=== MALU OUTPUT ===" << std::endl;
         for (int lane = 0; lane < 64; ++lane) {
            sc_uint<32> res = 0;
            for (int bit = 0; bit < 32; ++bit)
                res[bit] = (bool)out[lane * 32 + bit];
        
            union {
                uint32_t u;
                float f;
            } fp32;
            fp32.u = res.to_uint();
        
            std::cout << "Lane " << std::setw(2) << lane
                      << " : 0x" << std::hex << std::setw(8)
                      << res.to_uint() << std::dec
                      << " = " << fp32.f << "f"
                      << std::endl;
        }
        
     } else {
         std::cout << "No result available from MALU output." << std::endl;
     }
 
     // -------------------------------------------------------------
     // 9. End Simulation
     // -------------------------------------------------------------
     sc_stop();
     return 0;
 }
 
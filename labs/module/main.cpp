/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: main.cpp
 * Description: SystemC testbench for the MALU design. This testbench runs three separate test 
 *              cases (FP32 ADD, FP32 SUB, FP32 MUL) by setting SFR configurations, sending instructions,
 *              supplying MRF input data (64 lanes per line), and printing the outputs in hex and float.
 **********/

 #include <systemc.h>
 #include <iomanip>
 #include "malu.hpp"
 #include "npu2malu.hpp"
 #include "malu2npuc.hpp"
 #include "mrf2malu.hpp"
 #include "malu2mrf.hpp"
 #include "common_register.hpp"      // Defines _COMMON_REGISTERS and sfr_PTR
 #include "common_register_addr.hpp" // Defines register addresses
 
 // Helper union to convert uint32_t bits to float.
 union FloatConverter {
     uint32_t u;
     float f;
 };
 
 /// runTest() runs one test case:
 /// - opCode: the operation code (0 = add, 1 = sub, 2 = mul)
 /// - a, b: the 32-bit FP32 input values (in hex) for each lane (all lanes are set identically)
 /// - expected: the expected 32-bit result (in hex) for debugging
 void runTest(sc_uint<4> opCode, sc_uint<32> a, sc_uint<32> b, sc_uint<32> expected,
              const std::string& testName,
              sc_fifo<sfr_PTR>& sfr_fifo,
              sc_fifo<npuc2malu_PTR>& npuc2malu_fifo,
              sc_vector< sc_fifo<mrf2malu_PTR> >& mrf2malu_fifo,
              sc_fifo<malu2mrf_PTR>& malu2mrf_fifo)
 {
     std::cout << "\n===== Running Test: " << testName << " =====\n";
 
     // -------------------------------------------------------------
     // 1. Push SFR configuration for FP32; we set:
     //    - operation from 0 to 12 (here opCode)
     //    - input_number_format = 0 => FP32
     //    - output_number_format = 0 => FP32
     // Other fields are set to 0 (default).
     // -------------------------------------------------------------
     {
         auto sfr_ptr = std::make_shared<_COMMON_REGISTERS>();
         sfr_ptr->reg_parsed_mode_math.operation            = opCode;
         sfr_ptr->reg_parsed_mode_math.input_number_format  = 0;
         sfr_ptr->reg_parsed_mode_math.output_number_format = 0;
         sfr_ptr->reg_parsed_mode_math.type_input_1         = 0;
         sfr_ptr->reg_parsed_mode_math.fused_operation      = 0;
         sfr_ptr->reg_parsed_mode_math.rounding             = 0;
         sfr_ptr->reg_parsed_mode_math.saturation           = 0;
         // Also set some default load/store options
         sfr_ptr->reg_parsed_option_math_load_store.load_input_0 = 1;
         sfr_ptr->reg_parsed_option_math_load_store.load_input_1 = 1;
         sfr_ptr->reg_parsed_option_math_load_store.store_output = 1;
 
         sfr_fifo.write(sfr_ptr);
     }
 
     // -------------------------------------------------------------
     // 2. Push an npuc2malu instruction (start = 1)
     // -------------------------------------------------------------
     {
         auto inst_ptr = std::make_shared<npuc2malu>();
         inst_ptr->start = 1;
         npuc2malu_fifo.write(inst_ptr);
     }
 
     // -------------------------------------------------------------
     // 3. Prepare and send MRF data lines.
     // Create two 2048-bit lines representing 64 lanes.
     // For each lane, fill with the same a and b values.
     // -------------------------------------------------------------
     auto mrfA = std::make_shared<mrf2malu>();
     auto mrfB = std::make_shared<mrf2malu>();
     sc_bv<2048> lineA = 0, lineB = 0;
 
     // For every lane (0 to 63), set all 32 bits with input value a and b.
     for (int lane = 0; lane < 64; ++lane) {
         for (int bit = 0; bit < 32; ++bit) {
             // Use explicit (bool) cast for each bit to avoid assignment issues.
             lineA[lane*32 + bit] = (bool) a[bit];
             lineB[lane*32 + bit] = (bool) b[bit];
         }
     }
     mrfA->data = lineA;
     mrfB->data = lineB;
     mrfA->done = 1;
     mrfB->done = 1;
 
     mrf2malu_fifo[0].write(mrfA);
     mrf2malu_fifo[1].write(mrfB);
 
     // -------------------------------------------------------------
     // 4. Advance simulation for processing.
     // -------------------------------------------------------------
     sc_start(100, SC_NS);
 
     // -------------------------------------------------------------
     // 5. Read output from malu2mrf FIFO and display results.
     // -------------------------------------------------------------
     if (malu2mrf_fifo.num_available() > 0) {
         auto result_ptr = malu2mrf_fifo.read();
         sc_bv<2048> resLine = result_ptr->data;
 
         std::cout << "Test " << testName << " Results:\n";
         for (int lane = 0; lane < 64; ++lane) {
             sc_uint<32> resWord = 0;
             for (int bit = 0; bit < 32; ++bit) {
                 resWord[bit] = (bool) resLine[lane*32 + bit];
             }
 
             union {
                 uint32_t u;
                 float f;
             } conv;
             conv.u = resWord.to_uint();
 
             std::cout << "Lane " << std::setw(2) << lane << " : 0x"
                       << std::hex << std::setw(8) << resWord.to_uint() << std::dec
                       << " => " << conv.f << "f";
             if(resWord == expected)
                 std::cout << "  [PASS]";
             else
                 std::cout << "  [FAIL]";
             std::cout << std::endl;
         }
     }
     else {
         std::cout << "No result available for test " << testName << "!\n";
     }
 
     // Wait a little before next test.
     sc_start(50, SC_NS);
 }
 
 int sc_main(int argc, char* argv[])
 {
     // -------------------------------------------------------------
     // 1. Setup: Clock, Reset, and FIFOs.
     // -------------------------------------------------------------
     sc_clock clk("clk", 10, SC_NS);
     sc_signal<bool> rst("rst");
 
     // FIFOs for communication
     sc_fifo<npuc2malu_PTR>  fifo_npuc2malu("fifo_npuc2malu", 8);
     sc_fifo<malu2npuc_PTR>  fifo_malu2npuc("fifo_malu2npuc", 8);
     sc_vector<sc_fifo<mrf2malu_PTR>> fifo_mrf2malu("fifo_mrf2malu", 2);
     sc_fifo<malu2mrf_PTR>   fifo_malu2mrf("fifo_malu2mrf", 8);
     sc_fifo<sfr_PTR>        fifo_sfr("fifo_sfr", 8);
 
     // -------------------------------------------------------------
     // 2. Instantiate the MALU device.
     // -------------------------------------------------------------
     malu dut("dut_malu", 0);
     dut.clk(clk);
     dut.reset(rst);
     dut.i_npuc2malu(fifo_npuc2malu);
     dut.o_malu2npuc(fifo_malu2npuc);
     for (int i = 0; i < 2; ++i)
         dut.i_mrf2malu[i](fifo_mrf2malu[i]);
     dut.o_malu2mrf(fifo_malu2mrf);
     dut.i_reg_map(fifo_sfr);
 
     // -------------------------------------------------------------
     // 3. Reset Sequence.
     // -------------------------------------------------------------
     rst.write(true);
     sc_start(40, SC_NS);
     rst.write(false);
     sc_start(40, SC_NS);
 
     // -------------------------------------------------------------
     // 4. Run Tests:
     // Test 1: FP32 ADD (1.0f + 2.0f = 3.0f)
     //    a = 0x3F800000, b = 0x40000000, expected = 0x40400000
     // -------------------------------------------------------------
     runTest(0, 0x3F800000, 0x40000000, 0x40400000, "FP32 ADD", fifo_sfr, fifo_npuc2malu, fifo_mrf2malu, fifo_malu2mrf);
 
     // Test 2: FP32 SUB (2.0f - 1.0f = 1.0f)
     //    a = 0x40000000, b = 0x3F800000, expected = 0x3F800000
     runTest(1, 0x40000000, 0x3F800000, 0x3F800000, "FP32 SUB", fifo_sfr, fifo_npuc2malu, fifo_mrf2malu, fifo_malu2mrf);
 
     // Test 3: FP32 MUL (2.0f * 3.0f = 6.0f)
     //    a = 0x40000000, b = 0x40400000, expected = 0x40C00000 
     runTest(2, 0x40000000, 0x40400000, 0x40C00000, "FP32 MUL", fifo_sfr, fifo_npuc2malu, fifo_mrf2malu, fifo_malu2mrf);
 
     sc_start(200, SC_NS);
     sc_stop();
     return 0;
 }
 
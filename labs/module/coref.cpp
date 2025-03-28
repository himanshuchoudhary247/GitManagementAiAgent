#include "malu_funccore.hpp"
#include <cmath>
#include <cstring>
#include <algorithm>

// Example opcodes (extend as needed)
enum MALU_OPCODES {
    OP_ADD        = 0x00,
    OP_SUB        = 0x01,
    OP_MUL        = 0x02,
    OP_DIV        = 0x03,
    OP_RECIP      = 0x04,
    OP_SQRT       = 0x05,
    OP_LOG        = 0x06,
    OP_EXP        = 0x07,
    OP_REDUCE_SUM = 0x08,
    OP_REDUCE_MAX = 0x09,
    OP_CAST_FP32_TO_BF16 = 0x0A,
    OP_CAST_FP32_TO_INT8 = 0x0B
    // ... add others (sin, cos, etc.) if needed
};

//------------------------------------------------------------------------------
// Constructor
//   - Registers multiple threads/methods
//   - Initializes ports, local data, etc.
//------------------------------------------------------------------------------
malu_funccore::malu_funccore(sc_core::sc_module_name name)
  : sc_module(name)
  , clk("clk")
  , reset("reset")
  , i_npuc2malu("i_npuc2malu")
  , o_malu2npuc("o_malu2npuc")
  , i_mrf2malu("i_mrf2malu", 2)
  , o_malu2mrf("o_malu2mrf")
  , i_reg_map("i_reg_map")
  , i_tcm2malu("i_tcm2malu", 4)  // 4 lines of 512 bits each from TCM
  , id(-1)
{
    // Thread for LUT loading from TCM
    SC_CTHREAD(lut_load_thread, clk.pos());
    reset_signal_is(reset, true);

    // Main pipeline thread for arithmetic
    SC_CTHREAD(malu_pipeline_thread, clk.pos());
    reset_signal_is(reset, true);

    // Method to monitor register-map changes
    SC_METHOD(reg_map_monitor_method);
    sensitive << i_reg_map;
    dont_initialize();
}

//------------------------------------------------------------------------------
// Thread: LUT Loading from TCM
//   - Waits for data from TCM and stores in local LUT arrays
//   - In a real design, you’d parse addresses/commands, handle large blocks, etc.
//------------------------------------------------------------------------------
void malu_funccore::lut_load_thread()
{
    // Example: clear local LUT arrays on reset
    for(int i=0; i<MAX_LUT_SIZE; i++) {
        rcp_lut[i]  = 0.0f;
        sqrt_lut[i] = 0.0f;
        log_lut[i]  = 0.0f;
        exp_lut[i]  = 0.0f;
    }
    wait(); // wait for first clock after reset

    // Simple demonstration loop
    while(true) {
        // Potentially read i_tcm2malu[0..3] here each cycle
        // and store them into your LUT arrays
        // e.g., sc_bv<512> chunk0 = i_tcm2malu[0].read();
        // parse chunk0 into float coefficients
        // ...

        //What instruction i shall give to the TCM to read the data from the TCM. How to select specific LUT array to read from TCM
        // e.g., i_tcm2malu[0].read();
        // i_tcm2malu[1].read();       
        wait();
    }
}

//------------------------------------------------------------------------------

//Figureout protocol to read the data from TCM
//Figureout how to select the specific LUT array to read from TCM
//Figureout how to parse the data from TCM
//Figureout how to store the data in LUT arrays
//Figureout how to read the data from TCM
//Figureout how to select the specific LUT array to read from TCM
//Figureout how to parse the data from TCM
//Protocol to communicate between each unit and use on npu(like where is npu, is it the matrix multiplication unit?).

// Thread: MALU Pipeline
//   - Fetch command from i_npuc2malu
//   - Read operands from i_mrf2malu
//   - Decode & execute operation
//   - Write result to o_malu2mrf
//   - Signal status to o_malu2npuc
//------------------------------------------------------------------------------
void malu_funccore::malu_pipeline_thread()
{
    // Reset actions
    o_malu2npuc.write(0);
    o_malu2mrf.write(0);
    wait(); // wait one cycle after reset

    while(true) {
        // 1) Read command from NPUC
        sc_uint<32> cmd_word = i_npuc2malu.read();
        sc_uint<8> opcode = cmd_word.range(7, 0);

        // 2) Read operands from MRF
        sc_uint<32> opA = i_mrf2malu[0].read();
        sc_uint<32> opB = i_mrf2malu[1].read();

        // 3) Decode & Execute
        sc_uint<32> result = 0;
        switch(opcode) {
            case OP_ADD:        result = do_add(opA, opB);             break;
            case OP_SUB:        result = do_sub(opA, opB);             break;
            case OP_MUL:        result = do_mul(opA, opB);             break;
            case OP_DIV:        result = do_div(opA, opB);             break;
            case OP_RECIP:      result = do_recip(opA);                break;
            case OP_SQRT:       result = do_sqrt(opA);                 break;
            case OP_LOG:        result = do_log(opA);                  break;
            case OP_EXP:        result = do_exp(opA);                  break;
            case OP_REDUCE_SUM: result = do_reduce_sum(opA, opB);      break;
            case OP_REDUCE_MAX: result = do_reduce_max(opA, opB);      break;
            case OP_CAST_FP32_TO_BF16: result = do_cast_fp32_to_bf16(opA); break;
            case OP_CAST_FP32_TO_INT8: result = do_cast_fp32_to_int8(opA); break;
            default:
                // Unknown opcode
                result = 0;
                break;
        }

        // 4) Write result to MRF
        o_malu2mrf.write(result);

        // 5) Status to NPUC
        sc_uint<32> status = 0;
        status[0] = 1;  // e.g. “done” bit
        o_malu2npuc.write(status);

        wait(); // go to next clock cycle
    }
}

//------------------------------------------------------------------------------
// Method: Monitor register-map changes (configure debug, modes, ID, etc.)
//------------------------------------------------------------------------------
void malu_funccore::reg_map_monitor_method()
{
    sc_uint<32> cfg = i_reg_map.read();
    // Example: if bit[0] = 1 => debug mode enable, etc.
    bool debug_mode = (cfg[0] == 1);
    // Potentially store or use it
}

//------------------------------------------------------------------------------
// Arithmetic & Utility Functions
//   * In real hardware, these might do LUT-based polynomial expansions
//     with range reduction. Here we show direct float usage for simplicity.
//------------------------------------------------------------------------------
sc_uint<32> malu_funccore::do_add(sc_uint<32> A, sc_uint<32> B)
{
    float fA, fB, fRes;
    std::memcpy(&fA, &A, sizeof(fA));
    std::memcpy(&fB, &B, sizeof(fB));
    fRes = fA + fB;

    sc_uint<32> out;
    std::memcpy(&out, &fRes, sizeof(fRes));
    return out;
}

sc_uint<32> malu_funccore::do_sub(sc_uint<32> A, sc_uint<32> B)
{
    float fA, fB, fRes;
    std::memcpy(&fA, &A, sizeof(fA));
    std::memcpy(&fB, &B, sizeof(fB));
    fRes = fA - fB;

    sc_uint<32> out;
    std::memcpy(&out, &fRes, sizeof(fRes));
    return out;
}

sc_uint<32> malu_funccore::do_mul(sc_uint<32> A, sc_uint<32> B)
{
    float fA, fB, fRes;
    std::memcpy(&fA, &A, sizeof(fA));
    std::memcpy(&fB, &B, sizeof(fB));
    fRes = fA * fB;

    sc_uint<32> out;
    std::memcpy(&out, &fRes, sizeof(fRes));
    return out;
}

sc_uint<32> malu_funccore::do_div(sc_uint<32> A, sc_uint<32> B)
{
    float fA, fB, fRes;
    std::memcpy(&fA, &A, sizeof(fA));
    std::memcpy(&fB, &B, sizeof(fB));
    if(fB == 0.0f) {
        // handle divide-by-zero
        fRes = 0.0f;
    } else {
        fRes = fA / fB;
    }

    sc_uint<32> out;
    std::memcpy(&out, &fRes, sizeof(fRes));
    return out;
}

sc_uint<32> malu_funccore::do_recip(sc_uint<32> A)
{
    float fA;
    std::memcpy(&fA, &A, sizeof(fA));
    if(fA == 0.0f) fA = 1e-12f; // handle zero
    float fRes = 1.0f / fA;

    sc_uint<32> out;
    std::memcpy(&out, &fRes, sizeof(fRes));
    return out;
}

sc_uint<32> malu_funccore::do_sqrt(sc_uint<32> A)
{
    float fA;
    std::memcpy(&fA, &A, sizeof(fA));
    if(fA < 0.0f) fA = 0.0f; // handle negative domain
    float fRes = std::sqrt(fA);

    sc_uint<32> out;
    std::memcpy(&out, &fRes, sizeof(fRes));
    return out;
}

sc_uint<32> malu_funccore::do_log(sc_uint<32> A)
{
    float fA;
    std::memcpy(&fA, &A, sizeof(fA));
    if(fA <= 0.0f) fA = 1e-12f;
    float fRes = std::log(fA);

    sc_uint<32> out;
    std::memcpy(&out, &fRes, sizeof(fRes));
    return out;
}

sc_uint<32> malu_funccore::do_exp(sc_uint<32> A)
{
    float fA;
    std::memcpy(&fA, &A, sizeof(fA));
    float fRes = std::exp(fA);

    sc_uint<32> out;
    std::memcpy(&out, &fRes, sizeof(fRes));
    return out;
}

// Example reduce-sum of 2 elements
sc_uint<32> malu_funccore::do_reduce_sum(sc_uint<32> A, sc_uint<32> B)
{
    float fA, fB;
    std::memcpy(&fA, &A, sizeof(fA));
    std::memcpy(&fB, &B, sizeof(fB));
    float fRes = fA + fB;

    sc_uint<32> out;
    std::memcpy(&out, &fRes, sizeof(fRes));
    return out;
}

// Example reduce-max of 2 elements
sc_uint<32> malu_funccore::do_reduce_max(sc_uint<32> A, sc_uint<32> B)
{
    float fA, fB;
    std::memcpy(&fA, &A, sizeof(fA));
    std::memcpy(&fB, &B, sizeof(fB));
    float fRes = std::max(fA, fB);

    sc_uint<32> out;
    std::memcpy(&out, &fRes, sizeof(fRes));
    return out;
}

// Cast FP32 -> BF16
sc_uint<32> malu_funccore::do_cast_fp32_to_bf16(sc_uint<32> A)
{
    float fA;
    std::memcpy(&fA, &A, sizeof(fA));

    union {
        float    f;
        uint32_t u;
    } bits;
    bits.f = fA;

    // BF16 => top 16 bits of FP32
    uint32_t bf16 = (bits.u >> 16) & 0xFFFF;

    sc_uint<32> out = bf16; // stored in the lower 16 bits
    return out;
}

// Cast FP32 -> INT8
sc_uint<32> malu_funccore::do_cast_fp32_to_int8(sc_uint<32> A)
{
    float fA;
    std::memcpy(&fA, &A, sizeof(fA));
    int8_t iVal = (int8_t)std::round(fA);

    sc_uint<32> out = (uint8_t)iVal; // stored in lower 8 bits
    return out;
}

//------------------------------------------------------------------------------
// ID / configuration
//------------------------------------------------------------------------------
void malu_funccore::set_Id(int set_id)
{
    id = set_id;
}

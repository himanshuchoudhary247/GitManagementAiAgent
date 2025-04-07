#include "malu_funccore.hpp"
#include "ops.hpp"
#include "typecast_ops.hpp"

enum MALU_OPCODES {
    OP_ADD      = 0x00,
    OP_SUB      = 0x01,
    OP_MUL      = 0x02,
    OP_TYPECAST = 0x03,
    OP_NOOP     = 0xFF
};

malu_funccore::malu_funccore(sc_core::sc_module_name name)
 : sc_module(name)
 , clk("clk")
 , reset("reset")
 , i_npuc2malu("i_npuc2malu")
 , o_malu2npuc("o_malu2npuc")
 , i_mrf2malu("i_mrf2malu",2)
 , o_malu2mrf("o_malu2mrf")
 , i_reg_map("i_reg_map")
 , i_tcm2malu("i_tcm2malu",4)
 , id(-1)
{
    SC_CTHREAD(pipeline_thread, clk.pos());
    reset_signal_is(reset,true);

    SC_CTHREAD(lut_load_thread, clk.pos());
    reset_signal_is(reset,true);

    SC_METHOD(opcode_decode_method);
    sensitive << i_npuc2malu;
    dont_initialize();

    SC_METHOD(reg_map_monitor_method);
    sensitive << i_reg_map;
    dont_initialize();

    state_reg = ST_IDLE;
    dec_add = false; dec_sub = false; dec_mul = false; dec_typecast = false;
    use_fp32 = false;
    enable_except = false; enable_clamp = false; enable_trunc = false; enable_subnorm = false;
    srcFmt = FP32; dstFmt = BF16;
}

void malu_funccore::lut_load_thread() {
    wait();
    while(true) {
        // For future LUT-based interpolation (not used in current ops)
        wait();
    }
}

void malu_funccore::opcode_decode_method() {
    sc_uint<32> cmd = i_npuc2malu.read();
    sc_uint<8> op = cmd.range(7,0);
    dec_add.write(op == OP_ADD);
    dec_sub.write(op == OP_SUB);
    dec_mul.write(op == OP_MUL);
    dec_typecast.write(op == OP_TYPECAST);

    // For typecast, assume bits [9:8] = srcFmt and [11:10] = dstFmt.
    sc_uint<2> sF = cmd.range(9,8);
    sc_uint<2> dF = cmd.range(11,10);
    NumFormat sFmt = (sF == 0) ? FP32 : ((sF == 1) ? BF16 : INT8);
    NumFormat dFmt = (dF == 0) ? FP32 : ((dF == 1) ? BF16 : INT8);
    srcFmt.write(sFmt);
    dstFmt.write(dFmt);
}

void malu_funccore::reg_map_monitor_method() {
    sc_uint<32> cfg = i_reg_map.read();
    bool modefp = (cfg[0] == 1);      // Bit 0: 1 = FP32, 0 = BF16.
    bool excpt  = (cfg[1] == 1);      // Bit 1: Exception handling.
    bool clamp  = (cfg[2] == 1);      // Bit 2: Clamp outputs.
    bool trnc   = (cfg[3] == 1);      // Bit 3: Use truncate instead of round half-up.
    bool subnorm= (cfg[4] == 1);      // Bit 4: Normalize subnormals.
    use_fp32.write(modefp);
    enable_except.write(excpt);
    enable_clamp.write(clamp);
    enable_trunc.write(trnc);
    enable_subnorm.write(subnorm);
}

void malu_funccore::pipeline_thread() {
    o_malu2mrf.write(sc_bv<2048>(0));
    o_malu2npuc.write(0);
    wait();
    while(true) {
        PipeState st = state_reg.read();
        PipeState st_next = st;
        if(st == ST_IDLE) {
            sc_uint<8> op = i_npuc2malu.read().range(7,0);
            if(op != OP_NOOP)
                st_next = ST_EX;
        } else if(st == ST_EX) {
            sc_bv<2048> inA = i_mrf2malu[0].read();
            sc_bv<2048> inB = i_mrf2malu[1].read();
            sc_bv<2048> outVal = 0;
            bool fp32m = use_fp32.read();
            bool subn  = enable_subnorm.read();
            bool trnc  = enable_trunc.read();
            bool clmp  = enable_clamp.read();
            bool excp  = enable_except.read();
            NumFormat sF = srcFmt.read();
            NumFormat dF = dstFmt.read();
            bool doAdd = dec_add.read();
            bool doSub = dec_sub.read();
            bool doMul = dec_mul.read();
            bool doTcast = dec_typecast.read();
            for(int lane = 0; lane < VECTOR_LEN; lane++) {
                sc_uint<32> valA = 0, valB = 0;
                for(int b = 0; b < 32; b++){
                    valA[b] = inA[lane*32 + b];
                    valB[b] = inB[lane*32 + b];
                }
                sc_uint<32> res = 0;
                if(doAdd) {
                    res = (fp32m) ? fp32_add_1c(valA, valB, subn, trnc, clmp, excp)
                                  : bf16_add_1c(valA, valB, subn, trnc, clmp, excp);
                }
                else if(doSub) {
                    res = (fp32m) ? fp32_sub_1c(valA, valB, subn, trnc, clmp, excp)
                                  : bf16_sub_1c(valA, valB, subn, trnc, clmp, excp);
                }
                else if(doMul) {
                    res = (fp32m) ? fp32_mul_1c(valA, valB, subn, trnc, clmp, excp)
                                  : bf16_mul_1c(valA, valB, subn, trnc, clmp, excp);
                }
                else if(doTcast) {
                    res = typecast_single_cycle(valA, sF, dF, subn, trnc, clmp, excp);
                }
                for(int b = 0; b < 32; b++){
                    outVal[lane*32 + b] = res[b];
                }
            }
            o_malu2mrf.write(outVal);
            sc_uint<32> stat = 0;
            stat[0] = 1;
            o_malu2npuc.write(stat);
            st_next = ST_IDLE;
        }
        state_reg.write(st_next);
        wait();
    }
}

void malu_funccore::set_Id(int set_id) {
    id = set_id;
}

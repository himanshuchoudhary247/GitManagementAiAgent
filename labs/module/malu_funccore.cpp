#include "malu_funccore.hpp"
#include "ops.hpp" // single-cycle lane ops

enum MALU_OPCODES {
    OP_ADD  = 0x00,
    OP_SUB  = 0x01,
    OP_NOOP = 0xFF
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

    state_reg=ST_IDLE;
    dec_add=false; dec_sub=false;
    use_fp32=false;
}

void malu_funccore::lut_load_thread()
{
    // If you want to read from i_tcm2malu for LUT data, do it here
    wait();
    while(true){
        wait();
    }
}

void malu_funccore::opcode_decode_method()
{
    sc_uint<32> cmd = i_npuc2malu.read();
    sc_uint<8> op = cmd.range(7,0);

    dec_add.write( (op==OP_ADD) );
    dec_sub.write( (op==OP_SUB) );
}

void malu_funccore::reg_map_monitor_method()
{
    sc_uint<32> cfg = i_reg_map.read();
    bool modefp = (cfg[0]==1);
    use_fp32.write(modefp);
}

void malu_funccore::pipeline_thread()
{
    // On reset
    o_malu2mrf.write(sc_bv<2048>(0));
    o_malu2npuc.write(0);
    wait();

    while(true){
        PipeState st=state_reg.read();
        PipeState st_next=st;

        if(st==ST_IDLE){
            // wait for a non-FF opcode
            sc_uint<8> op = i_npuc2malu.read().range(7,0);
            if(op != OP_NOOP){
                st_next=ST_EX;
            }
        }
        else if(st==ST_EX){
            // do all 64-lane operations in 1 cycle of combinational logic
            sc_bv<2048> inA = i_mrf2malu[0].read();
            sc_bv<2048> inB = i_mrf2malu[1].read();
            sc_bv<2048> outVal=0;

            bool doAdd = dec_add.read();
            bool doSub = dec_sub.read();
            bool fp32  = use_fp32.read();

            for(int lane=0; lane<VECTOR_LEN; lane++){
                // extract 32 bits
                sc_uint<32> valA=0, valB=0;
                for(int b=0; b<32; b++){
                    valA[b] = inA[lane*32 + b];
                    valB[b] = inB[lane*32 + b];
                }

                sc_uint<32> res=0;

                // If we want no switch, do if/else decode signals
                if(doAdd){
                    if(fp32) { res = fp32_add_1cycle(valA, valB); }
                    else     { res = bf16_add_1cycle(valA, valB); }
                }
                else if(doSub){
                    if(fp32) { res = fp32_sub_1cycle(valA, valB); }
                    else     { res = bf16_sub_1cycle(valA, valB); }
                }

                // pack result
                for(int b=0; b<32; b++){
                    outVal[lane*32 + b] = res[b];
                }
            }

            // Write output
            o_malu2mrf.write(outVal);

            // Indicate done
            {
                sc_uint<32> status=0;
                status[0]=1;
                o_malu2npuc.write(status);
            }

            st_next=ST_IDLE;
        }

        state_reg.write(st_next);
        wait();
    }
}

void malu_funccore::set_Id(int set_id)
{
    id=set_id;
}

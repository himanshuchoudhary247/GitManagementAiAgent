#include "malu_funccore.hpp"

// Example opcodes
enum MALU_OPCODES {
    OP_ADD  = 0x00,
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
    // Create submodules
    fp32_addU = new fp32_add_unit("fp32_addU");
    fp32_addU->clk(clk);
    fp32_addU->reset(reset);
    fp32_addU->start(add_start);
    fp32_addU->a(addA);
    fp32_addU->b(addB);
    fp32_addU->done(add_done);
    fp32_addU->result(add_result);

    bf16_addU = new bf16_add_unit("bf16_addU");
    bf16_addU->clk(clk);
    bf16_addU->reset(reset);
    bf16_addU->start(add_start);
    bf16_addU->a(addA);
    bf16_addU->b(addB);
    bf16_addU->done(add_done);
    bf16_addU->result(add_result);

    SC_CTHREAD(lut_load_thread, clk.pos());
    reset_signal_is(reset,true);

    SC_CTHREAD(pipeline_thread, clk.pos());
    reset_signal_is(reset,true);

    SC_METHOD(opcode_decode_method);
    sensitive << i_npuc2malu;
    dont_initialize();

    SC_METHOD(reg_map_monitor_method);
    sensitive << i_reg_map;
    dont_initialize();

    state_reg=ST_IDLE;
    dec_add=false; 
    use_fp32=false; 
    add_start=false;
    addA=0; addB=0;
    add_done=false;
    add_result=0;
}

void malu_funccore::lut_load_thread()
{
    wait();
    while(true){
        // read i_tcm2malu if needed
        wait();
    }
}

void malu_funccore::opcode_decode_method()
{
    sc_uint<32> cmd=i_npuc2malu.read();
    sc_uint<8> op=cmd.range(7,0);

    dec_add.write( (op==OP_ADD) );
}

void malu_funccore::reg_map_monitor_method()
{
    sc_uint<32> cfg=i_reg_map.read();
    bool modefp=(cfg[0]==1); // bit0 => 0=BF16,1=FP32
    use_fp32.write(modefp);
}

void malu_funccore::pipeline_thread()
{
    // on reset
    o_malu2mrf.write(sc_bv<2048>(0));
    o_malu2npuc.write(0);
    state_reg.write(ST_IDLE);
    wait();

    while(true){
        PipeState st=state_reg.read();
        PipeState st_next=st;

        if(st==ST_IDLE){
            // wait for a non-FF opcode
            sc_uint<8> op=i_npuc2malu.read().range(7,0);
            if(op!=OP_NOOP){
                st_next=ST_DECODE;
            }
        }
        else if(st==ST_DECODE){
            // latch MRF input
            lat_inA.write(i_mrf2malu[0].read());
            lat_inB.write(i_mrf2malu[1].read());

            if(dec_add.read()){
                st_next=ST_EX_ADD;
            } else {
                st_next=ST_IDLE;
            }
        }
        else if(st==ST_EX_ADD){
            // parse 1 element from lat_inA, lat_inB for demonstration
            // real design might do vector approach or repeated calls
            sc_bv<2048> inA=lat_inA.read();
            sc_bv<2048> inB=lat_inB.read();

            // Example: just do element0 => top 32 bits or lower 32 bits
            // We'll pick the first 32 bits [31..0]
            sc_uint<32> aWord=0,bWord=0;
            for(int i=0; i<32; i++){
                aWord[i]=inA[i];
                bWord[i]=inB[i];
            }
            addA.write(aWord);
            addB.write(bWord);

            // set start => submodules. We pick either fp32 or bf16 submodule
            // We'll connect them both to the same signals, but only one will produce
            // correct result given the data format
            add_start.write(true);
            st_next=ST_WAIT_ADD;
        }
        else if(st==ST_WAIT_ADD){
            // watch add_done
            bool isDone=add_done.read();
            if(isDone){
                add_start.write(false);
                st_next=ST_WRITEBACK;
            }
        }
        else if(st==ST_WRITEBACK){
            // store add_result in first 32 bits of o_malu2mrf
            sc_uint<32> sum=add_result.read();
            sc_bv<2048> outv=0;
            for(int i=0; i<32; i++){
                outv[i]=sum[i];
            }
            o_malu2mrf.write(outv);

            // done bit
            {
                sc_uint<32> stat=0;
                stat[0]=1;
                o_malu2npuc.write(stat);
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

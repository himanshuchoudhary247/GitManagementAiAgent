#include <systemc.h>
#include "malu.hpp"

int sc_main(int argc, char* argv[])
{
    sc_signal<bool>          rst("rst");
    sc_clock                 clk_sig("clk_sig",10,SC_NS);
    sc_signal< sc_uint<32> > npuc2malu_sig("npuc2malu_sig");
    sc_signal< sc_uint<32> > malu2npuc_sig("malu2npuc_sig");
    sc_signal< sc_bv<2048> > mrf2malu_sig[2];
    sc_signal< sc_bv<2048> > malu2mrf_sig("malu2mrf_sig");
    sc_signal< sc_uint<32> > reg_map_sig("reg_map_sig");
    sc_signal< sc_bv<512> >  tcm2malu_sig[4];

    malu top("top_malu",0);

    top.clk(clk_sig);
    top.reset(rst);
    top.i_npuc2malu(npuc2malu_sig);
    top.o_malu2npuc(malu2npuc_sig);
    for(int i=0; i<2; i++){
        top.i_mrf2malu[i]( mrf2malu_sig[i] );
    }
    top.o_malu2mrf(malu2mrf_sig);
    top.i_reg_map(reg_map_sig);
    for(int j=0; j<4; j++){
        top.i_tcm2malu[j]( tcm2malu_sig[j] );
    }

    // init
    rst.write(true);
    reg_map_sig.write(0); // bit0=0 => BF16 mode or something
    npuc2malu_sig.write(0xFFFFFFFF); // no-op
    for(int i=0;i<2;i++){
        mrf2malu_sig[i].write(sc_bv<2048>(0));
    }
    for(int j=0;j<4;j++){
        tcm2malu_sig[j].write(sc_bv<512>(0));
    }

    sc_start(50,SC_NS);
    rst.write(false);

    // issue OP_ADD=0x00
    {
        sc_uint<32> cmd=0;
        cmd.range(7,0)=0x00; // OP_ADD
        npuc2malu_sig.write(cmd);

        // fill MRF lines => pick first 32 bits for data
        sc_bv<2048> lineA=0, lineB=0;
        // Suppose we put an FP32 or BF16 number in lower bits
        // E.g. for BF16 => top 16 bits
        // For demonstration, let’s just do naive int bits
        sc_uint<32> valA=0x3F800000; // ~1.0 in float
        sc_uint<32> valB=0x40000000; // ~2.0 in float
        for(int b=0;b<32;b++){
            lineA[b]=valA[b];
            lineB[b]=valB[b];
        }
        mrf2malu_sig[0].write(lineA);
        mrf2malu_sig[1].write(lineB);
    }

    sc_start(300,SC_NS);

    // check malu2mrf_sig => first 32 bits should have result
    sc_bv<2048> outv = malu2mrf_sig.read();
    sc_uint<32> sum=0;
    for(int b=0;b<32;b++){
        sum[b]=outv[b];
    }
    std::cout<<"[TB] Summation result = 0x"<< std::hex << sum.to_uint() 
             <<" at time="<<sc_time_stamp()<<std::endl;

    sc_stop();
    return 0;
}

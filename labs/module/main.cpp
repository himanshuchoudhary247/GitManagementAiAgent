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

    // Instantiate top MALU
    malu top("top_malu",0);

    // Connect signals
    top.clk(clk_sig);
    top.reset(rst);
    top.i_npuc2malu(npuc2malu_sig);
    top.o_malu2npuc(malu2npuc_sig);
    for(int i=0;i<2;i++){
        top.i_mrf2malu[i](mrf2malu_sig[i]);
    }
    top.o_malu2mrf(malu2mrf_sig);
    top.i_reg_map(reg_map_sig);
    for(int j=0;j<4;j++){
        top.i_tcm2malu[j](tcm2malu_sig[j]);
    }

    // Initialize
    rst.write(true);
    reg_map_sig.write(0x00000001);  // bit0=1 => FP32 mode
    npuc2malu_sig.write(0xFFFFFFFF); // no-op
    for(int i=0;i<2;i++){
        mrf2malu_sig[i].write(sc_bv<2048>(0));
    }
    for(int j=0;j<4;j++){
        tcm2malu_sig[j].write(sc_bv<512>(0));
    }

    // Start
    sc_start(50,SC_NS);
    rst.write(false);

    // Issue OP_ADD=0x00 => single-cycle for entire 64-lane vector
    {
        sc_uint<32> cmd=0;
        cmd.range(7,0)=0x00; // OP_ADD
        npuc2malu_sig.write(cmd);

        // Fill the first 32 bits of each of the 64 lanes in MRF lines
        sc_bv<2048> lineA=0, lineB=0;
        for(int lane=0; lane<64; lane++){
            sc_uint<32> valA=0x3F800000; // ~1.0 in FP32
            sc_uint<32> valB=0x40000000; // ~2.0 in FP32
            // place into bits
            for(int b=0; b<32; b++){
                lineA[lane*32 + b] = valA[b];
                lineB[lane*32 + b] = valB[b];
            }
        }
        mrf2malu_sig[0].write(lineA);
        mrf2malu_sig[1].write(lineB);
    }

    sc_start(20,SC_NS);
    // Now after 1 cycle in ST_EX, results should be in malu2mrf

    // Check results
    sc_bv<2048> outVec=malu2mrf_sig.read();
    for(int lane=0; lane<64; lane++){
        sc_uint<32> sum=0;
        for(int b=0; b<32; b++){
            sum[b]=outVec[lane*32 + b];
        }
        std::cout<<"Lane "<<lane<<" => 0x"<< std::hex << sum.to_uint() <<"\n";
    }

    sc_stop();
    return 0;
}

#include <systemc.h>
#include "malu.hpp"

int sc_main(int argc, char* argv[])
{
    sc_signal<bool>          rst("rst");
    sc_clock                 clk_sig("clk_sig", 10, SC_NS);
    sc_signal< sc_uint<32> > npuc2malu_sig("npuc2malu_sig");
    sc_signal< sc_uint<32> > malu2npuc_sig("malu2npuc_sig");
    sc_signal< sc_bv<2048> > mrf2malu_sig[2];
    sc_signal< sc_bv<2048> > malu2mrf_sig("malu2mrf_sig");
    sc_signal< sc_uint<32> > reg_map_sig("reg_map_sig");
    sc_signal< sc_bv<512> >  tcm2malu_sig[4];

    malu top("top_malu", 0);
    top.clk(clk_sig);
    top.reset(rst);
    top.i_npuc2malu(npuc2malu_sig);
    top.o_malu2npuc(malu2npuc_sig);
    for (int i = 0; i < 2; i++) {
        top.i_mrf2malu[i](mrf2malu_sig[i]);
    }
    top.o_malu2mrf(malu2mrf_sig);
    top.i_reg_map(reg_map_sig);
    for (int j = 0; j < 4; j++) {
        top.i_tcm2malu[j](tcm2malu_sig[j]);
    }

    rst.write(true);
    // Set register map bits:
    // Bit0=use_fp32=1, Bit1=enable_except=1, Bit2=enable_clamp=1,
    // Bit3=enable_trunc=0 (thus round half up), Bit4=enable_subnorm=1.
    reg_map_sig.write(0x0000001B); // binary 0001 1011 -> bit0=1, bit1=1, bit2=0, bit3=1, bit4=1; adjust as needed.
    npuc2malu_sig.write(0xFFFFFFFF);
    for (int i = 0; i < 2; i++) {
        mrf2malu_sig[i].write(sc_bv<2048>(0));
    }
    for (int j = 0; j < 4; j++) {
        tcm2malu_sig[j].write(sc_bv<512>(0));
    }

    sc_start(50, SC_NS);
    rst.write(false);

    // Issue an FP32 ADD operation (opcode 0x00)
    {
        sc_uint<32> cmd = 0;
        cmd.range(7,0) = 0x00; // OP_ADD
        // Set typecast fields if needed; here not used.
        npuc2malu_sig.write(cmd);

        sc_bv<2048> lineA = 0, lineB = 0;
        for (int lane = 0; lane < 64; lane++){
            // Using FP32 values: 0x3F800000 = 1.0, 0x40000000 = 2.0.
            sc_uint<32> fpA = 0x3F800000;
            sc_uint<32> fpB = 0x40000000;
            for (int b = 0; b < 32; b++){
                lineA[lane*32 + b] = fpA[b];
                lineB[lane*32 + b] = fpB[b];
            }
        }
        mrf2malu_sig[0].write(lineA);
        mrf2malu_sig[1].write(lineB);
    }

    sc_start(100, SC_NS);

    // Print results for each lane.
    sc_bv<2048> outData = malu2mrf_sig.read();
    for (int lane = 0; lane < 64; lane++){
        sc_uint<32> res = 0;
        for (int b = 0; b < 32; b++){
            res[b] = outData[lane*32 + b];
        }
        std::cout << "Lane " << lane << " result: 0x" << std::hex << res.to_uint() << std::endl;
    }

    sc_stop();
    return 0;
}

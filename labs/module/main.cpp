/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: main.cpp
 * Description: Example SystemC testbench for the MALU design, demonstrating
 *              pushing an SFR config, an NPU instruction, and MRF data,
 *              then retrieving the MALU output.
 **********/
#include <systemc.h>
#include "malu.hpp"
#include "npu2malu.hpp"
#include "malu2npuc.hpp"
#include "mrf2malu.hpp"
#include "malu2mrf.hpp"

/**
 * This main testbench:
 * 1) Creates FIFOs
 * 2) Instantiates the 'malu' module
 * 3) Sends an SFR command to set OPERATION=0 => add, input_format=0 => FP32
 * 4) Sends an npuc2malu command with start=1
 * 5) Sends two lines of MRF data (64-lane, each lane has FP32(1.0) or FP32(2.0))
 * 6) After the pipeline runs, we read results from o_malu2mrf
 */
int sc_main(int argc, char* argv[])
{
    // local reset
    sc_signal<bool> rst("rst");

    // Create FIFOs
    sc_fifo<npuc2malu_PTR>  npuc2malu_fifo("npuc2malu_fifo", 10);
    sc_fifo<malu2npuc_PTR>  malu2npuc_fifo("malu2npuc_fifo", 10);
    sc_vector< sc_fifo<mrf2malu_PTR> > mrf2malu_fifo("mrf2malu_fifo", 2);
    sc_fifo<malu2mrf_PTR>   malu2mrf_fifo("malu2mrf_fifo", 10);
    sc_fifo<sfr_PTR>        sfr_fifo("sfr_fifo", 10);

    // Instantiate top MALU
    malu top("top_malu", 0);
    top.reset(rst);
    top.i_npuc2malu(npuc2malu_fifo);
    top.o_malu2npuc(malu2npuc_fifo);

    for(int i=0; i<2; i++){
        top.i_mrf2malu[i]( mrf2malu_fifo[i] );
    }
    top.o_malu2mrf(malu2mrf_fifo);
    top.i_reg_map(sfr_fifo);

    // Start simulation
    rst.write(true);
    sc_start(50,SC_NS);
    rst.write(false);

    // 1) Send an SFR entry: address=0xA0 => operation=0 => add, input_format=0 => FP32, etc.
    {
        auto sfr_data = std::make_shared<sfr>();
        sfr_data->addr = 0xA0;
        // Let's define bits: operation=0 => add, input_format=0 => FP32, output_format=0 => FP32
        sfr_data->data = 0x00000000; 
        sfr_fifo.write(sfr_data);
    }

    // 2) Push an instruction to npuc2malu => start=1
    {
        auto cmd = std::make_shared<npuc2malu>();
        cmd->start=1;  // indicates do operation=0 => add
        npuc2malu_fifo.write(cmd);
    }

    // 3) Provide MRF data for 2 lines, each 64 lanes with FP32 values (1.0, 2.0)
    {
        auto mrfA = std::make_shared<mrf2malu>();
        auto mrfB = std::make_shared<mrf2malu>();
        sc_bv<2048> lineA=0, lineB=0;
        for(int lane=0; lane<64; lane++){
            sc_uint<32> valA=0x3F800000; // 1.0f in hex
            sc_uint<32> valB=0x40000000; // 2.0f in hex
            for(int bit=0; bit<32; bit++){
                lineA[lane*32 + bit] = valA[bit];
                lineB[lane*32 + bit] = valB[bit];
            }
        }
        mrfA->data= lineA;
        mrfA->done=1;
        mrfB->data= lineB;
        mrfB->done=1;
        mrf2malu_fifo[0].write(mrfA);
        mrf2malu_fifo[1].write(mrfB);
    }

    // 4) let the pipeline run
    sc_start(200, SC_NS);

    // 5) read output from malu2mrf
    while(malu2mrf_fifo.num_available()>0) {
        auto outptr = malu2mrf_fifo.read();
        sc_bv<2048> outData = outptr->data;
        for(int lane=0; lane<64; lane++){
            sc_uint<32> result=0;
            for(int bit=0; bit<32; bit++){
                result[bit]= outData[lane*32 + bit];
            }
            std::cout<<"Lane "<<lane<<" => 0x"<< std::hex << result.to_uint()<< std::dec <<std::endl;
        }
    }

    sc_stop();
    return 0;
}

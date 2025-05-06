#include "tb_top_ru.hpp"

tb_top_ru::tb_top_ru(sc_core::sc_module_name n,int argc,char*argv[])
: sc_module(n)
, clk("clk"), reset("reset")
, dut("dut",0)
, tb ("tb")
{
    // bind DUT
    dut.clk(clk);  dut.reset(reset);
    dut.i_reg_map(reg_map_fifo);
    dut.i_npuc2mmu(npuc2mmu_fifo);
    dut.i_mmu2npuc(mmu2npuc_fifo);
    for(int i=0;i<NUM_PORTS;i++) dut.i_mmu2ru[i](mmu2ru_fifo[i]);
    for(int i=0;i<NUM_LINES;i++){
        dut.o_ru2tcm[i](ru2tcm_fifo[i]);
        dut.o_ru2mlsu[i](ru2mlsu_fifo[i]);
    }

    // bind TB
    tb.clk(clk); tb.reset(reset);
    tb.o_reg_map   (reg_map_fifo);
    tb.o_npuc2mmu  (npuc2mmu_fifo);
    tb.o_mmu2npuc  (mmu2npuc_fifo);
    for(int i=0;i<NUM_PORTS;i++) tb.o_mmu2ru[i](mmu2ru_fifo[i]);
    for(int i=0;i<NUM_LINES;i++){
        tb.i_ru2tcm[i](ru2tcm_fifo[i]);
        tb.i_ru2mlsu[i](ru2mlsu_fifo[i]);
    }
}

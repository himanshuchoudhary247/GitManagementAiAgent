#include "tb_top_ru.hpp"
#include <systemc>

tb_top_ru::tb_top_ru(sc_core::sc_module_name n,int argc,char* argv[])
: sc_module(n)
, clk("clk"), reset("reset")
, dut("dut",0)
, tb ("tb")                     // 🠖 TB ctor now takes no argc/argv
, npuc2mmu_fifo("npuc2mmu_fifo",2)
, mmu2npuc_fifo("mmu2npuc_fifo",2)
, reg_map_fifo ("reg_map_fifo" ,2)
{
    init();
}

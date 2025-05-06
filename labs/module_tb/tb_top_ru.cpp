#include "tb_top_ru.hpp"

tb_top_ru::tb_top_ru(sc_core::sc_module_name n,int argc,char* argv[])
: sc_module(n)
, clk("clk"), reset("reset")
, dut("dut",0)
, tb ("tb")
, npuc2mmu_fifo("npuc2mmu_fifo",8)
, mmu2npuc_fifo("mmu2npuc_fifo",8)
, reg_map_fifo ("reg_map_fifo",1)
{
    init();
}

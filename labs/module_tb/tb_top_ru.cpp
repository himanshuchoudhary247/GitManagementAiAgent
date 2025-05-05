tb_top_ru::tb_top_ru(sc_core::sc_module_name n,int argc,char* argv[])
: sc_module(n)
, clk("clk"), reset("reset")
, dut("dut",0)
, tb ("tb")
, mmu2ru_fifo{nullptr}
, ru2tcm_fifo{nullptr}
, ru2mlsu_fifo{nullptr}
, reg_map_fifo("reg_map_fifo",4)
{
    /* allocate FIFOs */
    for(auto &f : mmu2ru_fifo)  f = new sc_fifo<mmu2ru_PTR>(4);
    for(auto &f : ru2tcm_fifo)  f = new sc_fifo<ru2tcm_PTR>(4);
    for(auto &f : ru2mlsu_fifo) f = new sc_fifo<ru2mlsu_PTR>(4);

    /* bind DUT */
    dut.clk(clk); dut.reset(reset);
    dut.i_reg_map(reg_map_fifo);
    for(int i=0;i<NUM_PORTS;i++) dut.i_mmu2ru[i] (*mmu2ru_fifo[i]);
    for(int i=0;i<NUM_LINES;i++){
        dut.o_ru2tcm[i] (*ru2tcm_fifo[i]);
        dut.o_ru2mlsu[i](*ru2mlsu_fifo[i]);
    }

    /* bind TB */
    tb.clk(clk); tb.reset(reset);
    tb.o_reg_map(reg_map_fifo);
    for(int i=0;i<NUM_PORTS;i++) tb.o_mmu2ru[i] (*mmu2ru_fifo[i]);
    for(int i=0;i<NUM_LINES;i++){
        tb.i_ru2tcm[i] (*ru2tcm_fifo[i]);
        tb.i_ru2mlsu[i](*ru2mlsu_fifo[i]);
    }
}

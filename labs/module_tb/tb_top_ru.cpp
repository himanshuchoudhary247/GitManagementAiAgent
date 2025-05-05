tb_top_ru::tb_top_ru(sc_core::sc_module_name n,int argc,char* argv[])
: sc_module(n)
, clk("clk"), reset("reset")
, dut("dut",0)
, tb ("tb")
, npuc2ru_fifo("npuc2ru_fifo",2)
, ru2npuc_fifo("ru2npuc_fifo",2)
/* ... rest identical wiring ... */
{
    /* pass ports */
    tb.clk(clk); tb.reset(reset);
    tb.o_npuc2ru(npuc2ru_fifo);
    tb.i_ru2npuc(ru2npuc_fifo);
    /* rest of init() unchanged */
}

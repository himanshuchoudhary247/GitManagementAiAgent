#include "tb_top_ru.hpp"

int sc_main(int argc,char* argv[]){
    sc_core::sc_clock clk("clk",10,SC_NS);
    sc_core::sc_signal<bool> rst;
    tb_top_ru top("top",argc,argv);
    top.clk(clk); top.reset(rst);

    rst = true; sc_core::sc_start(20,SC_NS);
    rst = false; sc_core::sc_start();

    return 0;
}

#include "tb_top_ru.hpp"
#include <systemc>
#include <cstdlib>

int sc_main(int argc,char* argv[])
{
    sc_clock        clk("clk",10,SC_NS);
    sc_signal<bool> rst("rst");

    tb_top_ru top("top",argc,argv); top.clk(clk); top.reset(rst);

    rst=true;  sc_start(40,SC_NS);  rst=false;

    double sim_us=(argc>1)?std::atof(argv[1]):50.0;
    sc_start(sim_us,SC_US);
    return 0;
}

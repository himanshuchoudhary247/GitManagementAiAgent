#include "tb_top_ru.hpp"

int sc_main(int argc, char* argv[])
{
    sc_clock        clk("clk", 10, SC_NS);
    sc_signal<bool> reset;

    tb_top_ru top("top", argc, argv);
    top.clk   (clk);
    top.reset (reset);

    // simple reset pulse
    reset.write(true);
    sc_start(20, SC_NS);
    reset.write(false);

    sc_start();  // until sc_stop()

    return 0;
}

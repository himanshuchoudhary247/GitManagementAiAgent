#include <systemc.h>
#include "tb.h"
#include <iostream>

int sc_main(int argc, char* argv[]) {
    // In this design, tb does internal binding so no external signals are needed.
    tb tb_inst("tb_inst");
    sc_start();
    std::cout << "[sc_main] Simulation finished at time " << sc_time_stamp() << std::endl;
    return 0;
}

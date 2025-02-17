#include <systemc.h>
#include "tb.h"
#include <iostream>

int sc_main(int argc, char* argv[]) {
    tb tb_inst("tb_inst");
    sc_start();
    std::cout << "[sc_main] Simulation finished at " << sc_time_stamp() << std::endl;
    return 0;
}

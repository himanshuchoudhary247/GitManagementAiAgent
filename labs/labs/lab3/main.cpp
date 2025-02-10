#include <systemc.h>
#include "tb.h"
#include <iostream>

int sc_main(int argc, char* argv[]) {
    // Instantiate the testbench.
    tb tb_inst("tb_inst");
    
    // Start simulation.
    sc_start();
    
    std::cout << "Simulation finished at time " << sc_time_stamp() << std::endl;
    return 0;
}

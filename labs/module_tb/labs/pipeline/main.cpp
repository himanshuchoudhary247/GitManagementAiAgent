#include <systemc.h>
#include "tb.h"

/**
 * sc_main: Instantiates the testbench with a queue-based pipeline
 * of capacity n=4, each cycle=10ns.
 */
int sc_main(int argc, char* argv[]) {
    tb tb_inst("tb_inst", /*capacity=*/4, /*cycle_time=*/sc_time(10, SC_NS));

    sc_start();
    std::cout << "[sc_main] Simulation ended at " << sc_time_stamp() << std::endl;
    return 0;
}

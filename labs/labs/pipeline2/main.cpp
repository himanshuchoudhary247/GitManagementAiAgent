#include <systemc.h>
#include "tb.h"

int sc_main(int argc, char* argv[]) {
    Testbench tb("tb");
    sc_start(200, SC_NS);
    return 0;
}

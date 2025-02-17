#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "npu.h"

/**
 * Testbench module for Lab 3.1.
 * Instantiates NPU and exercises its SFR read/write operations.
 * (The event_queue_demo_thread in NPU will print its triggers.)
 */
SC_MODULE(tb) {
public:
    SC_HAS_PROCESS(tb);

    tb(sc_module_name name);
    ~tb();

    void basic_test();

private:
    NPU* npu_inst;
};

#endif // TB_H

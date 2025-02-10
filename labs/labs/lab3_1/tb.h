#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "npu.h"

// Testbench module definition.
// This module instantiates NPU and defines an SC_THREAD and an SC_METHOD.
SC_MODULE(tb) {
public:
    // Constructor.
    tb(sc_core::sc_module_name name);
    // Destructor.
    ~tb();

    // Processes.
    void basic_test();             // SC_THREAD process.
    void handle_test_done_method(); // SC_METHOD process.

    // Event to signal that the test is done.
    sc_core::sc_event m_basic_test_done_event;

private:
    NPU *npu_inst; // Pointer to the NPU instance.
};

#endif // TB_H

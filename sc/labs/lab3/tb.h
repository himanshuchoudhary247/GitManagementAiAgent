#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "npu.h"

// Define a test value for writing
#define SAM_NPU_TEST_WRITE_VALUE 0xFFFFFFFF

SC_MODULE(tb) {
public:
    // Constructor
    SC_CTOR(tb);

private:
    // Instantiate NPU
    NPU* m_npu;

    // Event
    sc_event m_basic_test_done_event;

    // Processes
    void basic_test();
    void handle_test_done_method();
};

#endif // TB_H

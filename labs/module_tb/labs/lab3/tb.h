#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "npu.h"

/**
 * Testbench module for Lab 3.
 * It instantiates the NPU module, performs write/read operations on its SFRs,
 * and uses events to signal test completion.
 */
SC_MODULE(tb) {
public:
    SC_HAS_PROCESS(tb);

    tb(sc_module_name name);
    ~tb();

    // Processes
    void basic_test();              // SC_THREAD: performs read/write tests.
    void handle_test_done_method(); // SC_METHOD: sensitive to m_basic_test_done_event.

    // Event to signal that the test is done.
    sc_event m_basic_test_done_event;

private:
    NPU* npu_inst;
};

#endif // TB_H

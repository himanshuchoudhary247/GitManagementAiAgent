#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "npu.h"

// Testbench (tb) module definition for Lab 5 (SystemC 2.3.2).
// Instantiates the NPU and drives/monitors reset and interrupt ports,
// and includes an interrupt handling method that updates counters.
SC_MODULE(tb) {
public:
    tb(sc_module_name name);
    ~tb();

    // Processes.
    void basic_test();              // SC_THREAD for the basic test.
    void handle_reset();            // SC_METHOD to handle changes on the reset port.
    void handle_interrupt_method(); // SC_METHOD to handle changes on the interrupt port.

    void end_of_elaboration();

    // External ports.
    sc_out<bool> o_reset;     // Output port to drive reset to NPU.
    sc_in<int>   i_interrupt; // Input port to receive interrupt from NPU.

    // Event to signal that an interrupt has been received.
    sc_event m_interrupt_event;

    // Interrupt counters.
    int m_done_count;
    int m_error_count;

private:
    NPU *npu_inst;

    // Internal signals for port binding.
    sc_signal<bool> reset_internal;
    sc_signal<int>  intr_internal;
};

#endif // TB_H

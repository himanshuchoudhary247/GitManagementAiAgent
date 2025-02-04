#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "npu.h"

// Testbench module definition.
// Instantiates the NPU, drives/monitors reset and interrupt ports, and handles interrupts.
SC_MODULE(tb) {
public:
    tb(sc_core::sc_module_name name);
    ~tb();

    // Processes.
    void basic_test();              // SC_THREAD for the basic test.
    void handle_reset();            // SC_METHOD to handle reset changes.
    void handle_interrupt_method(); // SC_METHOD to handle interrupt changes.

    // end_of_elaboration callback.
    virtual void end_of_elaboration() override;

    // External ports.
    sc_out<bool> o_reset;     // Output port to drive reset to NPU.
    sc_in<int>   i_interrupt; // Input port to receive interrupt from NPU.

    // Event to signal that an interrupt has been received.
    sc_event m_interrupt_event;

    // Counters for interrupts.
    int m_done_count;
    int m_error_count;

private:
    NPU *npu_inst;

    // Internal signals for port binding.
    sc_signal<bool> reset_internal;
    sc_signal<int>  intr_internal;
};

#endif // TB_H

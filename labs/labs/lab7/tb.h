#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "npu.h"

// Testbench (tb) module definition for Lab 7.
// Instantiates the NPU and drives/monitors reset, interrupt, and micro‑start signals.
SC_MODULE(tb) {
public:
    SC_HAS_PROCESS(tb);

    tb(sc_module_name name);
    ~tb();

    void basic_test();              // SC_THREAD for basic test.
    void handle_reset();            // SC_METHOD for reset changes.
    void handle_interrupt_method(); // SC_METHOD for interrupt changes.
    void end_of_elaboration();

    // External ports.
    sc_out<bool> o_reset;       // Drives reset to NPU.
    sc_out<bool> o_start_micro; // Drives micro‑start to NPU.
    sc_in<int>   i_interrupt;   // Receives interrupt from NPU.

    // Event to signal that an interrupt has been received.
    sc_event m_interrupt_event;
    int m_done_count;
    int m_error_count;

private:
    NPU *npu_inst;

    // Internal signals for binding.
    sc_signal<bool> reset_internal;
    sc_signal<int>  intr_internal;
    sc_signal<bool> start_micro_internal; // For micro‑start.
};

#endif // TB_H

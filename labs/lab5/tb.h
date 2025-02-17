#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "npu.h"

/**
 * Lab 5 testbench:
 * - Has o_reset => drives reset to NPU
 * - Has i_interrupt => reads interrupt from NPU
 * - In handle_interrupt_method => increment counters for DONE or ERROR
 * - Wait on interrupt event => check counters => success/failure
 * - Clear interrupt => write to int_clear => read status => confirm bits cleared
 */
SC_MODULE(tb) {
public:
    SC_HAS_PROCESS(tb);

    tb(sc_module_name name);
    ~tb();

    // Ports
    sc_out<bool> o_reset;      // reset to NPU
    sc_in<int>   i_interrupt;  // read NPU interrupt

    // Methods
    void basic_test();
    void handle_interrupt_method();
    void end_of_elaboration();

    // For tracking interrupt counts
    int m_done_count;
    int m_error_count;

    // An event to signal that an interrupt arrived
    sc_event m_interrupt_event;

private:
    NPU* npu_inst;

    // Internal signals
    sc_signal<bool> reset_sig;
    sc_signal<int>  intr_sig;
};

#endif // TB_H

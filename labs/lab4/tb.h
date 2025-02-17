#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "npu.h"

/**
 * Lab 4 testbench:
 * - Has an output port (o_reset) to drive reset
 * - Has an input port (i_interrupt) to read NPU's interrupt
 * - Binds signals to NPU
 * - In basic_test, after writing/reading SFRs, check that i_interrupt=INT_IDLE
 *   Then trigger NPU => check i_interrupt=INT_DONE
 */
SC_MODULE(tb) {
public:
    SC_HAS_PROCESS(tb);

    tb(sc_module_name name);
    ~tb();

    // Ports
    sc_out<bool> o_reset;    // drive reset to NPU
    sc_in<int>   i_interrupt;  // read interrupt from NPU

    // Processes
    void basic_test();
    void end_of_elaboration();

private:
    NPU* npu_inst;

    // Internal signals
    sc_signal<bool> reset_sig;
    sc_signal<int>  intr_sig;

};

#endif // TB_H

#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "npu.h"

// Testbench module definition.
// Instantiates the NPU and drives/monitors reset and interrupt ports.
SC_MODULE(tb) {
public:
    // Constructor.
    tb(sc_core::sc_module_name name);
    // Destructor.
    ~tb();

    // SC_THREAD process for the basic test.
    void basic_test();

    // SC_METHOD process to handle changes on the reset port.
    void handle_reset();

    // Overridden end_of_elaboration callback: drive the reset port to a stable value.
    virtual void end_of_elaboration() override;

    // Ports to connect externally.
    // (In this design, these ports will be bound internally so that sc_main does not bind them again.)
    sc_out<bool> o_reset;     // Output port to drive reset to NPU.
    sc_in<int>   i_interrupt; // Input port to receive interrupt from NPU.

private:
    NPU *npu_inst;

    // Internal signals to bind the NPU ports to tb ports.
    sc_signal<bool> reset_internal; // Used to bind NPU.i_reset and tb.o_reset.
    sc_signal<int>  intr_internal;  // Used to bind NPU.o_interrupt and tb.i_interrupt.
};

#endif // TB_H

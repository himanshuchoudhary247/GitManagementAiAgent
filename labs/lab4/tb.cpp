#include "tb.h"
#include <iostream>

tb::tb(sc_core::sc_module_name name)
    : sc_module(name)
{
    // Instantiate the NPU module.
    npu_inst = new NPU("npu_inst");

    // Bind internal signals to connect NPU and tb ports.
    // Bind the internal reset signal to NPU's reset input.
    npu_inst->i_reset(reset_internal);
    // Bind the internal interrupt signal to NPU's interrupt output.
    npu_inst->o_interrupt(intr_internal);

    // Bind tb ports to the internal signals.
    // (Since these ports are declared in tb, we do the binding only once.)
    o_reset.bind(reset_internal);
    i_interrupt.bind(intr_internal);

    // Register the basic test SC_THREAD.
    SC_THREAD(basic_test);

    // Register an SC_METHOD to handle changes on the reset port.
    SC_METHOD(handle_reset);
    sensitive << o_reset;
    dont_initialize();
}

tb::~tb() {
    delete npu_inst;
}

void tb::basic_test() {
    // Wait to allow elaboration and initial stabilization.
    wait(20, SC_NS);

    // At the start of the basic test, check that the interrupt port has the IDLE value.
    int intr_val = i_interrupt.read();
    std::cout << "[" << name() << "] basic_test: At start, i_interrupt = " 
              << intr_val << " (expected INT_IDLE = " << NPU::INT_IDLE 
              << ") at time " << sc_time_stamp() << std::endl;
    if (intr_val != NPU::INT_IDLE) {
        std::cout << "[" << name() << "] basic_test: ERROR: i_interrupt is not INT_IDLE at start!" 
                  << std::endl;
    }

    // (Simulate additional SFR operations here if needed.)

    // Trigger the NPU operation by writing to CTRL via NPU's write method.
    // Writing '1' to CTRL (offset 0x00000000) sets the start bit.
    npu_inst->write(0x00000000, 0x1);

    // Wait to allow the NPU operation to complete.
    wait(20, SC_NS);

    // After the NPU operation, check that the interrupt port has the DONE value.
    intr_val = i_interrupt.read();
    std::cout << "[" << name() << "] basic_test: After operation, i_interrupt = " 
              << intr_val << " (expected INT_DONE = " << NPU::INT_DONE 
              << ") at time " << sc_time_stamp() << std::endl;
    if (intr_val != NPU::INT_DONE) {
        std::cout << "[" << name() << "] basic_test: ERROR: i_interrupt is not INT_DONE after operation!" 
                  << std::endl;
    }

    // End the simulation.
    sc_stop();

    // Remain in an infinite loop.
    while (true) {
        wait();
    }
}

void tb::handle_reset() {
    std::cout << "[" << name() << "] handle_reset: o_reset changed to " 
              << o_reset.read() << " at time " << sc_time_stamp() << std::endl;
    if (o_reset.read() == true) {
        std::cout << "[" << name() << "] handle_reset: Reset asserted at time " 
                  << sc_time_stamp() << std::endl;
    }
}

void tb::end_of_elaboration() {
    // Drive the reset port to a stable value (0).
    o_reset.write(false);
    std::cout << "[" << name() << "] end_of_elaboration: o_reset set to 0 at time " 
              << sc_time_stamp() << std::endl;
}

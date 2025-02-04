#include "tb.h"
#include <iostream>

tb::tb(sc_core::sc_module_name name)
    : sc_module(name), m_done_count(0), m_error_count(0)
{
    // Instantiate the NPU module.
    npu_inst = new NPU("npu_inst");

    // Bind internal signals to connect NPU and tb ports.
    npu_inst->i_reset(reset_internal);
    npu_inst->o_interrupt(intr_internal);

    // Bind tb ports to the internal signals.
    o_reset.bind(reset_internal);
    i_interrupt.bind(intr_internal);

    // Register the basic test thread.
    SC_THREAD(basic_test);

    // Register an SC_METHOD to handle reset changes.
    SC_METHOD(handle_reset);
    sensitive << o_reset;
    dont_initialize();

    // Register an SC_METHOD to handle interrupt changes.
    SC_METHOD(handle_interrupt_method);
    sensitive << i_interrupt;
    dont_initialize();
}

tb::~tb() {
    delete npu_inst;
}

void tb::basic_test() {
    // Wait for stabilization.
    wait(20, SC_NS);

    // At the start of the test, verify that the interrupt is INT_IDLE.
    int intr_val = i_interrupt.read();
    std::cout << "[" << name() << "] basic_test: At start, i_interrupt = "
              << intr_val << " (expected INT_IDLE = " << NPU::INT_IDLE
              << ") at time " << sc_time_stamp() << std::endl;
    if (intr_val != NPU::INT_IDLE) {
        std::cout << "[" << name() << "] basic_test: ERROR: i_interrupt is not INT_IDLE at start!" << std::endl;
    }

    // Trigger the NPU operation by writing '1' to CTRL (offset 0x0).
    npu_inst->write(0x00000000, 0x1);

    // Wait until the interrupt event is notified by the interrupt handler.
    wait(m_interrupt_event);

    // Print the interrupt counts.
    std::cout << "[" << name() << "] basic_test: Interrupt counts: DONE=" << m_done_count
              << ", ERROR=" << m_error_count << " at time " << sc_time_stamp() << std::endl;
    if (m_done_count != 1) {
        std::cout << "[" << name() << "] basic_test: ERROR: DONE count not as expected!" << std::endl;
    } else {
        std::cout << "[" << name() << "] basic_test: SUCCESS: DONE count as expected." << std::endl;
    }

    // Clear the interrupt by writing to int_clear SFR.
    // To clear the DONE bit, write 1 to bit 0.
    npu_inst->write(NPU::INT_CLEAR_OFFSET, 0x1);

    // Wait a short time.
    wait(10, SC_NS);

    // Read the STATUS SFR to verify that its bits are cleared.
    uint32_t status_val = 0;
    npu_inst->read(NPU::STATUS_OFFSET, status_val);
    std::cout << "[" << name() << "] basic_test: After clearing, STATUS = 0x"
              << std::hex << status_val << std::dec << " at time " << sc_time_stamp() << std::endl;
    if (status_val == 0) {
        std::cout << "[" << name() << "] basic_test: SUCCESS: STATUS cleared." << std::endl;
    } else {
        std::cout << "[" << name() << "] basic_test: FAILURE: STATUS not cleared." << std::endl;
    }

    sc_stop();
    while (true) { wait(); }
}

void tb::handle_reset() {
    std::cout << "[" << name() << "] handle_reset: o_reset changed to "
              << o_reset.read() << " at time " << sc_time_stamp() << std::endl;
    if (o_reset.read() == true) {
        std::cout << "[" << name() << "] handle_reset: Reset asserted at time "
                  << sc_time_stamp() << std::endl;
    }
}

void tb::handle_interrupt_method() {
    int intr_val = i_interrupt.read();
    std::cout << "[" << name() << "] handle_interrupt_method: i_interrupt = "
              << intr_val << " at time " << sc_time_stamp() << std::endl;
    if (intr_val == NPU::INT_DONE) {
        m_done_count++;
    } else if (intr_val == NPU::INT_ERROR) {
        m_error_count++;
    }
    // Notify the basic test thread that an interrupt has been received.
    m_interrupt_event.notify();
}

void tb::end_of_elaboration() {
    // Drive the reset port to a stable value (0).
    o_reset.write(false);
    std::cout << "[" << name() << "] end_of_elaboration: o_reset set to 0 at time "
              << sc_time_stamp() << std::endl;
}

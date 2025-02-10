#include "tb.h"
#include <iostream>

tb::tb(sc_module_name name)
    : sc_module(name), m_done_count(0), m_error_count(0)
{
    // Instantiate the NPU.
    npu_inst = new NPU("npu_inst");

    // Bind internal signals to NPU ports.
    npu_inst->i_reset(reset_internal);
    npu_inst->i_start_micro(start_micro_internal);
    npu_inst->o_interrupt(intr_internal);

    // Bind tb external ports to internal signals.
    o_reset.bind(reset_internal);
    o_start_micro.bind(start_micro_internal);
    i_interrupt.bind(intr_internal);

    // Register the basic test thread.
    SC_THREAD(basic_test);

    // Register SC_METHODs for reset and interrupt handling.
    SC_METHOD(handle_reset);
    sensitive << o_reset;
    dont_initialize();

    SC_METHOD(handle_interrupt_method);
    sensitive << i_interrupt;
    dont_initialize();
}

tb::~tb() {
    delete npu_inst;
}

void tb::basic_test() {
    wait(20, SC_NS);
    int intr_val = i_interrupt.read();
    std::cout << "[" << name() << "] basic_test: At start, i_interrupt = " 
              << intr_val << " (expected INT_IDLE = " << NPU::INT_IDLE 
              << ") at time " << sc_time_stamp() << std::endl;
    if (intr_val != NPU::INT_IDLE) {
        std::cout << "[" << name() << "] basic_test: ERROR: i_interrupt is not INT_IDLE at start!" << std::endl;
    }

    // First, verify that micro‑start is disabled.
    // o_start_micro is driven to 0 in end_of_elaboration.
    // Trigger NPU via CTRL SFR.
    npu_inst->write(NPU::CTRL_OFFSET, 0x1);
    wait(20, SC_NS);
    uint32_t status_val = 0;
    npu_inst->read(NPU::STATUS_OFFSET, status_val);
    std::cout << "[" << name() << "] basic_test: After CTRL trigger with o_start_micro=0, STATUS = 0x"
              << std::hex << status_val << std::dec << " at time " << sc_time_stamp() << std::endl;
    if (status_val != 0) {
        std::cout << "[" << name() << "] basic_test: ERROR: NPU triggered when micro‑start is disabled!" << std::endl;
    } else {
        std::cout << "[" << name() << "] basic_test: SUCCESS: NPU did not trigger when micro‑start is 0." << std::endl;
    }

    // Now, enable micro‑start.
    o_start_micro.write(true);
    wait(10, SC_NS); // Allow time for check_start_micro to update m_enable_start.
    // Trigger NPU via CTRL SFR again.
    npu_inst->write(NPU::CTRL_OFFSET, 0x1);
    wait(20, SC_NS);
    npu_inst->read(NPU::STATUS_OFFSET, status_val);
    std::cout << "[" << name() << "] basic_test: After CTRL trigger with o_start_micro=1, STATUS = 0x"
              << std::hex << status_val << std::dec << " at time " << sc_time_stamp() << std::endl;
    if (status_val & 0x1) {
        std::cout << "[" << name() << "] basic_test: SUCCESS: NPU triggered when micro‑start is enabled." << std::endl;
    } else {
        std::cout << "[" << name() << "] basic_test: ERROR: NPU did not trigger when micro‑start is enabled!" << std::endl;
    }

    // Wait for interrupt event.
    wait(m_interrupt_event);
    std::cout << "[" << name() << "] basic_test: Interrupt counts: DONE=" << m_done_count 
              << ", ERROR=" << m_error_count << " at time " << sc_time_stamp() << std::endl;

    sc_stop();
    while(true) { wait(); }
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
    m_interrupt_event.notify();
}

void tb::end_of_elaboration() {
    o_reset.write(false);
    o_start_micro.write(false); // Initially disable micro‑start.
    std::cout << "[" << name() << "] end_of_elaboration: o_reset set to 0 and o_start_micro set to 0 at time " 
              << sc_time_stamp() << std::endl;
}

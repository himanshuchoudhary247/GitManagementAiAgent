#include "tb.h"
#include <iostream>

tb::tb(sc_core::sc_module_name name) : sc_module(name) {
    // Instantiate the NPU module.
    npu_inst = new NPU("npu_inst");

    // Create the SC_THREAD process.
    SC_THREAD(basic_test);

    // Create the SC_METHOD process sensitive to the test-done event.
    SC_METHOD(handle_test_done_method);
    sensitive << m_basic_test_done_event;
    dont_initialize();
}

tb::~tb() {
    delete npu_inst;
}

void tb::basic_test() {
    // Run in a while(true) loop.
    while (true) {
        // wait(10, SC_NS);
        // Program the NPU registers.
        npu_inst->configure_ifm(0x10, 18);    // IFM: Width = 0x10, Height = 18.
        npu_inst->configure_ofm(14, 16);       // OFM: Width = 14, Height = 16.
        npu_inst->configure_weight(0x3, 0x3);    // Weight: Width = 0x3, Height = 0x3.
        // Trigger the NPU operation by writing '1' to CTRL (offset 0x00000000).
        npu_inst->write(0x00000000, 0x1);
        wait(10, SC_NS);
        uint32_t status_val = 0;
        // npu_inst->write(0x00000004, status_val)
        npu_inst->read(0x00000004, status_val);
        if (status_val & 0x1)
            std::cout << "[tb] NPU Operation SUCCESSFUL at time " << sc_time_stamp() << std::endl;
        else
            std::cout << "[tb] NPU Operation FAILED at time " << sc_time_stamp() << std::endl;
        // Notify that the test is done.
        m_basic_test_done_event.notify();
        break; // Run the test once.
    }
    // Remain in an infinite loop.
    // while (true) {
    //     wait();
    // }
}

void tb::handle_test_done_method() {
    std::cout << "[tb] Test done event received at time " << sc_time_stamp() 
              << ". Stopping simulation." << std::endl;
    sc_stop();
}

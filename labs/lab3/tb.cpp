#include "tb.h"
#include <iostream>

// Constructor
tb::tb(sc_module_name name) : sc_module(name) {
    // Instantiate NPU
    m_npu = new NPU("NPU_Instance");

    // Register SC_THREAD for basic_test
    SC_THREAD(basic_test);

    // Register SC_METHOD for handling test done event
    SC_METHOD(handle_test_done_method);
    sensitive << m_basic_test_done_event;
    dont_initialize();
}

// Basic Test Thread
void tb::basic_test() {
    uint32_t I_read_value = 0;
    bool I_success = true;

    // Configure NPU SFRs
    std::cout << "[" << sc_time_stamp() << "] tb: Configuring NPU SFRs.\n";
    m_npu->write(SAM_NPU_IFM_INFO_OFFSET, 0x0FFF0FFF0);    // IFM Info
    m_npu->write(SAM_NPU_WT_INFO_OFFSET, 0x0FFF0FFF0);     // WT Info
    m_npu->write(SAM_NPU_OFM_INFO_OFFSET, 0x0FFF0FFF0);    // OFM Info

    // Verify IFM Info
    m_npu->read(SAM_NPU_IFM_INFO_OFFSET, I_read_value);
    if (I_read_value != 0x0FFF0FFF0) {
        std::cout << "[" << sc_time_stamp() << "] tb: IFM Info Read Mismatch: 0x" 
                  << std::hex << I_read_value << " != 0x0FFF0FFF0\n" << std::dec;
        I_success = false;
    } else {
        std::cout << "[" << sc_time_stamp() << "] tb: IFM Info Read Success: 0x" 
                  << std::hex << I_read_value << std::dec << "\n";
    }

    // Verify WT Info
    m_npu->read(SAM_NPU_WT_INFO_OFFSET, I_read_value);
    if (I_read_value != 0x0FFF0FFF0) {
        std::cout << "[" << sc_time_stamp() << "] tb: WT Info Read Mismatch: 0x" 
                  << std::hex << I_read_value << " != 0x0FFF0FFF0\n" << std::dec;
        I_success = false;
    } else {
        std::cout << "[" << sc_time_stamp() << "] tb: WT Info Read Success: 0x" 
                  << std::hex << I_read_value << std::dec << "\n";
    }

    // Verify OFM Info
    m_npu->read(SAM_NPU_OFM_INFO_OFFSET, I_read_value);
    if (I_read_value != 0x0FFF0FFF0) {
        std::cout << "[" << sc_time_stamp() << "] tb: OFM Info Read Mismatch: 0x" 
                  << std::hex << I_read_value << " != 0x0FFF0FFF0\n" << std::dec;
        I_success = false;
    } else {
        std::cout << "[" << sc_time_stamp() << "] tb: OFM Info Read Success: 0x" 
                  << std::hex << I_read_value << std::dec << "\n";
    }

    // Trigger NPU operation at 10 ns
    wait(10, SC_NS);
    std::cout << "[" << sc_time_stamp() << "] tb: Triggering NPU operation.\n";
    m_npu->write(SAM_NPU_CTRL_OFFSET, SAM_NPU_CTRL_START_BIT);

    // Wait for operation to complete (operation takes 10 ns)
    wait(11, SC_NS); // Operation delay + some buffer

    // Read STATUS SFR to check if done bit is set
    m_npu->read(SAM_NPU_STATUS_OFFSET, I_read_value);
    if (I_read_value & SAM_NPU_STATUS_DONE_BIT) {
        std::cout << "[" << sc_time_stamp() << "] tb: NPU Operation Status: SUCCESS\n";
    } else {
        std::cout << "[" << sc_time_stamp() << "] tb: NPU Operation Status: FAILURE\n";
        I_success = false;
    }

    // Final Success or Failure Message
    if (I_success) {
        std::cout << "[" << sc_time_stamp() << "] tb: All SFR Read/Write Operations Successful.\n";
    } else {
        std::cout << "[" << sc_time_stamp() << "] tb: Some SFR Read/Write Operations Failed.\n";
    }

    // Raise event to indicate test completion
    m_basic_test_done_event.notify();
}

// Handle Test Done Method
void tb::handle_test_done_method() {
    std::cout << "[" << sc_time_stamp() << "] tb: Basic test done event triggered. Stopping simulation.\n";
    sc_stop();
}

#include "npu.h"
#include <iostream>

// Constructor
NPU::NPU(sc_module_name name) : sc_module(name) {
    // Initialize SFRs with specified configurations
    m_ctrl = new SFR("CTRL_SFR",
                     SAM_SFR_CTRL_OFFSET,
                     0xFFFFFFFE,        // SW Read mask
                     0x00000001,        // SW Write mask
                     0xFFFFFFFF,        // HW Read mask
                     0x00000000,        // HW Write mask
                     0x0);               // Reset value

    m_status = new SFR("STATUS_SFR",
                       SAM_SFR_STATUS_OFFSET,
                       0xFFFFFFFF,        // SW Read mask
                       0x00000000,        // SW Write mask
                       0xFFFFFFFF,        // HW Read mask
                       0x00000003,        // HW Write mask
                       0x0);               // Reset value

    m_ifm_info = new SFR("IFM_INFO_SFR",
                         SAM_SFR_IFM_INFO_OFFSET,
                         0xFFFFFFFF,        // SW Read mask
                         0x0FFF0FFF0,       // SW Write mask
                         0xFFFFFFFF,        // HW Read mask
                         0x00000000,        // HW Write mask
                         0x0);               // Reset value

    m_wt_info = new SFR("WT_INFO_SFR",
                        SAM_SFR_WT_INFO_OFFSET,
                        0xFFFFFFFF,        // SW Read mask
                        0x0FFF0FFF0,       // SW Write mask
                        0xFFFFFFFF,        // HW Read mask
                        0x00000000,        // HW Write mask
                        0x0);               // Reset value

    m_ofm_info = new SFR("OFM_INFO_SFR",
                         SAM_SFR_OFM_INFO_OFFSET,
                         0xFFFFFFFF,        // SW Read mask
                         0x0FFF0FFF0,       // SW Write mask
                         0xFFFFFFFF,        // HW Read mask
                         0x00000000,        // HW Write mask
                         0x0);               // Reset value

    // Populate the SFR map
    m_sfr_map[SAM_NPU_CTRL_OFFSET] = m_ctrl;
    m_sfr_map[SAM_NPU_STATUS_OFFSET] = m_status;
    m_sfr_map[SAM_NPU_IFM_INFO_OFFSET] = m_ifm_info;
    m_sfr_map[SAM_NPU_WT_INFO_OFFSET] = m_wt_info;
    m_sfr_map[SAM_NPU_OFM_INFO_OFFSET] = m_ofm_info;

    // Register SC_THREAD for operation handling
    SC_THREAD(operation_thread);
}

// Destructor
NPU::~NPU() {
    // Delete dynamically allocated SFRs
    delete m_ctrl;
    delete m_status;
    delete m_ifm_info;
    delete m_wt_info;
    delete m_ofm_info;
}

// Write to a specific SFR based on offset
void NPU::write(uint32_t a_offset, const uint32_t &a_value) {
    auto it = m_sfr_map.find(a_offset);
    if (it != m_sfr_map.end()) {
        if (it->first == SAM_NPU_STATUS_OFFSET) {
            // STATUS SFR is typically read-only; ignore or handle accordingly
            std::cout << "[" << sc_time_stamp() << "] NPU: Attempt to write to STATUS SFR ignored.\n";
            return;
        }
        it->second->write(a_value);

        // If CTRL SFR's start bit is set, trigger NPU operation
        if (a_offset == SAM_NPU_CTRL_OFFSET && (a_value & SAM_NPU_CTRL_START_BIT)) {
            std::cout << "[" << sc_time_stamp() << "] NPU: CTRL start bit set. Initiating operation.\n";
            start_operation();
        }
    } else {
        std::cout << "[" << sc_time_stamp() << "] NPU: Invalid SFR Offset: 0x" << std::hex << a_offset << std::dec << "\n";
    }
}

// Read from a specific SFR based on offset
void NPU::read(uint32_t a_offset, uint32_t &a_value) const {
    auto it = m_sfr_map.find(a_offset);
    if (it != m_sfr_map.end()) {
        it->second->read(a_value);
    } else {
        std::cout << "[" << sc_time_stamp() << "] NPU: Invalid SFR Offset: 0x" << std::hex << a_offset << std::dec << "\n";
        a_value = 0;
    }
}

// Initiates the NPU operation
void NPU::start_operation() {
    // Simulate NPU operation delay
    wait(10, SC_NS); // Operation takes 10 ns

    // Set the STATUS done bit
    uint32_t I_set_value = SAM_NPU_STATUS_DONE_BIT;
    m_status->set(I_set_value);

    std::cout << "[" << sc_time_stamp() << "] NPU: Operation completed, STATUS done bit set.\n";

    // Notify that operation is done
    m_operation_done_event.notify();
}

// SC_THREAD: operation_thread
void NPU::operation_thread() {
    while (true) {
        wait(m_operation_done_event);
        // Handle post-operation tasks if needed
        std::cout << "[" << sc_time_stamp() << "] NPU: Operation done event received.\n";
    }
}

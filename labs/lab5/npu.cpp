#include "npu.h"
#include <iostream>
#include <iomanip>

// Define interrupt state constants.
const int NPU::INT_IDLE  = 0;
const int NPU::INT_DONE  = 1;
const int NPU::INT_ERROR = 2;

// Define SFR offset constants.
const uint32_t NPU::CTRL_OFFSET      = 0x00000000;
const uint32_t NPU::STATUS_OFFSET    = 0x00000004;
const uint32_t NPU::INT_CLEAR_OFFSET = 0x00000008;
const uint32_t NPU::IFM_INFO_OFFSET  = 0x00000010;
const uint32_t NPU::WT_INFO_OFFSET   = 0x00000014;
const uint32_t NPU::OFM_INFO_OFFSET  = 0x00000018;

NPU::NPU(sc_core::sc_module_name name)
    : sc_module(name)
{
    // Instantiate SFR modules.
    ctrl = new SFR("ctrl", CTRL_OFFSET, 0xFFFFFFFE, 0x00000001, 0xFFFFFFFF, 0x00000000, 0x0);
    status = new SFR("status", STATUS_OFFSET, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000003, 0x0);
    // int_clear SFR:
    // SW read mask: 0xFFFFFFFC, SW write mask: 0x3, HW write mask: 0x0, HW read mask: 0xFFFFFFFF, reset value: 0.
    int_clear = new SFR("int_clear", INT_CLEAR_OFFSET, 0xFFFFFFFC, 0x00000003, 0xFFFFFFFF, 0x00000000, 0x0);
    ifm_info = new SFR("ifm_info", IFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF, 0xFFFFFFFF, 0x00000000, 0x0);
    wt_info = new SFR("wt_info", WT_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF, 0xFFFFFFFF, 0x00000000, 0x0);
    ofm_info = new SFR("ofm_info", OFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF, 0xFFFFFFFF, 0x00000000, 0x0);

    // Build the mapping.
    sfr_map[CTRL_OFFSET] = ctrl;
    sfr_map[STATUS_OFFSET] = status;
    sfr_map[INT_CLEAR_OFFSET] = int_clear;
    sfr_map[IFM_INFO_OFFSET] = ifm_info;
    sfr_map[WT_INFO_OFFSET] = wt_info;
    sfr_map[OFM_INFO_OFFSET] = ofm_info;

    // Register the reset handler.
    SC_METHOD(handle_reset);
    sensitive << i_reset.pos();
}

NPU::~NPU() {
    delete ctrl;
    delete status;
    delete int_clear;
    delete ifm_info;
    delete wt_info;
    delete ofm_info;
}

void NPU::write(uint32_t offset, const uint32_t &value) {
    auto it = sfr_map.find(offset);
    if (it != sfr_map.end()) {
        // Special handling for int_clear SFR.
        if (offset == INT_CLEAR_OFFSET) {
            it->second->write(value);
            uint32_t clear_val = value & 0x3; // Only bits [1:0] are writable.
            uint32_t status_val = 0;
            status->get(status_val);
            if (clear_val & 0x1) {
                status_val &= ~0x1; // Clear done bit.
            }
            if (clear_val & 0x2) {
                status_val &= ~0x2; // Clear error bit.
            }
            status->set(status_val);
            std::cout << "[" << name() << "] write: int_clear triggered. STATUS updated to 0x"
                      << std::hex << status_val << std::dec
                      << " at time " << sc_time_stamp() << std::endl;
            // Optionally, auto-clear int_clear register.
            it->second->reset();
            // If STATUS is now 0, deassert the interrupt.
            if (status_val == 0) {
                o_interrupt.write(INT_IDLE);
                std::cout << "[" << name() << "] write: STATUS is 0. Interrupt deasserted (INT_IDLE) at time "
                          << sc_time_stamp() << std::endl;
            }
        } else {
            // Normal SFR write.
            it->second->write(value);
            // If writing to CTRL, check the start bit.
            if (offset == CTRL_OFFSET) {
                uint32_t ctrl_val = 0;
                it->second->read(ctrl_val);
                if (ctrl_val & 0x1) {
                    start_operation();
                }
            }
        }
    }
}

void NPU::read(uint32_t offset, uint32_t &value) const {
    auto it = sfr_map.find(offset);
    if (it != sfr_map.end()) {
        it->second->read(value);
    } else {
        value = 0;
    }
}

void NPU::configure_ifm(uint32_t width, uint32_t height) {
    uint32_t config_value = (height << 16) | (width & 0xFFFF);
    ifm_info->write(config_value);
}

void NPU::configure_ofm(uint32_t width, uint32_t height) {
    uint32_t config_value = (height << 16) | (width & 0xFFFF);
    ofm_info->write(config_value);
}

void NPU::configure_weight(uint32_t width, uint32_t height) {
    uint32_t config_value = (height << 16) | (width & 0xFFFF);
    wt_info->write(config_value);
}

void NPU::handle_reset() {
    if (i_reset.read() == true) {
        std::cout << "[" << name() << "] handle_reset: Reset detected at time "
                  << sc_time_stamp() << ". Resetting all SFRs." << std::endl;
        for (auto &entry : sfr_map) {
            entry.second->reset();
        }
        // After reset, drive the interrupt port with INT_IDLE.
        o_interrupt.write(INT_IDLE);
    }
}

void NPU::end_of_elaboration() {
    // Drive the interrupt port with a stable value (INT_IDLE).
    o_interrupt.write(INT_IDLE);
    std::cout << "[" << name() << "] end_of_elaboration: o_interrupt set to INT_IDLE ("
              << INT_IDLE << ") at time " << sc_time_stamp() << std::endl;
}

void NPU::start_operation() {
    uint32_t status_val = 0;
    status->get(status_val);
    status_val |= 0x1; // Set done bit.
    status->set(status_val);
    std::cout << "[" << name() << "] start_operation: Operation done at time "
              << sc_time_stamp() << std::endl;
    // Assert the interrupt port with INT_DONE.
    o_interrupt.write(INT_DONE);
}

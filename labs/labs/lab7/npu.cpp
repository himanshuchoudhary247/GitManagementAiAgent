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

NPU::NPU(sc_module_name name)
    : sc_module(name), m_enable_start(false)
{
    // Instantiate SFR submodules.
    ctrl = new SFR("ctrl", CTRL_OFFSET, 0xFFFFFFFE, 0x00000001,
                   0xFFFFFFFF, 0x00000000, 0x0);
    status = new SFR("status", STATUS_OFFSET, 0xFFFFFFFF, 0x00000000,
                     0xFFFFFFFF, 0x00000003, 0x0);
    int_clear = new SFR("int_clear", INT_CLEAR_OFFSET, 0xFFFFFFFC, 0x00000003,
                        0xFFFFFFFF, 0x00000000, 0x0);
    ifm_info = new SFR("ifm_info", IFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF,
                      0xFFFFFFFF, 0x00000000, 0x0);
    wt_info = new SFR("wt_info", WT_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF,
                     0xFFFFFFFF, 0x00000000, 0x0);
    ofm_info = new SFR("ofm_info", OFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF,
                      0xFFFFFFFF, 0x00000000, 0x0);

    // Build the mapping.
    sfr_map[CTRL_OFFSET] = ctrl;
    sfr_map[STATUS_OFFSET] = status;
    sfr_map[INT_CLEAR_OFFSET] = int_clear;
    sfr_map[IFM_INFO_OFFSET] = ifm_info;
    sfr_map[WT_INFO_OFFSET] = wt_info;
    sfr_map[OFM_INFO_OFFSET] = ofm_info;

    // Register reset handler.
    SC_METHOD(handle_reset);
    sensitive << i_reset.pos();

    // Register method to update m_enable_start.
    SC_METHOD(check_start_micro);
    sensitive << i_start_micro;
    dont_initialize();
}

NPU::~NPU() {
    delete ctrl;
    delete status;
    delete int_clear;
    delete ifm_info;
    delete wt_info;
    delete ofm_info;
}

void NPU::check_start_micro() {
    m_enable_start = i_start_micro.read();
    std::cout << "[" << name() << "] check_start_micro: i_start_micro = " 
              << m_enable_start << " at time " << sc_time_stamp() << std::endl;
}

void NPU::write(uint32_t offset, const uint32_t &value) {
    std::unordered_map<uint32_t, SFR*>::iterator it = sfr_map.find(offset);
    if (it != sfr_map.end()) {
        if (offset == INT_CLEAR_OFFSET) {
            it->second->write(value);
            uint32_t clear_val = value & 0x3; // only bits [1:0] writable.
            uint32_t status_val = 0;
            status->get(status_val);
            if (clear_val & 0x1)
                status_val &= ~0x1; // clear DONE bit.
            if (clear_val & 0x2)
                status_val &= ~0x2; // clear ERROR bit.
            status->set(status_val);
            std::cout << "[" << name() << "] write: int_clear triggered. STATUS updated to 0x"
                      << std::hex << status_val << std::dec
                      << " at time " << sc_time_stamp() << std::endl;
            it->second->reset();
            if (status_val == 0) {
                o_interrupt.write(INT_IDLE);
                std::cout << "[" << name() << "] write: STATUS is 0. Interrupt deasserted (INT_IDLE) at time "
                          << sc_time_stamp() << std::endl;
            }
        } else {
            it->second->write(value);
            if (offset == CTRL_OFFSET) {
                uint32_t ctrl_val = 0;
                it->second->read(ctrl_val);
                if (ctrl_val & 0x1) {
                    if (m_enable_start) {
                        // Spawn a dynamic process to run start_operation.
                        sc_spawn(sc_bind(&NPU::start_operation, this), "dynamic_start_operation");
                    } else {
                        std::cout << "[" << name() << "] write: CTRL trigger ignored because i_start_micro is 0 at time " 
                                  << sc_time_stamp() << std::endl;
                    }
                }
            }
        }
    }
}

void NPU::read(uint32_t offset, uint32_t &value) const {
    std::unordered_map<uint32_t, SFR*>::const_iterator it = sfr_map.find(offset);
    if (it != sfr_map.end())
        it->second->read(value);
    else
        value = 0;
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
        for (std::unordered_map<uint32_t, SFR*>::iterator it = sfr_map.begin();
             it != sfr_map.end(); ++it) {
            it->second->reset();
        }
        o_interrupt.write(INT_IDLE);
    }
}

void NPU::end_of_elaboration() {
    o_interrupt.write(INT_IDLE);
    std::cout << "[" << name() << "] end_of_elaboration: o_interrupt set to INT_IDLE ("
              << INT_IDLE << ") at time " << sc_time_stamp() << std::endl;
    // Spawn a dynamic thread process: dummy_thread.
    sc_spawn(sc_bind(&NPU::dummy_thread, this), "dummy_thread_dynamic");
}

void NPU::start_operation() {
    uint32_t status_val = 0;
    status->get(status_val);
    status_val |= 0x1; // set DONE bit.
    status->set(status_val);
    std::cout << "[" << name() << "] start_operation: Operation done at time " 
              << sc_time_stamp() << std::endl;
    o_interrupt.write(INT_DONE);
}

void NPU::dummy_thread() {
    std::cout << "[" << name() << "] dummy_thread dynamic process started at time " 
              << sc_time_stamp() << std::endl;
    wait(10, SC_NS);
    std::cout << "[" << name() << "] dummy_thread dynamic process ended at time " 
              << sc_time_stamp() << std::endl;
}

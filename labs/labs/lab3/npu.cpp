#include "npu.h"
#include <iostream>

// Define the static constants.
const uint32_t NPU::CTRL_OFFSET    = 0x00000000;
const uint32_t NPU::STATUS_OFFSET  = 0x00000004;
const uint32_t NPU::IFM_INFO_OFFSET = 0x00000010;
const uint32_t NPU::WT_INFO_OFFSET  = 0x00000014;
const uint32_t NPU::OFM_INFO_OFFSET = 0x00000018;

NPU::NPU(sc_core::sc_module_name name) : sc_module(name) {
    // Dynamically instantiate SFR modules.
    ctrl = new SFR("ctrl", CTRL_OFFSET, 0xFFFFFFFE, 0x00000001, 0xFFFFFFFF, 0x00000000, 0x0);
    status = new SFR("status", STATUS_OFFSET, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000003, 0x0);
    ifm_info = new SFR("ifm_info", IFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF, 0xFFFFFFFF, 0x00000000, 0x0);
    wt_info = new SFR("wt_info", WT_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF, 0xFFFFFFFF, 0x00000000, 0x0);
    ofm_info = new SFR("ofm_info", OFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF, 0xFFFFFFFF, 0x00000000, 0x0);

    // Build the mapping from offsets to SFR pointers.
    sfr_map[CTRL_OFFSET]   = ctrl;
    sfr_map[STATUS_OFFSET] = status;
    sfr_map[IFM_INFO_OFFSET] = ifm_info;
    sfr_map[WT_INFO_OFFSET]  = wt_info;
    sfr_map[OFM_INFO_OFFSET] = ofm_info;
}

NPU::~NPU() {
    delete ctrl;
    delete status;
    delete ifm_info;
    delete wt_info;
    delete ofm_info;
}

void NPU::write(uint32_t offset, const uint32_t &value) {
    auto it = sfr_map.find(offset);
    if (it != sfr_map.end()) {
        it->second->write(value);
        // If writing to CTRL and the start bit is set, trigger the operation.
        if (offset == CTRL_OFFSET) {
            uint32_t ctrl_val = 0;
            it->second->read(ctrl_val);
            if (ctrl_val & 0x1) {
                start_operation();
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

void NPU::start_operation() {
    uint32_t status_val = 0;
    status->get(status_val);
    status_val |= 0x1; // Set done bit.
    status->set(status_val);
    std::cout << "[NPU] Operation started at time " << sc_time_stamp() << std::endl;
}

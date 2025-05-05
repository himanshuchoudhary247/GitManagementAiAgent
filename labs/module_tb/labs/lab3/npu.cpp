#include "npu.h"
#include <iostream>

// Offsets for each SFR (as given)
static const uint32_t CTRL_OFFSET     = 0x00000000;
static const uint32_t STATUS_OFFSET   = 0x00000004;
static const uint32_t IFM_INFO_OFFSET = 0x00000010;
static const uint32_t WT_INFO_OFFSET  = 0x00000014;
static const uint32_t OFM_INFO_OFFSET = 0x00000018;

NPU::NPU(sc_module_name name)
    : sc_module(name)
{
    ctrl = new SFR("ctrl",
                   CTRL_OFFSET, 0xFFFFFFFE, 0x00000001,
                   0xFFFFFFFF, 0x00000000, 0x0);
    status = new SFR("status",
                     STATUS_OFFSET, 0xFFFFFFFF, 0x00000000,
                     0xFFFFFFFF, 0x00000003, 0x0);
    ifm_info = new SFR("ifm_info",
                       IFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF, 0x00000000, 0x0);
    wt_info  = new SFR("wt_info",
                       WT_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF, 0x00000000, 0x0);
    ofm_info = new SFR("ofm_info",
                       OFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF, 0x00000000, 0x0);
}

NPU::~NPU() {
    delete ctrl;
    delete status;
    delete ifm_info;
    delete wt_info;
    delete ofm_info;
}

void NPU::write(uint32_t offset, const uint32_t &value) {
    switch (offset) {
        case CTRL_OFFSET:     ctrl->write(value); break;
        case STATUS_OFFSET:   status->write(value); break;
        case IFM_INFO_OFFSET: ifm_info->write(value); break;
        case WT_INFO_OFFSET:  wt_info->write(value); break;
        case OFM_INFO_OFFSET: ofm_info->write(value); break;
        default:
            std::cout << "[NPU] write: Unknown offset 0x" 
                      << std::hex << offset << std::dec << std::endl;
            break;
    }
}

void NPU::read(uint32_t offset, uint32_t &value) const {
    switch (offset) {
        case CTRL_OFFSET:     ctrl->read(value); break;
        case STATUS_OFFSET:   status->read(value); break;
        case IFM_INFO_OFFSET: ifm_info->read(value); break;
        case WT_INFO_OFFSET:  wt_info->read(value); break;
        case OFM_INFO_OFFSET: ofm_info->read(value); break;
        default:
            value = 0;
            std::cout << "[NPU] read: Unknown offset 0x" 
                      << std::hex << offset << std::dec << std::endl;
            break;
    }
}

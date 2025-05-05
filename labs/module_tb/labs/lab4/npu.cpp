#include "npu.h"
#include <iostream>

// Offsets
static const uint32_t CTRL_OFFSET     = 0x00000000;
static const uint32_t STATUS_OFFSET   = 0x00000004;
static const uint32_t IFM_INFO_OFFSET = 0x00000010;
static const uint32_t WT_INFO_OFFSET  = 0x00000014;
static const uint32_t OFM_INFO_OFFSET = 0x00000018;


// Define interrupt states
const int NPU::INT_IDLE  = 0;
const int NPU::INT_DONE  = 1;
const int NPU::INT_ERROR = 2;

NPU::NPU(sc_module_name name)
    : sc_module(name)
{
    ctrl = new SFR("ctrl",
                   CTRL_OFFSET,  0xFFFFFFFE, 0x00000001,
                   0xFFFFFFFF,   0x00000000, 0x0);
    status = new SFR("status",
                     STATUS_OFFSET, 0xFFFFFFFF, 0x00000000,
                     0xFFFFFFFF,   0x00000003, 0x0);
    ifm_info = new SFR("ifm_info",
                       IFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF,      0x00000000, 0x0);
    wt_info  = new SFR("wt_info",
                       WT_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF,     0x00000000, 0x0);
    ofm_info = new SFR("ofm_info",
                       OFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF,     0x00000000, 0x0);

    SC_METHOD(handle_reset);
    sensitive << i_reset.pos();
    dont_initialize();

    // Initialize o_interrupt to INT_IDLE in the constructor
    // but we also ensure stable value in end_of_elaboration
    o_interrupt.initialize(INT_IDLE);
}

NPU::~NPU() {
    delete ctrl;
    delete status;
    delete ifm_info;
    delete wt_info;
    delete ofm_info;
}

void NPU::end_of_elaboration() {
    // Drive interrupt port with stable value (active low => IDLE=0)
    o_interrupt.write(INT_IDLE);
    std::cout << "[" << name() << "] end_of_elaboration: o_interrupt=INT_IDLE at " 
              << sc_time_stamp() << std::endl;
}

void NPU::handle_reset() {
    // On posedge reset => reset internal state
    if (i_reset.read()) {
        std::cout << "[" << name() << "] handle_reset: i_reset=1 at " << sc_time_stamp() << std::endl;
        // Reset each SFR
        ctrl->reset();
        status->reset();
        ifm_info->reset();
        wt_info->reset();
        ofm_info->reset();
        // Deassert interrupt
        o_interrupt.write(INT_IDLE);
    }
}

void NPU::check_done_bit(const uint32_t &val) {
    // If DONE bit is set in 'val', set interrupt => INT_DONE
    if (val & 0x1) {
        o_interrupt.write(INT_DONE);
        std::cout << "[" << name() << "] check_done_bit: status=0x"
                  << std::hex << val << std::dec 
                  << " => interrupt = INT_DONE at " << sc_time_stamp() << std::endl;
    }
    // If ERROR bit is set in 'val', we could do => INT_ERROR, but not specified
}

void NPU::write(uint32_t offset, const uint32_t &value) {
    switch (offset) {
        case CTRL_OFFSET:
            ctrl->write(value);
            break;
        case STATUS_OFFSET: {
            status->write(value);
            // After writing to status, check if done bit is set
            uint32_t valAfter;
            status->read(valAfter);
            check_done_bit(valAfter);
            break;
        }
        case IFM_INFO_OFFSET:
            ifm_info->write(value);
            break;
        case WT_INFO_OFFSET:
            wt_info->write(value);
            break;
        case OFM_INFO_OFFSET:
            ofm_info->write(value);
            break;
        default:
            std::cout << "[" << name() << "] write: Unknown offset 0x" 
                      << std::hex << offset << std::dec << std::endl;
            break;
    }
}

void NPU::read(uint32_t offset, uint32_t &value) const {
    switch (offset) {
        case CTRL_OFFSET:
            ctrl->read(value);
            break;
        case STATUS_OFFSET:
            status->read(value);
            break;
        case IFM_INFO_OFFSET:
            ifm_info->read(value);
            break;
        case WT_INFO_OFFSET:
            wt_info->read(value);
            break;
        case OFM_INFO_OFFSET:
            ofm_info->read(value);
            break;
        default:
            value = 0;
            std::cout << "[" << name() << "] read: Unknown offset=0x"
                      << std::hex << offset << std::dec << std::endl;
            break;
    }
}

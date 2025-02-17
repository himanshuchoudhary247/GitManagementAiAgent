#include "npu.h"
#include <iostream>

// Offsets
static const uint32_t CTRL_OFFSET      = 0x00000000;
static const uint32_t STATUS_OFFSET    = 0x00000004;
static const uint32_t INT_CLEAR_OFFSET = 0x00000008; // new
static const uint32_t IFM_INFO_OFFSET  = 0x00000010;
static const uint32_t WT_INFO_OFFSET   = 0x00000014;
static const uint32_t OFM_INFO_OFFSET  = 0x00000018;

// Interrupt states
const int NPU::INT_IDLE  = 0;
const int NPU::INT_DONE  = 1;
const int NPU::INT_ERROR = 2;

NPU::NPU(sc_module_name name)
    : sc_module(name)
{
    // Create SFRs
    ctrl = new SFR("ctrl",
                   CTRL_OFFSET, 0xFFFFFFFE, 0x00000001,
                   0xFFFFFFFF,  0x00000000, 0x0);
    status = new SFR("status",
                     STATUS_OFFSET, 0xFFFFFFFF, 0x00000000,
                     0xFFFFFFFF,  0x00000003, 0x0);

    // new SFR for interrupt clear
    // sw_read_mask=0xFFFFFFFC, sw_write_mask=0x00000003,
    // hw_read_mask=0xFFFFFFFF, hw_write_mask=0x00000000, reset_value=0
    int_clear = new SFR("int_clear",
                        INT_CLEAR_OFFSET, 0xFFFFFFFC, 0x00000003,
                        0xFFFFFFFF,       0x00000000, 0x0);

    ifm_info = new SFR("ifm_info",
                       IFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF,      0x00000000, 0x0);
    wt_info  = new SFR("wt_info",
                       WT_INFO_OFFSET,  0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF,      0x00000000, 0x0);
    ofm_info = new SFR("ofm_info",
                       OFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF,      0x00000000, 0x0);

    SC_METHOD(handle_reset);
    sensitive << i_reset.pos();
    dont_initialize();

    // Initialize interrupt
    o_interrupt.initialize(INT_IDLE);
}

NPU::~NPU() {
    delete ctrl;
    delete status;
    delete int_clear;
    delete ifm_info;
    delete wt_info;
    delete ofm_info;
}

void NPU::end_of_elaboration() {
    // stable value => INT_IDLE
    o_interrupt.write(INT_IDLE);
    std::cout << "[" << name() << "] end_of_elaboration => o_interrupt=INT_IDLE at "
              << sc_time_stamp() << std::endl;
}

void NPU::handle_reset() {
    if (i_reset.read()) {
        std::cout << "[" << name() << "] handle_reset => i_reset=1 at "
                  << sc_time_stamp() << std::endl;
        // Reset SFRs
        ctrl->reset();
        status->reset();
        int_clear->reset();
        ifm_info->reset();
        wt_info->reset();
        ofm_info->reset();
        // Deassert interrupt
        o_interrupt.write(INT_IDLE);
    }
}

void NPU::check_done_bit(uint32_t val) {
    // If bit 0 is set => DONE
    if (val & 0x1) {
        o_interrupt.write(INT_DONE);
        std::cout << "[" << name() << "] check_done_bit => status=0x"
                  << std::hex << val << std::dec 
                  << " => o_interrupt=INT_DONE at " << sc_time_stamp() << std::endl;
    }
    // For Lab5 we skip error bit handling or can do similarly
}

void NPU::handle_int_clear(uint32_t val) {
    // If bit0 is set => clear DONE bit in STATUS
    // If bit1 is set => clear ERROR bit in STATUS
    // Then read back status => if 0 => interrupt=IDLE
    uint32_t sVal;
    status->read(sVal);  // existing value in status
    if (val & 0x1) {
        sVal &= ~0x1; // clear done bit
    }
    if (val & 0x2) {
        sVal &= ~0x2; // clear error bit
    }
    // write back to status
    status->write(sVal);
    // now read final status => if 0 => interrupt=IDLE
    uint32_t newVal;
    status->read(newVal);
    if (newVal == 0) {
        o_interrupt.write(INT_IDLE);
        std::cout << "[" << name() << "] handle_int_clear => status=0 => o_interrupt=INT_IDLE at "
                  << sc_time_stamp() << std::endl;
    }
}

void NPU::write(uint32_t offset, const uint32_t &value) {
    switch (offset) {
        case CTRL_OFFSET:
            ctrl->write(value);
            break;
        case STATUS_OFFSET: {
            status->write(value);
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
        case INT_CLEAR_OFFSET: {
            int_clear->write(value);
            // now handle clearing bits in status
            handle_int_clear(value);
            break;
        }
        default:
            std::cout << "[" << name() << "] write => unknown offset=0x"
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
        case INT_CLEAR_OFFSET:
            int_clear->read(value);
            break;
        default:
            value = 0;
            std::cout << "[" << name() << "] read => unknown offset=0x"
                      << std::hex << offset << std::dec << std::endl;
            break;
    }
}

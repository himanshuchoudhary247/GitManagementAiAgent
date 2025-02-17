#include "npu.h"
#include <iostream>

// Offsets
static const uint32_t CTRL_OFFSET      = 0x00000000;
static const uint32_t STATUS_OFFSET    = 0x00000004;
static const uint32_t INT_CLEAR_OFFSET = 0x00000008;
static const uint32_t IFM_INFO_OFFSET  = 0x00000010;
static const uint32_t WT_INFO_OFFSET   = 0x00000014;
static const uint32_t OFM_INFO_OFFSET  = 0x00000018;

const int NPU::INT_IDLE  = 0;
const int NPU::INT_DONE  = 1;
const int NPU::INT_ERROR = 2;

NPU::NPU(sc_module_name name)
    : sc_module(name),
      m_enable_start(false)
{
    ctrl = new SFR("ctrl",
                   CTRL_OFFSET,  0xFFFFFFFE, 0x00000001,
                   0xFFFFFFFF,   0x00000000, 0x0);
    status = new SFR("status",
                     STATUS_OFFSET, 0xFFFFFFFF, 0x00000000,
                     0xFFFFFFFF,   0x00000003, 0x0);
    int_clear = new SFR("int_clear",
                        INT_CLEAR_OFFSET, 0xFFFFFFFC, 0x00000003,
                        0xFFFFFFFF,        0x00000000, 0x0);
    ifm_info = new SFR("ifm_info",
                       IFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF,      0x00000000, 0x0);
    wt_info  = new SFR("wt_info",
                       WT_INFO_OFFSET,  0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF,      0x00000000, 0x0);
    ofm_info = new SFR("ofm_info",
                       OFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF,      0x00000000, 0x0);

    // handle reset => posedge i_reset
    SC_METHOD(handle_reset);
    sensitive << i_reset.pos();
    dont_initialize();

    // watch i_start_micro => sets m_enable_start
    SC_METHOD(check_start_micro);
    sensitive << i_start_micro;
    dont_initialize();

    // init interrupt
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
    // stable => interrupt=IDLE
    o_interrupt.write(INT_IDLE);
    // also ensure i_start_micro => disable start
    m_enable_start = false;
    std::cout << "[" << name() << "] end_of_elaboration => interrupt=IDLE, disable start at "
              << sc_time_stamp() << std::endl;
}

void NPU::handle_reset() {
    if (i_reset.read()) {
        std::cout << "[" << name() << "] handle_reset => i_reset=1 at "
                  << sc_time_stamp() << std::endl;
        ctrl->reset();
        status->reset();
        int_clear->reset();
        ifm_info->reset();
        wt_info->reset();
        ofm_info->reset();
        o_interrupt.write(INT_IDLE);
        m_enable_start = false;
    }
}

void NPU::check_done_bit(uint32_t val) {
    // bit0 => done
    if (val & 0x1) {
        o_interrupt.write(INT_DONE);
        std::cout << "[" << name() << "] check_done_bit => status=0x"
                  << std::hex << val << std::dec
                  << " => interrupt=DONE at " << sc_time_stamp() << std::endl;
    }
}

void NPU::handle_int_clear(uint32_t val) {
    uint32_t sVal;
    status->read(sVal);
    if (val & 0x1) {
        sVal &= ~0x1; // clear done
    }
    if (val & 0x2) {
        sVal &= ~0x2; // clear error
    }
    status->write(sVal);
    uint32_t newVal;
    status->read(newVal);
    if (newVal == 0) {
        o_interrupt.write(INT_IDLE);
        std::cout << "[" << name() << "] handle_int_clear => status=0 => interrupt=IDLE at "
                  << sc_time_stamp() << std::endl;
    }
}

void NPU::check_start_micro() {
    bool new_val = i_start_micro.read();
    m_enable_start = new_val;
    std::cout << "[" << name() << "] check_start_micro => i_start_micro=" 
              << new_val << " => m_enable_start=" << m_enable_start
              << " at " << sc_time_stamp() << std::endl;
}

void NPU::write(uint32_t offset, const uint32_t &value) {
    switch (offset) {
        case CTRL_OFFSET:
            // only trigger if m_enable_start==true
            if (m_enable_start && (value & 0x1)) {
                // set done bit in status
                uint32_t sVal;
                status->read(sVal);
                sVal |= 0x1; // bit0 => done
                status->write(sVal);
                // check done bit => set interrupt
                check_done_bit(sVal);
            } else {
                std::cout << "[" << name() << "] write => CTRL ignored because m_enable_start="
                          << m_enable_start << " at " << sc_time_stamp() << std::endl;
            }
            break;
        case STATUS_OFFSET: {
            status->write(value);
            uint32_t valAfter;
            status->read(valAfter);
            check_done_bit(valAfter);
            break;
        }
        case INT_CLEAR_OFFSET: {
            int_clear->write(value);
            handle_int_clear(value);
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
        case INT_CLEAR_OFFSET:
            int_clear->read(value);
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
            std::cout << "[" << name() << "] read => unknown offset=0x"
                      << std::hex << offset << std::dec << std::endl;
            break;
    }
}

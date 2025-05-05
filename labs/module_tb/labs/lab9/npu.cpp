#include "npu.h"
#include <systemc.h>
#include <iostream>

// Define interrupt constants.
const int NPU::INT_IDLE  = 0;
const int NPU::INT_DONE  = 1;
const int NPU::INT_ERROR = 2;

// Offsets.
static const uint32_t CTRL_OFFSET      = 0x00000000;
static const uint32_t STATUS_OFFSET    = 0x00000004;
static const uint32_t INT_CLEAR_OFFSET = 0x00000008;
static const uint32_t IFM_INFO_OFFSET  = 0x00000010;
static const uint32_t WT_INFO_OFFSET   = 0x00000014;
static const uint32_t OFM_INFO_OFFSET  = 0x00000018;

NPU::NPU(sc_module_name name)
    : sc_module(name),
      o_npu2mem_ifm("o_npu2mem_ifm"),
      o_npu2mem_wt("o_npu2mem_wt"),
      o_npu2mem_ofm("o_npu2mem_ofm"),
      ifm_counter(0),
      ofm_counter(0)
{
    // Instantiate SFRs.
    ctrl = new SFR("ctrl",
                   CTRL_OFFSET, 0xFFFFFFFE, 0x00000001,
                   0xFFFFFFFF, 0x00000000, 0x0);
    status = new SFR("status",
                     STATUS_OFFSET, 0xFFFFFFFF, 0x00000000,
                     0xFFFFFFFF, 0x00000003, 0x0);
    int_clear = new SFR("int_clear",
                        INT_CLEAR_OFFSET, 0xFFFFFFFC, 0x00000003,
                        0xFFFFFFFF,      0x00000000, 0x0);
    ifm_info = new SFR("ifm_info",
                       IFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF,      0x00000000, 0x0);
    wt_info = new SFR("wt_info",
                      WT_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                      0xFFFFFFFF,      0x00000000, 0x0);
    ofm_info = new SFR("ofm_info",
                       OFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF,      0x00000000, 0x0);

    // Initialize FIFO vectors.
    o_npu2mem_ifm.init(2);
    o_npu2mem_wt.init(1);
    o_npu2mem_ofm.init(2);
}

NPU::~NPU() {
    delete ctrl;
    delete status;
    delete int_clear;
    delete ifm_info;
    delete wt_info;
    delete ofm_info;
}

void NPU::start_operation() {
    npu2mem trans;
    
    // IFM read (round robin)
    wait(10, SC_NS);
    unsigned int ifm_addr;
    ifm_info->read(ifm_addr);
    trans.address = ifm_addr;
    trans.is_read = true;
    trans.data = 0;
    unsigned int ifm_index = ifm_counter % 2;
    o_npu2mem_ifm[ifm_index].write(trans);
    std::cout << "[" << name() << "] start_operation: Sent IFM read for address "
              << ifm_addr << " on FIFO index " << ifm_index
              << " at " << sc_time_stamp() << std::endl;
    ifm_counter++;

    // Weight read (only one FIFO)
    wait(10, SC_NS);
    unsigned int wt_addr;
    wt_info->read(wt_addr);
    trans.address = wt_addr;
    trans.is_read = true;
    trans.data = 0;
    o_npu2mem_wt[0].write(trans);
    std::cout << "[" << name() << "] start_operation: Sent Weight read for address "
              << wt_addr << " at " << sc_time_stamp() << std::endl;

    // OFM write (round robin)
    wait(10, SC_NS);
    unsigned int ofm_addr;
    ofm_info->read(ofm_addr);
    trans.address = ofm_addr;
    trans.is_read = false;
    trans.data = 0xDEADBEEF;
    unsigned int ofm_index = ofm_counter % 2;
    o_npu2mem_ofm[ofm_index].write(trans);
    std::cout << "[" << name() << "] start_operation: Sent OFM write for address "
              << ofm_addr << " on FIFO index " << ofm_index
              << " at " << sc_time_stamp() << std::endl;
    ofm_counter++;

    // Wait and then assert interrupt.
    wait(10, SC_NS);
    uint32_t sVal;
    status->read(sVal);
    sVal |= 0x1;
    status->write(sVal);
    o_interrupt.write(INT_DONE);
    std::cout << "[" << name() << "] start_operation: Operation complete, interrupt=INT_DONE at "
              << sc_time_stamp() << std::endl;
}

void NPU::write(uint32_t offset, const uint32_t &value) {
    switch (offset) {
        case CTRL_OFFSET:
            if (value & 0x1) {
                start_operation();
            }
            ctrl->write(value);
            break;
        case STATUS_OFFSET:
            status->write(value);
            break;
        case INT_CLEAR_OFFSET: {
            int_clear->write(value);
            uint32_t sVal;
            status->read(sVal);
            if (value & 0x1) { sVal &= ~0x1; }
            if (value & 0x2) { sVal &= ~0x2; }
            status->write(sVal);
            if (sVal == 0) {
                o_interrupt.write(INT_IDLE);
                std::cout << "[" << name() << "] write: STATUS cleared, interrupt=INT_IDLE at "
                          << sc_time_stamp() << std::endl;
            }
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
            std::cout << "[" << name() << "] read: Unknown offset 0x"
                      << std::hex << offset << std::dec << std::endl;
            break;
    }
}

void NPU::end_of_elaboration() {
    o_interrupt.write(INT_IDLE);
    std::cout << "[" << name() << "] end_of_elaboration: o_interrupt set to INT_IDLE at "
              << sc_time_stamp() << std::endl;
}

#include "tb.h"
#include <iostream>

static const uint32_t CTRL_OFFSET     = 0x00000000;
static const uint32_t STATUS_OFFSET   = 0x00000004;
static const uint32_t IFM_INFO_OFFSET = 0x00000010;
static const uint32_t WT_INFO_OFFSET  = 0x00000014;
static const uint32_t OFM_INFO_OFFSET = 0x00000018;

tb::tb(sc_module_name name)
    : sc_module(name)
{
    npu_inst = new NPU("npu_inst");

    SC_THREAD(basic_test);
}

tb::~tb() {
    delete npu_inst;
}

void tb::basic_test() {
    wait(10, SC_NS);

    npu_inst->write(IFM_INFO_OFFSET, 0xABCD1234);
    npu_inst->write(WT_INFO_OFFSET,  0x11112222);
    npu_inst->write(OFM_INFO_OFFSET, 0xDEADDEAD);

    uint32_t val = 0;
    npu_inst->read(IFM_INFO_OFFSET, val);
    std::cout << "[" << name() << "] basic_test: IFM_INFO = 0x" 
              << std::hex << val << std::dec << " at time " << sc_time_stamp() << std::endl;
    npu_inst->read(WT_INFO_OFFSET, val);
    std::cout << "[" << name() << "] basic_test: WT_INFO = 0x" 
              << std::hex << val << std::dec << " at time " << sc_time_stamp() << std::endl;
    npu_inst->read(OFM_INFO_OFFSET, val);
    std::cout << "[" << name() << "] basic_test: OFM_INFO = 0x" 
              << std::hex << val << std::dec << " at time " << sc_time_stamp() << std::endl;

    npu_inst->write(CTRL_OFFSET, 0x1);
    std::cout << "[" << name() << "] basic_test: Wrote 1 to ctrl at time " 
              << sc_time_stamp() << std::endl;

    npu_inst->read(STATUS_OFFSET, val);
    std::cout << "[" << name() << "] basic_test: STATUS = 0x" 
              << std::hex << val << std::dec << " at time " << sc_time_stamp() << std::endl;

    // Wait to allow event_queue_demo_thread to process scheduled events.
    wait(20, SC_NS);
}

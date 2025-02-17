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
    // Instantiate NPU
    npu_inst = new NPU("npu_inst");

    // Bind NPU ports
    npu_inst->i_reset(reset_sig);
    npu_inst->o_interrupt(intr_sig);

    // Bind tb ports to internal signals
    o_reset(reset_sig);
    i_interrupt(intr_sig);

    SC_THREAD(basic_test);
}

tb::~tb() {
    delete npu_inst;
}

void tb::end_of_elaboration() {
    // Initialize reset to 0
    o_reset.write(false);
    std::cout << "[" << name() << "] end_of_elaboration: o_reset=0 at " << sc_time_stamp() << std::endl;
}

void tb::basic_test() {
    // Wait 10 ns
    wait(10, SC_NS);

    // Drive reset => 1
    o_reset.write(true);
    wait(5, SC_NS);
    o_reset.write(false);

    // Write some data to SFRs
    npu_inst->write(IFM_INFO_OFFSET, 0xABCD1234);
    npu_inst->write(WT_INFO_OFFSET,  0x11112222);
    npu_inst->write(OFM_INFO_OFFSET, 0xDEADDEAD);

    // Check that interrupt = IDLE
    int intr_val = i_interrupt.read();
    std::cout << "[" << name() << "] after SFR writes: i_interrupt=" << intr_val 
              << " (expected IDLE=" << NPU::INT_IDLE 
              << ") at " << sc_time_stamp() << std::endl;

    // Trigger NPU => set status done bit
    npu_inst->write(STATUS_OFFSET, 0x1);
    // Check interrupt
    wait(1, SC_NS);
    intr_val = i_interrupt.read();
    std::cout << "[" << name() << "] after writing done bit: i_interrupt=" << intr_val
              << " (expected DONE=" << NPU::INT_DONE
              << ") at " << sc_time_stamp() << std::endl;

    wait(10, SC_NS);
}

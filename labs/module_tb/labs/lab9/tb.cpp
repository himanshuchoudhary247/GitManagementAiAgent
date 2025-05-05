#include "tb.h"
#include <iostream>

static const uint32_t CTRL_OFFSET      = 0x00000000;
static const uint32_t STATUS_OFFSET    = 0x00000004;
static const uint32_t INT_CLEAR_OFFSET = 0x00000008;
static const uint32_t IFM_INFO_OFFSET  = 0x00000010;
static const uint32_t WT_INFO_OFFSET   = 0x00000014;
static const uint32_t OFM_INFO_OFFSET  = 0x00000018;

tb::tb(sc_module_name name)
    : sc_module(name),
      fifo_ifm("fifo_ifm", 2),
      fifo_wt("fifo_wt", 1),
      fifo_ofm("fifo_ofm", 2)
{
    // Instantiate NPU.
    npu_inst = new NPU("npu_inst");

    // Bind NPU ports.
    npu_inst->i_reset(reset_sig);
    npu_inst->o_interrupt(intr_sig);

    // Instantiate Memory modules with corresponding FIFO vector sizes.
    mem_ifm = new memory("mem_ifm", 2);
    mem_wt  = new memory("mem_wt", 1);
    mem_ofm = new memory("mem_ofm", 2);

    // Bind FIFO channels between NPU and Memory.
    npu_inst->o_npu2mem_ifm.bind(fifo_ifm);
    npu_inst->o_npu2mem_wt.bind(fifo_wt);
    npu_inst->o_npu2mem_ofm.bind(fifo_ofm);

    mem_ifm->i_npu2mem.bind(fifo_ifm);
    mem_wt->i_npu2mem.bind(fifo_wt);
    mem_ofm->i_npu2mem.bind(fifo_ofm);

    SC_THREAD(basic_test);
}

tb::~tb() {
    delete npu_inst;
    delete mem_ifm;
    delete mem_wt;
    delete mem_ofm;
}

void tb::basic_test() {
    // Program SFRs for dimensions.
    // High 16 bits = height, low 16 bits = width.
    npu_inst->write(IFM_INFO_OFFSET, (6 << 16) | 6);
    npu_inst->write(WT_INFO_OFFSET,  (3 << 16) | 3);
    npu_inst->write(OFM_INFO_OFFSET, (5 << 16) | 5);

    // Program SFRs for memory addresses.
    npu_inst->write(IFM_INFO_OFFSET, 4096);
    npu_inst->write(WT_INFO_OFFSET,  8192);
    npu_inst->write(OFM_INFO_OFFSET, 12288);

    wait(10, SC_NS);

    // Trigger NPU operation by writing 1 to CTRL.
    npu_inst->write(CTRL_OFFSET, 0x1);
    std::cout << "[tb] basic_test: Triggered NPU operation at " << sc_time_stamp() << std::endl;

    wait(50, SC_NS);

    uint32_t stVal;
    npu_inst->read(STATUS_OFFSET, stVal);
    std::cout << "[tb] basic_test: STATUS=0x" << std::hex << stVal << std::dec
              << " at " << sc_time_stamp() << std::endl;

    wait(10, SC_NS);
}

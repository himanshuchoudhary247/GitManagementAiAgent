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
      m_done_count(0),
      m_error_count(0)
{
    // Instantiate NPU
    npu_inst = new NPU("npu_inst");

    // Bind NPU ports
    npu_inst->i_reset(reset_sig);
    npu_inst->i_start_micro(start_micro_sig);
    npu_inst->o_interrupt(intr_sig);

    // Bind tb external ports
    o_reset(reset_sig);
    o_start_micro(start_micro_sig);
    i_interrupt(intr_sig);

    SC_THREAD(basic_test);

    SC_METHOD(handle_interrupt_method);
    sensitive << i_interrupt;
    dont_initialize();
}

tb::~tb() {
    delete npu_inst;
}

void tb::end_of_elaboration() {
    // stable => reset=0, start_micro=0
    o_reset.write(false);
    o_start_micro.write(false);
    std::cout << "[" << name() << "] end_of_elaboration => o_reset=0, o_start_micro=0 at "
              << sc_time_stamp() << std::endl;
}

void tb::handle_interrupt_method() {
    int val = i_interrupt.read();
    std::cout << "[" << name() << "] handle_interrupt_method => i_interrupt="
              << val << " at " << sc_time_stamp() << std::endl;
    if (val == NPU::INT_DONE) {
        m_done_count++;
    } else if (val == NPU::INT_ERROR) {
        m_error_count++;
    }
    m_interrupt_event.notify();
}

void tb::basic_test() {
    // Wait 10 ns
    wait(10, SC_NS);

    // Drive reset =>1
    o_reset.write(true);
    wait(5, SC_NS);
    o_reset.write(false);

    // Program SFRs
    npu_inst->write(IFM_INFO_OFFSET, 0xABCD0000);
    npu_inst->write(WT_INFO_OFFSET,  0x11110000);
    npu_inst->write(OFM_INFO_OFFSET, 0xDEAD0000);

    // Check interrupt => should be IDLE
    int intr_val = i_interrupt.read();
    std::cout << "[" << name() << "] basic_test => after SFR writes, i_interrupt="
              << intr_val << " (expect IDLE=0) at " << sc_time_stamp() << std::endl;

    // Try to trigger NPU by writing CTRL=1 while start_micro=0
    npu_inst->write(CTRL_OFFSET, 0x1);
    // Wait a little => check STATUS => should remain 0 => idle
    wait(1, SC_NS);
    uint32_t stVal;
    npu_inst->read(STATUS_OFFSET, stVal);
    std::cout << "[" << name() << "] basic_test => wrote ctrl=1 with start_micro=0 => STATUS=0x"
              << std::hex << stVal << std::dec << " (expect 0) at " << sc_time_stamp() << std::endl;

    // Now set start_micro=1 => enabling the method
    o_start_micro.write(true);
    wait(1, SC_NS);
    // Write ctrl=1 => should trigger done
    npu_inst->write(CTRL_OFFSET, 0x1);
    wait(m_interrupt_event);
    std::cout << "[" << name() << "] basic_test => got interrupt => done_count="
              << m_done_count << ", error_count=" << m_error_count
              << " at " << sc_time_stamp() << std::endl;

    // Clear interrupt => write 1 to bit0 in int_clear
    npu_inst->write(INT_CLEAR_OFFSET, 0x1);
    npu_inst->read(STATUS_OFFSET, stVal);
    std::cout << "[" << name() << "] basic_test => after clearing, STATUS=0x"
              << std::hex << stVal << std::dec << " at " << sc_time_stamp() << std::endl;

    wait(10, SC_NS);
}

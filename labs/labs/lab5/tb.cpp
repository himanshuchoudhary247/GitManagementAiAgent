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

    // Bind npu ports
    npu_inst->i_reset(reset_sig);
    npu_inst->o_interrupt(intr_sig);

    // Bind tb ports to internal signals
    o_reset(reset_sig);
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
    // stable => reset=0
    o_reset.write(false);
    std::cout << "[" << name() << "] end_of_elaboration => o_reset=0 at "
              << sc_time_stamp() << std::endl;
}

void tb::handle_interrupt_method() {
    int intr_val = i_interrupt.read();
    std::cout << "[" << name() << "] handle_interrupt_method => i_interrupt="
              << intr_val << " at " << sc_time_stamp() << std::endl;
    if (intr_val == NPU::INT_DONE) {
        m_done_count++;
    } else if (intr_val == NPU::INT_ERROR) {
        m_error_count++;
    }
    // Notify event so basic_test can proceed
    m_interrupt_event.notify();
}

void tb::basic_test() {
    // Wait 10 ns
    wait(10, SC_NS);

    // Drive reset => 1
    std::cout << "[" << name() << "] basic_test => drive reset=1 at "
              << sc_time_stamp() << std::endl;
    o_reset.write(true);
    wait(5, SC_NS);
    o_reset.write(false);

    // Program some SFRs
    npu_inst->write(IFM_INFO_OFFSET, 0xABCD0000);
    npu_inst->write(WT_INFO_OFFSET,  0x11110000);
    npu_inst->write(OFM_INFO_OFFSET, 0xDEAD0000);

    // Check that interrupt=IDLE
    int intr_val = i_interrupt.read();
    std::cout << "[" << name() << "] basic_test => after SFR writes, i_interrupt="
              << intr_val << " (expect IDLE=0) at " << sc_time_stamp() << std::endl;
    if (intr_val != NPU::INT_IDLE) {
        std::cout << "[" << name() << "] basic_test => ERROR: interrupt is not IDLE" << std::endl;
    }

    // Trigger => set DONE bit in STATUS
    npu_inst->write(STATUS_OFFSET, 0x1); // bit0 => DONE
    // Wait on interrupt event => handle_interrupt_method
    wait(m_interrupt_event);
    std::cout << "[" << name() << "] basic_test => m_done_count=" << m_done_count
              << ", m_error_count=" << m_error_count 
              << " after DONE interrupt at " << sc_time_stamp() << std::endl;

    // Expect m_done_count=1, m_error_count=0 => success/failure
    if (m_done_count == 1 && m_error_count == 0) {
        std::cout << "[" << name() << "] basic_test => SUCCESS: correct interrupt=DONE" << std::endl;
    } else {
        std::cout << "[" << name() << "] basic_test => FAILURE: unexpected interrupt counts" << std::endl;
    }

    // Clear interrupt => write 1 to bit0 in int_clear
    npu_inst->write(INT_CLEAR_OFFSET, 0x1);
    // read status => should be 0 => interrupt=IDLE
    uint32_t stVal;
    npu_inst->read(STATUS_OFFSET, stVal);
    std::cout << "[" << name() << "] basic_test => after clearing interrupt, STATUS=0x"
              << std::hex << stVal << std::dec << " at " << sc_time_stamp() << std::endl;
    if (stVal == 0) {
        // OK => interrupt should be IDLE
        intr_val = i_interrupt.read();
        std::cout << "[" << name() << "] basic_test => i_interrupt=" << intr_val
                  << " (expect IDLE=0) at " << sc_time_stamp() << std::endl;
    } else {
        std::cout << "[" << name() << "] basic_test => ERROR: status not cleared?" << std::endl;
    }

    wait(10, SC_NS);
}

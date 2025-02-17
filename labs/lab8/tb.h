#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "npu.h"
#include "memory.h"
#include "npu2mem.h"

/**
 * Testbench module for Lab 8.
 *
 * Instantiates one NPU and three memory modules:
 *  - mem_ifm for IFM data,
 *  - mem_wt for Weight data,
 *  - mem_ofm for OFM data.
 *
 * Uses FIFO channels to connect NPU’s FIFO outputs to memory’s FIFO inputs.
 * Programs SFRs for dimensions and memory addresses before triggering NPU.
 */
SC_MODULE(tb) {
public:
    SC_HAS_PROCESS(tb);

    tb(sc_module_name name);
    ~tb();

    void basic_test();

    // External ports.
    sc_out<bool> o_reset;        // drives i_reset in NPU
    sc_out<bool> o_start_micro;  // (if needed for later labs)
    sc_in<int>   i_interrupt;    // reads o_interrupt from NPU

    // For interrupt handling (if needed in future labs).
    sc_event m_interrupt_event;

private:
    NPU* npu_inst;
    memory* mem_ifm;
    memory* mem_wt;
    memory* mem_ofm;

    // Internal signals.
    sc_signal<bool> reset_sig;
    sc_signal<bool> start_micro_sig;
    sc_signal<int>  intr_sig;

    // FIFO channels.
    sc_fifo<npu2mem> fifo_ifm;
    sc_fifo<npu2mem> fifo_wt;
    sc_fifo<npu2mem> fifo_ofm;
};

#endif // TB_H

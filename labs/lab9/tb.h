#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "npu.h"
#include "memory.h"
#include "npu2mem.h"

/**
 * Testbench module for Lab 9.
 *
 * Instantiates one NPU and three Memory modules:
 *   - mem_ifm for IFM data (FIFO vector size 2),
 *   - mem_wt for Weight data (FIFO vector size 1),
 *   - mem_ofm for OFM data (FIFO vector size 2).
 *
 * Binds FIFO channels between NPU and memory modules.
 */
SC_MODULE(tb) {
public:
    SC_HAS_PROCESS(tb);

    tb(sc_module_name name);
    ~tb();

    void basic_test();

private:
    NPU* npu_inst;
    memory* mem_ifm;
    memory* mem_wt;
    memory* mem_ofm;

    // Internal signals for reset and interrupt.
    sc_signal<bool> reset_sig;
    sc_signal<int> intr_sig;

    // FIFO channels as sc_vector.
    sc_vector< sc_fifo<npu2mem> > fifo_ifm; // size 2
    sc_vector< sc_fifo<npu2mem> > fifo_wt;  // size 1
    sc_vector< sc_fifo<npu2mem> > fifo_ofm; // size 2
};

#endif // TB_H

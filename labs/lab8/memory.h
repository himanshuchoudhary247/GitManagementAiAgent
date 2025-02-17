#ifndef MEMORY_H
#define MEMORY_H

#include <systemc.h>
#include "npu2mem.h"

/**
 * Memory module.
 *
 * Ports:
 *   - i_reset (bool) for reset.
 *   - i_npu2mem: FIFO input of type npu2mem.
 *
 * When a transaction is received, prints its details.
 */
SC_MODULE(memory) {
public:
    SC_HAS_PROCESS(memory);

    memory(sc_module_name name);
    ~memory();

    sc_in<bool> i_reset;
    sc_fifo_in<npu2mem> i_npu2mem;

    void process_fifo();
};

#endif // MEMORY_H

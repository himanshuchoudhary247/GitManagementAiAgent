#ifndef MEMORY_H
#define MEMORY_H

#include <systemc.h>
#include "npu2mem.h"

// Memory module definition for Lab 9.
// The FIFO input interface is now an sc_vector of ports. The vector size is passed as a constructor argument.
SC_MODULE(memory) {
public:
    SC_HAS_PROCESS(memory);

    // Constructor: "size" is the number of FIFO ports.
    memory(sc_module_name name, unsigned int size);
    ~memory();

    // sc_vector of FIFO input ports.
    sc_vector< sc_fifo_in<npu2mem> > i_npu2mem;

    // Process: function to process FIFO for a given index.
    void process_fifo(unsigned int index);
};

#endif // MEMORY_H

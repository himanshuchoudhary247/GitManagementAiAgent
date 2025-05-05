#include "memory.h"
#include <iostream>

memory::memory(sc_module_name name)
    : sc_module(name)
{
    SC_THREAD(process_fifo);
}

memory::~memory() {
    // Nothing to free.
}

void memory::process_fifo() {
    while (true) {
        npu2mem trans;
        i_npu2mem.read(trans);
        std::cout << "[" << name() << "] Memory received transaction: " 
                  << trans << " at " << sc_time_stamp() << std::endl;
    }
}

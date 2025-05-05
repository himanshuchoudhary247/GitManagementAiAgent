#define SC_INCLUDE_DYNAMIC_PROCESSES
#include "memory.h"
#include <systemc.h>
#include <iostream>
#include <string>

memory::memory(sc_module_name name, unsigned int size)
    : sc_module(name),
      i_npu2mem("i_npu2mem", size)
{
    for (unsigned int i = 0; i < size; i++) {
        std::string proc_name = "process_fifo_" + std::to_string(i);
        sc_core::sc_spawn([this, i]() { this->process_fifo(i); }, proc_name.c_str());
    }
}

memory::~memory() {
    // Nothing to free.
}

void memory::process_fifo(unsigned int index) {
    while (true) {
        npu2mem trans;
        i_npu2mem[index].read(trans);
        std::cout << "[" << name() << "] Memory (port " << index << ") received transaction: "
                  << trans << " at time " << sc_time_stamp() << std::endl;
    }
}

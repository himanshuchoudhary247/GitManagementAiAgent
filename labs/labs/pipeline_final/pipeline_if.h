#ifndef PIPELINE_IF_H
#define PIPELINE_IF_H

#include <systemc.h>

struct PipelineIF : public sc_module {
  sc_fifo<sc_uint<32>> fifo;

  SC_CTOR(PipelineIF) : fifo("fifo", 10) {} // FIFO depth = 10
};

#endif // PIPELINE_IF_H

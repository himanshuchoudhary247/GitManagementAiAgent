#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "pipeline.h"

// Testbench uses only two FIFO channels: one for input and one for output.
SC_MODULE(TB) {
  // FIFO channels.
  sc_fifo<sc_uint<32>> input_fifo;
  sc_fifo<sc_uint<32>> output_fifo;

  // Pipeline instance.
  Pipeline* pipeline_inst;

  SC_CTOR(TB)
    : input_fifo("input_fifo", 10),   // Size 10 (arbitrary)
      output_fifo("output_fifo", 10)
  {
    // Create the pipeline with 5 stages and a cycle time of 10 ns.
    pipeline_inst = new Pipeline("pipeline", 5, sc_time(10, SC_NS));

    // Bind the pipeline's FIFO ports.
    pipeline_inst->in_port(input_fifo);
    pipeline_inst->out_port(output_fifo);

    SC_THREAD(producer);
    SC_THREAD(consumer);
  }

  ~TB() {
    delete pipeline_inst;
  }

  void producer();
  void consumer();
};

#endif // TB_H

#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "pipeline.h"

SC_MODULE(TB) {
  // Signals to connect to the Pipeline.
  sc_signal<sc_uint<32>> sig_in_data;
  sc_signal<bool>        sig_in_valid;
  sc_signal<bool>        sig_in_ready;
  
  sc_signal<sc_uint<32>> sig_out_data;
  sc_signal<bool>        sig_out_valid;
  sc_signal<bool>        sig_out_ready;

  // Pipeline instance.
  Pipeline* pipeline_inst;

  SC_CTOR(TB) {
    // Create a Pipeline with 5 stages and a cycle time of 10 ns.
    pipeline_inst = new Pipeline("pipeline", 5, sc_time(10, SC_NS));

    // Bind ports.
    pipeline_inst->in_data(sig_in_data);
    pipeline_inst->in_valid(sig_in_valid);
    pipeline_inst->in_ready(sig_in_ready);

    pipeline_inst->out_data(sig_out_data);
    pipeline_inst->out_valid(sig_out_valid);
    pipeline_inst->out_ready(sig_out_ready);

    // Create producer and consumer threads.
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

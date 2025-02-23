#ifndef PIPELINE_H
#define PIPELINE_H

#include <systemc.h>

SC_MODULE(Pipeline) {
  // Input ports
  sc_in<sc_uint<32>> in_data;
  sc_in<bool>        in_valid;
  sc_out<bool>       in_ready;

  // Output ports
  sc_out<sc_uint<32>> out_data;
  sc_out<bool>        out_valid;
  sc_in<bool>         out_ready;

  // Pipeline parameters
  unsigned int num_stages;   // e.g., 5
  sc_time cycle_time;        // e.g., 10 ns per cycle
  sc_time pipeline_delay;    // = num_stages * cycle_time

  // FIFO for storing in-flight data (depth = num_stages)
  sc_fifo<sc_uint<32>> fifo;

  // Event queue for scheduling output events
  sc_event_queue output_schedule;

  SC_HAS_PROCESS(Pipeline);

  // Constructor
  Pipeline(sc_module_name name, unsigned int stages, sc_time clk);

  // Threads for handling input and output
  void input_thread();
  void output_thread();
};

#endif // PIPELINE_H

#ifndef PIPELINE_H
#define PIPELINE_H

#include <systemc.h>

SC_MODULE(Pipeline) {
  // FIFO ports for minimal interface:
  sc_fifo_in<sc_uint<32>>  in_port;   // Input data port
  sc_fifo_out<sc_uint<32>> out_port;  // Output data port

  // Pipeline parameters.
  unsigned int num_stages;   // e.g., 5 stages.
  sc_time cycle_time;        // e.g., 10 ns per stage.
  sc_time pipeline_delay;    // = num_stages * cycle_time.

  // Internal FIFO for buffering in-flight data.
  sc_fifo<sc_uint<32>> fifo;

  // Event queue to schedule output events.
  sc_event_queue output_schedule;

  // Counter for number of items that have completed the delay.
  int available_count;

  SC_HAS_PROCESS(Pipeline);

  // Constructor.
  Pipeline(sc_module_name name, unsigned int stages, sc_time clk);

  // Processes:
  void input_thread();           // Reads from in_port and writes to internal fifo.
  void update_available_count(); // Increments available_count when an output event triggers.
  void output_thread();          // Writes data from internal fifo to out_port based on available_count.
};

#endif // PIPELINE_H

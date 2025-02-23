#include "pipeline.h"
#include <iostream>
using namespace std;

Pipeline::Pipeline(sc_module_name name, unsigned int stages, sc_time clk)
    : sc_module(name)
    , num_stages(stages)
    , cycle_time(clk)
    , pipeline_delay(stages * clk)
    , fifo("fifo", stages)  // FIFO depth equals number of stages
{
  SC_THREAD(input_thread);
  SC_THREAD(output_thread);
}

void Pipeline::input_thread() {
  while (true) {
    // Set in_ready high if there is free space in the FIFO.
    in_ready.write(fifo.num_free() > 0);

    // If valid data is available and FIFO is not full, write it.
    if (in_valid.read() && fifo.num_free() > 0) {
      sc_uint<32> data = in_data.read();
      fifo.write(data);  // Blocking write into FIFO

      // Schedule an event for output after the full pipeline delay.
      output_schedule.notify(pipeline_delay);

      cout << "[Pipeline] Data " << data << " entered at " 
           << sc_time_stamp() << ", will be available at " 
           << sc_time_stamp() + pipeline_delay << endl;
    }
    
    // Check for new data frequently (a fraction of the cycle time).
    wait(cycle_time / 10);
  }
}

void Pipeline::output_thread() {
  out_valid.write(false);

  while (true) {
    // Wait for the next scheduled event from the event queue.
    wait(output_schedule.default_event());

    // If FIFO has data and consumer is ready, output one data item.
    if (fifo.num_available() > 0 && out_ready.read()) {
      sc_uint<32> data = fifo.read();
      out_data.write(data);
      out_valid.write(true);

      cout << "[Pipeline] Data " << data << " exiting at " << sc_time_stamp() << endl;

      // Data remains valid for one cycle.
      wait(cycle_time);
      out_valid.write(false);
    } else {
      // If the consumer is not ready or no data is available, wait one cycle and try again.
      wait(cycle_time);
    }
  }
}

#include "pipeline.h"
#include <iostream>
using namespace std;

Pipeline::Pipeline(sc_module_name name, unsigned int stages, sc_time clk)
    : sc_module(name)
    , num_stages(stages)
    , cycle_time(clk)
    , pipeline_delay(stages * clk)
    , fifo("fifo", stages)  // FIFO depth equals number of stages.
    , available_count(0)
{
  // Create three processes:
  SC_THREAD(input_thread);
  SC_THREAD(output_thread);
  SC_METHOD(update_available_count);
    sensitive << output_schedule.default_event();
    dont_initialize();
}

void Pipeline::input_thread() {
  while (true) {
    // in_ready is high if there is free space in the FIFO.
    in_ready.write(fifo.num_free() > 0);

    if (in_valid.read() && fifo.num_free() > 0) {
      sc_uint<32> data = in_data.read();
      fifo.write(data);  // Write data into FIFO.

      // Instead of incrementing available_count here,
      // we schedule an output event after the full pipeline delay.
      output_schedule.notify(pipeline_delay);

      cout << "[Pipeline] Data " << data << " entered at " << sc_time_stamp()
           << ", will be available at " << sc_time_stamp() + pipeline_delay << endl;
    }
    
    // Check frequently (a fraction of a cycle) for new input.
    wait(cycle_time / 10);
  }
}

void Pipeline::update_available_count() {
  // This method is invoked whenever output_schedule notifies.
  // Each notification indicates that one more data item has
  // finished the pipeline delay and is now available for output.
  available_count++;
  // For debugging, print the updated available_count.
  cout << "[Pipeline] update_available_count: available_count now " 
       << available_count << " at " << sc_time_stamp() << endl;
}

void Pipeline::output_thread() {
  out_valid.write(false);

  while (true) {
    // If available_count is positive, attempt to output data.
    if (available_count > 0) {
      // If the consumer is ready and data exists in FIFO, output it.
      if (fifo.num_available() > 0 && out_ready.read()) {
        sc_uint<32> data = fifo.read();
        out_data.write(data);
        out_valid.write(true);

        cout << "[Pipeline] Data " << data << " exiting at " 
             << sc_time_stamp() << endl;

        // Decrement the count as we are consuming one item.
        available_count--;

        // Data remains valid for one cycle.
        wait(cycle_time);
        out_valid.write(false);
      } else {
        // If consumer is not ready, wait one cycle.
        wait(cycle_time);
      }
    } else {
      // If no data is available (i.e., available_count == 0), wait a short while.
      wait(cycle_time / 10);
    }
  }
}

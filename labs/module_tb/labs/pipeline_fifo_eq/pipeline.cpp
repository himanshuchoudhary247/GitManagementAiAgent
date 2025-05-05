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
  SC_THREAD(input_thread);
  SC_THREAD(output_thread);
  SC_METHOD(update_available_count);
    sensitive << output_schedule.default_event();
    dont_initialize();
}

void Pipeline::input_thread() {
  while (true) {
    in_ready.write(fifo.num_free() > 0);

    if (in_valid.read() && fifo.num_free() > 0) {
      sc_uint<32> data = in_data.read();
      fifo.write(data);

      // Schedule an output event after the full pipeline delay.
      output_schedule.notify(pipeline_delay);

      cout << "[Pipeline] Data " << data << " entered at " 
           << sc_time_stamp() << ", will be available at " 
           << sc_time_stamp() + pipeline_delay << endl;
    }

    wait(cycle_time / 10);
  }
}

void Pipeline::update_available_count() {
  // Increment available_count when an output event triggers.
  available_count++;
  cout << "[Pipeline] update_available_count: available_count now " 
       << available_count << " at " << sc_time_stamp() << endl;
}

void Pipeline::output_thread() {
  out_valid.write(false);

  while (true) {
    if (available_count > 0) {
      if (fifo.num_available() > 0 && out_ready.read()) {
        sc_uint<32> data = fifo.read();
        out_data.write(data);
        out_valid.write(true);

        cout << "[Pipeline] Data " << data << " exiting at " 
             << sc_time_stamp() << endl;

        available_count--;

        wait(cycle_time);
        out_valid.write(false);
      } else {
        wait(cycle_time);
      }
    } else {
      wait(cycle_time / 10);
    }
  }
}

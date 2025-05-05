#include "pipeline.h"
#include <iostream>
using namespace std;

Pipeline::Pipeline(sc_module_name name, unsigned int stages, sc_time clk)
    : sc_module(name)
    , num_stages(stages)
    , cycle_time(clk)
    , pipeline_delay(stages * clk)
    , fifo("fifo", stages)  // Internal FIFO depth equals number of stages.
    , available_count(0)
{
  SC_THREAD(input_thread);
  SC_THREAD(output_thread);
  // SC_METHOD to update available_count when an event is triggered.
  SC_METHOD(update_available_count);
    sensitive << output_schedule.default_event();
    dont_initialize();
}

void Pipeline::input_thread() {
  while (true) {
    // Blocking read from the input FIFO port.
    sc_uint<32> data = in_port.read();
    // Write data into the internal FIFO.
    fifo.write(data);
    // Schedule an output event after the full pipeline delay.
    output_schedule.notify(pipeline_delay);
    cout << "[Pipeline] Data " << data << " entered at " 
         << sc_time_stamp() << ", available at " 
         << sc_time_stamp() + pipeline_delay << endl;
  }
}

void Pipeline::update_available_count() {
  // This SC_METHOD is triggered whenever the event queue notifies.
  // It increments available_count to indicate that one more item has completed the delay.
  available_count++;
//   cout << "[Pipeline] update_available_count: available_count now " 
//        << available_count << " at " << sc_time_stamp() << endl;
}

void Pipeline::output_thread() {
  while (true) {
    if (available_count > 0) {
      if (fifo.num_available() > 0) {
        sc_uint<32> data = fifo.read();
        out_port.write(data);
        available_count--;
        cout << "[Pipeline] Data " << data << " output at " 
             << sc_time_stamp() << endl;
      }
    } else {
      // If no data is available for output, wait a fraction of a cycle.
      wait(cycle_time);
    }
  }
}

#define SC_INCLUDE_DYNAMIC_PROCESSES
#include "pipeline.h"
#include <iostream>

pipeline::pipeline(sc_module_name name, unsigned int capacity, sc_time cycle_time)
    : sc_module(name), n(capacity), T(cycle_time)
{
    SC_THREAD(pipeline_thread);
}

void pipeline::pipeline_thread() {
    // Initialize
    in_ready.write(true);
    out_valid.write(false);

    while (true) {
        wait(T); // pipeline cycle

        // 1) If queue not empty => we have data to output
        if (!queue.empty()) {
            // If out_ready is true, output front data and pop
            if (out_ready.read()) {
                out_data.write(queue.front());
                out_valid.write(true);
                queue.pop_front();
            } else {
                // out_ready= false => keep out_valid= true, hold item
                out_data.write(queue.front());
                out_valid.write(true);
            }
        } else {
            // queue empty => no data to output
            out_valid.write(false);
        }

        // 2) If queue not full => possibly read new input
        if (queue.size() < n) {
            in_ready.write(true);
            if (in_valid.read()) {
                // push data into queue
                queue.push_back(in_data.read());
            }
        } else {
            // queue is full
            in_ready.write(false);
        }
    }
}

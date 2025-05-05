#include "pipeline_stage.h"

void PipelineStage::process() {
    while (true) {
        // Blocking read: wait until data is available.
        sc_uint<32> data = in_fifo.read();
        wait(cycle_time); // simulate one stage delay (e.g., one clock cycle)
        // Blocking write: will wait if the downstream FIFO is full.
        out_fifo.write(data);
    }
}

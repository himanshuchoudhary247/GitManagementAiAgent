#ifndef PIPELINE_CHAIN_H
#define PIPELINE_CHAIN_H

#include <systemc.h>
#include "pipeline_stage.h"

SC_MODULE(PipelineChain) {
    // Number of pipeline stages.
    unsigned int num_stages;
    sc_time cycle_time;

    // Array of FIFO pointers (one more than the number of stages).
    sc_fifo<sc_uint<32>> **fifos;
    // Array of pipeline stage pointers.
    PipelineStage **stages;

    SC_HAS_PROCESS(PipelineChain);

    PipelineChain(sc_module_name name, unsigned int stages_num, sc_time t);
    ~PipelineChain();

    // Expose the input and output FIFO channels.
    sc_fifo<sc_uint<32>>& in();
    sc_fifo<sc_uint<32>>& out();
};

#endif // PIPELINE_CHAIN_H

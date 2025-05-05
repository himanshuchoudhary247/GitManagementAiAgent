#include "pipeline_chain.h"
#include <cstdio>

PipelineChain::PipelineChain(sc_module_name name, unsigned int stages_num, sc_time t)
: sc_module(name), num_stages(stages_num), cycle_time(t)
{
    // Create an array of FIFO pointers. We need (num_stages+1) FIFOs.
    fifos = new sc_fifo<sc_uint<32>>*[num_stages+1];
    for (unsigned int i = 0; i < num_stages+1; i++) {
        char fifo_name[20];
        sprintf(fifo_name, "fifo_%d", i);
        // Create each FIFO with depth=1 (acts like a register)
        fifos[i] = new sc_fifo<sc_uint<32>>(fifo_name, 1);
    }
    // Create pipeline stages and connect the FIFOs.
    stages = new PipelineStage*[num_stages];
    for (unsigned int i = 0; i < num_stages; i++) {
        char stage_name[20];
        sprintf(stage_name, "stage_%d", i);
        stages[i] = new PipelineStage(stage_name);
        stages[i]->cycle_time = cycle_time;
        stages[i]->in_fifo(*fifos[i]);
        stages[i]->out_fifo(*fifos[i+1]);
    }
}

PipelineChain::~PipelineChain() {
    // Delete pipeline stages.
    for (unsigned int i = 0; i < num_stages; i++) {
        delete stages[i];
    }
    delete[] stages;
    // Delete FIFOs.
    for (unsigned int i = 0; i < num_stages+1; i++) {
        delete fifos[i];
    }
    delete[] fifos;
}

sc_fifo<sc_uint<32>>& PipelineChain::in() {
    return *fifos[0];
}

sc_fifo<sc_uint<32>>& PipelineChain::out() {
    return *fifos[num_stages];
}

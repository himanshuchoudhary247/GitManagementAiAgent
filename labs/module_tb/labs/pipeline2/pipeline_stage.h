#ifndef PIPELINE_STAGE_H
#define PIPELINE_STAGE_H

#include <systemc.h>

SC_MODULE(PipelineStage) {
    // Input and output FIFO ports.
    sc_fifo_in<sc_uint<32>>  in_fifo;
    sc_fifo_out<sc_uint<32>> out_fifo;
    
    // Delay per stage (typically one clock cycle).
    sc_time cycle_time;

    SC_CTOR(PipelineStage)
    : cycle_time(SC_ZERO_TIME)
    {
        SC_THREAD(process);
    }

    void process();
};

#endif // PIPELINE_STAGE_H

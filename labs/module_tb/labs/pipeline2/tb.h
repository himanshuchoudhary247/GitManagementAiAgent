#ifndef TESTBENCH_H
#define TESTBENCH_H

#include <systemc.h>
#include "pipeline_chain.h"

SC_MODULE(Testbench) {
    PipelineChain* pipeline_chain;

    SC_CTOR(Testbench);
    ~Testbench();

    void producer();
    void consumer();
};

#endif // TESTBENCH_H

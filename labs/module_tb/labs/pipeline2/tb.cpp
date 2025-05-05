#include "tb.h"
#include <iostream>
using namespace std;

Testbench::Testbench(sc_module_name name)
: sc_module(name)
{
    // Create a pipeline chain with 4 stages and 10 ns delay per stage.
    pipeline_chain = new PipelineChain("pipeline_chain", 4, sc_time(10, SC_NS));

    SC_THREAD(producer);
    SC_THREAD(consumer);
}

Testbench::~Testbench() {
    delete pipeline_chain;
}

void Testbench::producer() {
    // Wait a bit before starting.
    wait(10, SC_NS);
    cout << "[TB] Producer sending first data 101 at " << sc_time_stamp() << endl;
    // Write the first data word.
    pipeline_chain->in().write(101);

    // Wait enough time for the first data to propagate through all stages.
    wait(50, SC_NS);

    // Send a burst of data.
    for (int i = 0; i < 5; i++) {
        sc_uint<32> data = 200 + i;
        cout << "[TB] Producer sending burst data " << data << " at " << sc_time_stamp() << endl;
        pipeline_chain->in().write(data);
        wait(5, SC_NS); // Small gap between burst data words.
    }
    cout << "[TB] Producer finished burst data at " << sc_time_stamp() << endl;
}

void Testbench::consumer() {
    while (true) {
        // Blocking read from the output FIFO.
        sc_uint<32> data = pipeline_chain->out().read();
        cout << "[TB] Consumer received data " << data << " at " << sc_time_stamp() << endl;
        wait(1, SC_NS); // Simulate some processing delay if needed.
    }
}

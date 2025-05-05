#include "tb.h"
#include <iostream>
#include <cstdlib> // for rand()

tb::tb(sc_module_name name, unsigned int capacity, sc_time cycle_time)
    : sc_module(name)
{
    // Instantiate pipeline
    pipe_inst = new pipeline("pipe_inst", capacity, cycle_time);

    // Bind pipeline I/O
    pipe_inst->in_data(sig_in_data);
    pipe_inst->in_valid(sig_in_valid);
    pipe_inst->in_ready(sig_in_ready);

    pipe_inst->out_data(sig_out_data);
    pipe_inst->out_valid(sig_out_valid);
    pipe_inst->out_ready(sig_out_ready);

    // Producer and consumer threads
    SC_THREAD(producer_thread);
    SC_THREAD(consumer_thread);
}

tb::~tb() {
    delete pipe_inst;
}

void tb::producer_thread() {
    // Initialize signals
    sig_in_valid.write(false);
    sig_in_data.write(0);

    wait(10, SC_NS);

    // Send 12 data words with random burstiness
    for (unsigned int i = 0; i < 12; i++) {
        // Wait for pipeline to be ready
        while (!sig_in_ready.read()) {
            wait(1, SC_NS);
        }
        // Drive data & valid
        sig_in_data.write(i + 100);
        sig_in_valid.write(true);

        // Wait 1 cycle
        wait(1, SC_NS);

        // Deassert valid
        sig_in_valid.write(false);

        // Random gap
        wait(rand() % 5 + 1, SC_NS);
    }

    // Finished
    std::cout << "[TB] Producer done sending at " << sc_time_stamp() << std::endl;
}

void tb::consumer_thread() {
    // Initially always ready
    sig_out_ready.write(true);

    while (true) {
        wait(1, SC_NS);

        if (sig_out_valid.read() == true) {
            sc_uint<32> val = sig_out_data.read();
            std::cout << "[TB] Consumer got " << val
                      << " at time " << sc_time_stamp() << std::endl;

            // Sometimes the consumer stops being ready for a few cycles
            if ((rand() % 4) == 0) {
                sig_out_ready.write(false);
                wait(rand() % 5 + 1, SC_NS);
                sig_out_ready.write(true);
            }
        }
    }
}

#ifndef PIPELINE_H
#define PIPELINE_H

#include <systemc.h>
#include <deque>

/**
 * \brief A queue-based pipeline module with capacity n and cycle time T.
 *
 * The pipeline stores data in a bounded std::deque (size = n). Each cycle T:
 *   - If queue is not empty and out_ready is true, pop front and produce out_data.
 *   - If queue is not empty and out_ready is false, keep out_valid = true with the same front data.
 *   - If queue is not full and in_valid is true, push new data onto the queue.
 *   - If queue is full, set in_ready = false to block input.
 *   - If queue is empty, set out_valid = false (no data to output).
 *
 * Ports:
 *   - in_data, in_valid, in_ready
 *   - out_data, out_valid, out_ready
 *
 * Constructor args:
 *   - n:   capacity (max number of items in the queue)
 *   - T:   cycle time (sc_time)
 */
SC_MODULE(pipeline) {
public:
    // Constructor parameters
    unsigned int n;   // pipeline capacity
    sc_time T;        // cycle time

    // Ports
    sc_in<sc_uint<32>>  in_data;
    sc_in<bool>         in_valid;
    sc_out<bool>        in_ready;

    sc_out<sc_uint<32>> out_data;
    sc_out<bool>        out_valid;
    sc_in<bool>         out_ready;

    SC_HAS_PROCESS(pipeline);

    pipeline(sc_module_name name, unsigned int capacity, sc_time cycle_time);

private:
    // Deque to store data
    std::deque< sc_uint<32> > queue;

    void pipeline_thread();
};

#endif // PIPELINE_H

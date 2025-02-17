#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "pipeline.h"

/**
 * \brief A testbench that drives data into the pipeline in a bursty manner
 *        and reads data from the pipeline's output.
 */
SC_MODULE(tb) {
public:
    // Pipeline instance
    pipeline* pipe_inst;

    // Signals
    sc_signal<sc_uint<32>> sig_in_data;
    sc_signal<bool>        sig_in_valid;
    sc_signal<bool>        sig_in_ready;

    sc_signal<sc_uint<32>> sig_out_data;
    sc_signal<bool>        sig_out_valid;
    sc_signal<bool>        sig_out_ready;

    SC_HAS_PROCESS(tb);

    tb(sc_module_name name, unsigned int capacity, sc_time cycle_time);
    ~tb();

private:
    void producer_thread();
    void consumer_thread();
};

#endif // TB_H

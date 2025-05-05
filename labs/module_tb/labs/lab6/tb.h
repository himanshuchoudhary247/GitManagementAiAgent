#ifndef TB_H
#define TB_H

#include <systemc.h>
#include "npu.h"

SC_MODULE(tb) {
public:
    SC_HAS_PROCESS(tb);

    tb(sc_module_name name);
    ~tb();

    // Ports
    sc_out<bool> o_reset;        // drives i_reset in NPU
    sc_out<bool> o_start_micro;  // drives i_start_micro in NPU
    sc_in<int>   i_interrupt;    // reads o_interrupt from NPU

    void basic_test();
    void handle_interrupt_method();
    void end_of_elaboration();

    int m_done_count;
    int m_error_count;
    sc_event m_interrupt_event;

private:
    NPU* npu_inst;

    // Internal signals
    sc_signal<bool> reset_sig;
    sc_signal<bool> start_micro_sig;
    sc_signal<int>  intr_sig;
};

#endif // TB_H

#ifndef NPU_H
#define NPU_H

#include <systemc.h>
#include "sfr.h"

/**
 * NPU module (Lab 6)
 * - Instantiates 6 SFRs (ctrl, status, int_clear, ifm_info, wt_info, ofm_info)
 * - i_reset (bool), o_interrupt (int), i_start_micro (bool)
 * - If i_start_micro=0 => disable start operation; writing to CTRL won't trigger done
 * - If i_start_micro=1 => enable start operation; writing 1 to CTRL triggers done bit
 */
SC_MODULE(NPU) {
public:
    // Interrupt states
    static const int INT_IDLE  ;
    static const int INT_DONE  ;
    static const int INT_ERROR ;

    SC_HAS_PROCESS(NPU);

    NPU(sc_module_name name);
    ~NPU();

    // SFR read/write
    void write(uint32_t offset, const uint32_t &value);
    void read(uint32_t offset, uint32_t &value) const;

    // Ports
    sc_in<bool> i_reset;        // reset
    sc_out<int> o_interrupt;    // interrupt
    sc_in<bool> i_start_micro;  // micro-start control

    void end_of_elaboration();

private:
    // SFR pointers
    SFR* ctrl;
    SFR* status;
    SFR* int_clear;
    SFR* ifm_info;
    SFR* wt_info;
    SFR* ofm_info;

    // handle_reset => on posedge i_reset
    void handle_reset();
    // check_done_bit => sets interrupt=INT_DONE if bit0 is set
    void check_done_bit(uint32_t val);
    // handle_int_clear => clearing bits in status
    void handle_int_clear(uint32_t val);

    // SC_METHOD to watch i_start_micro => sets m_enable_start
    void check_start_micro();
    bool m_enable_start;
};

#endif // NPU_H

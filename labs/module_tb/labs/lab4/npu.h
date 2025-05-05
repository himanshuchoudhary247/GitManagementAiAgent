#ifndef NPU_H
#define NPU_H

#include <systemc.h>
#include "sfr.h"

/**
 * NPU module for Lab 4.
 * - Instantiates 5 SFR modules
 * - Has an input port i_reset
 * - Has an output port o_interrupt (3 states: IDLE=0, DONE=1, ERROR=2)
 * - end_of_elaboration sets o_interrupt to IDLE (active low)
 * - SC_METHOD handle_reset: sensitive to posedge of i_reset
 * - Once done bit is set in status SFR, set o_interrupt=DONE
 */
SC_MODULE(NPU) {
public:
    // We define possible interrupt states as constants
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
    sc_in<bool> i_reset;          // reset input
    sc_out<int> o_interrupt;      // interrupt output

    // end_of_elaboration
    void end_of_elaboration();

private:
    // SFR pointers
    SFR* ctrl;
    SFR* status;
    SFR* ifm_info;
    SFR* wt_info;
    SFR* ofm_info;

    // SC_METHOD to handle reset
    void handle_reset();

    // Helper function to check if DONE bit is set
    void check_done_bit(const uint32_t &val);
};

#endif // NPU_H

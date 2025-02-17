#ifndef NPU_H
#define NPU_H

#include <systemc.h>
#include "sfr.h"

/**
 * NPU module (Lab 5)
 * - Instantiates 6 SFRs now (including int_clear).
 * - Has i_reset port, o_interrupt port
 * - handle_reset method on posedge i_reset => resets SFRs, sets interrupt=IDLE
 * - On writing to STATUS => if DONE bit is set => o_interrupt=DONE
 * - On writing to int_clear => if bits are set => clear corresponding bits in STATUS
 *   if all bits in STATUS=0 => o_interrupt=IDLE
 */
SC_MODULE(NPU) {
public:
    // Interrupt states
    static const int INT_IDLE ;
    static const int INT_DONE ;
    static const int INT_ERROR ;

    SC_HAS_PROCESS(NPU);

    NPU(sc_module_name name);
    ~NPU();

    // SFR read/write
    void write(uint32_t offset, const uint32_t &value);
    void read(uint32_t offset, uint32_t &value) const;

    // Ports
    sc_in<bool> i_reset;
    sc_out<int> o_interrupt;

    void end_of_elaboration();

private:
    // SFR pointers
    SFR* ctrl;
    SFR* status;
    SFR* int_clear; // new SFR for Lab 5
    SFR* ifm_info;
    SFR* wt_info;
    SFR* ofm_info;

    // handle_reset => on posedge i_reset
    void handle_reset();
    // check_done_bit => sets interrupt=INT_DONE if bit 0 is set
    void check_done_bit(uint32_t val);
    // handle_int_clear => if bits are set => clear corresponding bits in STATUS
    void handle_int_clear(uint32_t val);
};

#endif // NPU_H

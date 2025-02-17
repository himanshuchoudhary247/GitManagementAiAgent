#ifndef NPU_H
#define NPU_H

#include <systemc.h>
#include "sfr.h"
#include "npu2mem.h"

/**
 * NPU module for Lab 9.
 *
 * Instantiates six SFRs:
 *  - ctrl       (offset 0x00000000)
 *  - status     (offset 0x00000004)
 *  - int_clear  (offset 0x00000008)
 *  - ifm_info   (offset 0x00000010)
 *  - wt_info    (offset 0x00000014)
 *  - ofm_info   (offset 0x00000018)
 *
 * Provides three FIFO output ports implemented as sc_vector:
 *  - o_npu2mem_ifm: for IFM read transactions (vector size = 2, round-robin)
 *  - o_npu2mem_wt:  for Weight read transactions (vector size = 1)
 *  - o_npu2mem_ofm: for OFM write transactions (vector size = 2, round-robin)
 *
 * Also has reset (i_reset) and interrupt (o_interrupt) ports.
 *
 * When triggered (writing 1 to CTRL), start_operation() performs:
 *   - Wait 10 ns, then read IFM address from ifm_info and send a read transaction on FIFO index = ifm_counter % 2.
 *   - Wait 10 ns, then read Weight address from wt_info and send a read transaction on the single FIFO.
 *   - Wait 10 ns, then read OFM address from ofm_info and send a write transaction on FIFO index = ofm_counter % 2.
 *   - Wait 10 ns, then assert interrupt (set DONE bit in status and write INT_DONE to o_interrupt).
 *
 * Interrupt state constants are defined in this header and in npu.cpp.
 */
SC_MODULE(NPU) {
public:
    SC_HAS_PROCESS(NPU);

    // Interrupt state constants.
    static const int INT_IDLE;  // active low = 0
    static const int INT_DONE;  // 1
    static const int INT_ERROR; // 2

    NPU(sc_module_name name);
    ~NPU();

    void write(uint32_t offset, const uint32_t &value);
    void read(uint32_t offset, uint32_t &value) const;

    // FIFO interfaces as sc_vector.
    sc_vector< sc_fifo_out<npu2mem> > o_npu2mem_ifm; // size 2
    sc_vector< sc_fifo_out<npu2mem> > o_npu2mem_wt;  // size 1
    sc_vector< sc_fifo_out<npu2mem> > o_npu2mem_ofm; // size 2

    // Reset and interrupt ports.
    sc_in<bool> i_reset;
    sc_out<int> o_interrupt;

    // Trigger NPU operation.
    void start_operation();

    // end_of_elaboration: drive o_interrupt to INT_IDLE.
    void end_of_elaboration();

private:
    SFR* ctrl;
    SFR* status;
    SFR* int_clear;
    SFR* ifm_info;
    SFR* wt_info;
    SFR* ofm_info;

    // Round-robin counters for IFM and OFM.
    unsigned int ifm_counter;
    unsigned int ofm_counter;
};

#endif // NPU_H

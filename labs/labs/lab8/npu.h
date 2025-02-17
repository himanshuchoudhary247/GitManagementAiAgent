#ifndef NPU_H
#define NPU_H

#include <systemc.h>
#include "sfr.h"
#include "npu2mem.h"

/**
 * NPU module for Lab 8.
 *
 * It instantiates six SFRs:
 *  - ctrl       (offset 0x00000000)
 *  - status     (offset 0x00000004)
 *  - int_clear  (offset 0x00000008)
 *  - ifm_info   (offset 0x00000010)
 *  - wt_info    (offset 0x00000014)
 *  - ofm_info   (offset 0x00000018)
 *
 * It provides three FIFO output ports (o_npu2mem_ifm, o_npu2mem_wt, o_npu2mem_ofm)
 * for communicating with Memory modules.
 *
 * It also has reset and interrupt ports:
 *  - i_reset: reset input.
 *  - o_interrupt: interrupt output.
 *
 * When triggered (via writing 1 to CTRL), start_operation() performs:
 *  - 10 ns delay, then reads IFM address from ifm_info and sends a read transaction.
 *  - 10 ns delay, then reads Weight address from wt_info and sends a read transaction.
 *  - 10 ns delay, then reads OFM address from ofm_info and sends a write transaction with dummy data.
 *  - 10 ns delay, then asserts interrupt by setting DONE in status.
 *
 * If a write to int_clear is performed, it clears bits in status accordingly.
 *
 * Interrupt state constants are declared here and defined in npu.cpp.
 */
SC_MODULE(NPU) {
public:
    SC_HAS_PROCESS(NPU);

    // Declare interrupt state constants.
    static const int INT_IDLE;   // active low
    static const int INT_DONE;
    static const int INT_ERROR;

    NPU(sc_module_name name);
    ~NPU();

    void write(uint32_t offset, const uint32_t &value);
    void read(uint32_t offset, uint32_t &value) const;

    // FIFO interfaces.
    sc_fifo_out<npu2mem> o_npu2mem_ifm;
    sc_fifo_out<npu2mem> o_npu2mem_wt;
    sc_fifo_out<npu2mem> o_npu2mem_ofm;

    // Reset and interrupt ports.
    sc_in<bool> i_reset;
    sc_out<int> o_interrupt;

    // Trigger operation.
    void start_operation();

    // end_of_elaboration: sets o_interrupt to stable value (INT_IDLE).
    void end_of_elaboration();

private:
    SFR* ctrl;
    SFR* status;
    SFR* int_clear;
    SFR* ifm_info;
    SFR* wt_info;
    SFR* ofm_info;
};

#endif // NPU_H

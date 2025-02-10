#ifndef NPU_H
#define NPU_H

#include <systemc.h>
#include <cstdint>
#include <unordered_map>
#include "sfr.h"

// NPU module definition (for Lab 5, SystemC 2.3.2).
// Instantiates six SFR submodules and provides functions for SFR access.
// Also handles reset and interrupt signaling.
// A new SFR (int_clear) is used to clear/deassert the interrupt.
SC_MODULE(NPU) {
public:
    // Constructor.
    NPU(sc_module_name name);
    // Destructor.
    ~NPU();

    // SFR access functions.
    void write(uint32_t offset, const uint32_t &value);
    void read(uint32_t offset, uint32_t &value) const;
    void configure_ifm(uint32_t width, uint32_t height);
    void configure_ofm(uint32_t width, uint32_t height);
    void configure_weight(uint32_t width, uint32_t height);

    // Reset handler.
    void handle_reset();

    // end_of_elaboration callback.
    void end_of_elaboration();

    // Simulate NPU operation.
    void start_operation();

    // Ports for reset and interrupt.
    sc_in<bool>  i_reset;      // Input port for reset.
    sc_out<int>  o_interrupt;  // Output port for interrupt.

    // Interrupt state definitions.
    static const int INT_IDLE;   // e.g., 0 (active low)
    static const int INT_DONE;   // e.g., 1
    static const int INT_ERROR;  // e.g., 2

    // Public SFR offset constants (moved from private to public so tb can access them)
    static const uint32_t CTRL_OFFSET;      // 0x00000000
    static const uint32_t STATUS_OFFSET;    // 0x00000004
    static const uint32_t INT_CLEAR_OFFSET;   // 0x00000008
    static const uint32_t IFM_INFO_OFFSET;    // 0x00000010
    static const uint32_t WT_INFO_OFFSET;     // 0x00000014
    static const uint32_t OFM_INFO_OFFSET;    // 0x00000018

private:
    // SFR submodules.
    SFR *ctrl;
    SFR *status;
    SFR *int_clear; // New SFR for clearing the interrupt.
    SFR *ifm_info;
    SFR *wt_info;
    SFR *ofm_info;

    // Mapping from offset to SFR pointer.
    std::unordered_map<uint32_t, SFR*> sfr_map;
};

#endif // NPU_H

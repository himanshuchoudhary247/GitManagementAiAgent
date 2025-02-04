#ifndef NPU_H
#define NPU_H

#include <systemc.h>
#include <cstdint>
#include <unordered_map>
#include "sfr.h"

// NPU module definition.
// Instantiates six SFR submodules and provides access functions.
// Also handles reset and interrupt signaling.
// It now includes a new SFR (int_clear) used to clear the interrupt.
SC_MODULE(NPU) {
public:
    // Constructor.
    NPU(sc_core::sc_module_name name);
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

    // end_of_elaboration callback: drives the interrupt port with a stable value.
    virtual void end_of_elaboration() override;

    // Simulate NPU operation.
    void start_operation();

    // Ports for reset and interrupt.
    sc_in<bool>  i_reset;      // Input port for reset.
    sc_out<int>  o_interrupt;  // Output port for interrupt.

    // Interrupt state definitions.
    static const int INT_IDLE;   // For example, 0 (active low)
    static const int INT_DONE;   // For example, 1
    static const int INT_ERROR;  // For example, 2

    // New constant for int_clear SFR offset.
    static const uint32_t INT_CLEAR_OFFSET;

private:
    // SFR offset definitions.
    static const uint32_t CTRL_OFFSET;      // 0x00000000
    static const uint32_t STATUS_OFFSET;    // 0x00000004
    // INT_CLEAR_OFFSET is defined above (0x00000008)
    static const uint32_t IFM_INFO_OFFSET;    // 0x00000010
    static const uint32_t WT_INFO_OFFSET;     // 0x00000014
    static const uint32_t OFM_INFO_OFFSET;    // 0x00000018

    // SFR submodules.
    SFR *ctrl;
    SFR *status;
    SFR *int_clear; // New SFR to clear interrupt.
    SFR *ifm_info;
    SFR *wt_info;
    SFR *ofm_info;

    // Mapping from offset to SFR pointer.
    std::unordered_map<uint32_t, SFR*> sfr_map;
};

#endif // NPU_H

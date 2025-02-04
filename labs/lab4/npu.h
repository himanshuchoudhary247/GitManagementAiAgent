#ifndef NPU_H
#define NPU_H

#include <systemc.h>
#include <cstdint>
#include <unordered_map>
#include "sfr.h"

// NPU module definition.
// Instantiates five SFR submodules, provides SFR access functions, and
// handles reset and interrupt signaling.
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

    // Reset handling method: called on positive edge of i_reset.
    void handle_reset();

    // Overridden end_of_elaboration callback: drive the interrupt port with a stable value.
    virtual void end_of_elaboration() override;

    // Simulate NPU operation.
    void start_operation();

    // Ports for reset and interrupt.
    sc_in<bool>  i_reset;      // Input port for reset.
    sc_out<int>  o_interrupt;  // Output port for interrupt.
                              // Drives three values: IDLE, DONE, ERROR.

    // Interrupt state definitions.
    static const int INT_IDLE;   // e.g., 0 (active low)
    static const int INT_DONE;   // e.g., 1
    static const int INT_ERROR;  // e.g., 2

private:
    // Offset definitions.
    static const uint32_t CTRL_OFFSET;    // 0x00000000
    static const uint32_t STATUS_OFFSET;  // 0x00000004
    static const uint32_t IFM_INFO_OFFSET; // 0x00000010
    static const uint32_t WT_INFO_OFFSET;  // 0x00000014
    static const uint32_t OFM_INFO_OFFSET; // 0x00000018

    // SFR submodules.
    SFR *ctrl;
    SFR *status;
    SFR *ifm_info;
    SFR *wt_info;
    SFR *ofm_info;

    // Mapping from offset to SFR pointer.
    std::unordered_map<uint32_t, SFR*> sfr_map;
};

#endif // NPU_H

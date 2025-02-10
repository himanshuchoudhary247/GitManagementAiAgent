#ifndef NPU_H
#define NPU_H

#include <systemc.h>
#include <cstdint>
#include <unordered_map>
#include "sfr.h"

// NPU module definition.
// (Based on Lab 6, with the following modifications for Lab 7:)
// • The static (direct) call to start_operation is removed.
// • Instead, when a CTRL trigger is detected and micro‑start is enabled,
//   the NPU spawns a dynamic process to execute start_operation.
// • A dynamic thread process (dummy_thread) is spawned during end_of_elaboration.
SC_MODULE(NPU) {
public:
    NPU(sc_module_name name);
    ~NPU();

    void write(uint32_t offset, const uint32_t &value);
    void read(uint32_t offset, uint32_t &value) const;
    void configure_ifm(uint32_t width, uint32_t height);
    void configure_ofm(uint32_t width, uint32_t height);
    void configure_weight(uint32_t width, uint32_t height);

    void handle_reset();
    void end_of_elaboration();
    void start_operation(); // Now to be invoked dynamically via sc_spawn.
    void dummy_thread();    // New dynamic thread process.
    void check_start_micro(); // Updates m_enable_start based on i_start_micro.

    // Ports.
    sc_in<bool>  i_reset;       // Reset input.
    sc_in<bool>  i_start_micro; // New micro-start control input.
    sc_out<int>  o_interrupt;   // Interrupt output.

    // Interrupt state definitions.
    static const int INT_IDLE;   // e.g., 0 (active low)
    static const int INT_DONE;   // e.g., 1
    static const int INT_ERROR;  // e.g., 2

    // SFR offset constants.
    static const uint32_t CTRL_OFFSET;      
    static const uint32_t STATUS_OFFSET;    
    static const uint32_t INT_CLEAR_OFFSET;   
    static const uint32_t IFM_INFO_OFFSET;    
    static const uint32_t WT_INFO_OFFSET;     
    static const uint32_t OFM_INFO_OFFSET;    

private:
    SFR *ctrl;
    SFR *status;
    SFR *int_clear; // SFR used to clear the interrupt.
    SFR *ifm_info;
    SFR *wt_info;
    SFR *ofm_info;
    std::unordered_map<uint32_t, SFR*> sfr_map;

    // New flag: if true, CTRL-triggered start is enabled.
    bool m_enable_start;
};

#endif // NPU_H

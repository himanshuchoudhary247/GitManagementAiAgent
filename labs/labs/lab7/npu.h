#ifndef NPU_H
#define NPU_H

#include <systemc.h>
#include <cstdint>
#include <unordered_map>
#include "sfr.h"

// NPU module definition.
// Based on Lab 6, with Lab 7 modifications:
// • Remove the static call to start_operation and instead spawn a dynamic process.
// • Add a dynamic dummy_thread process spawned during end_of_elaboration.
SC_MODULE(NPU) {
public:
    // Include SC_HAS_PROCESS macro.
    SC_HAS_PROCESS(NPU);

    NPU(sc_module_name name);
    ~NPU();

    void write(uint32_t offset, const uint32_t &value);
    void read(uint32_t offset, uint32_t &value) const;
    void configure_ifm(uint32_t width, uint32_t height);
    void configure_ofm(uint32_t width, uint32_t height);
    void configure_weight(uint32_t width, uint32_t height);

    void handle_reset();
    void end_of_elaboration();
    void start_operation(); // This will be spawned dynamically.
    void dummy_thread();    // Dynamic thread process spawned during end_of_elaboration.
    void check_start_micro(); // Updates m_enable_start based on i_start_micro.

    // Ports.
    sc_in<bool>  i_reset;       // Reset input.
    sc_in<bool>  i_start_micro; // Micro-start control input.
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

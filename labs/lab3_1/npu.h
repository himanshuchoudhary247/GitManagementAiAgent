#ifndef NPU_H
#define NPU_H

#include <systemc.h>
#include <cstdint>
#include <unordered_map>
#include "sfr.h"

// NPU module definition.
// Instantiates five SFR submodules and provides access functions.
// Also demonstrates the use of an event queue.
SC_MODULE(NPU) {
public:
    // Constructor.
    NPU(sc_core::sc_module_name name);
    // Destructor.
    ~NPU();

    // Public member functions.
    void write(uint32_t offset, const uint32_t &value);
    void read(uint32_t offset, uint32_t &value) const;
    void configure_ifm(uint32_t width, uint32_t height);
    void configure_ofm(uint32_t width, uint32_t height);
    void configure_weight(uint32_t width, uint32_t height);

    // SC_THREAD process to demonstrate the event queue.
    void event_queue_demo_thread();

    // End-of-elaboration callback.
    virtual void end_of_elaboration() override;

private:
    // Offset definitions.
    static const uint32_t CTRL_OFFSET;     // 0x00000000
    static const uint32_t STATUS_OFFSET;   // 0x00000004
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

    // Private helper: simulate starting the NPU operation.
    void start_operation();

    // Event queue object.
    sc_core::sc_event_queue m_event_queue;
};

#endif // NPU_H

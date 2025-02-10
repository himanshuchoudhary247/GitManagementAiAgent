#ifndef NPU_H
#define NPU_H

#include <systemc.h>
#include <cstdint>
#include <unordered_map>
#include "sfr.h"

// NPU module definition.
// Instantiates five SFR submodules and provides access functions.
SC_MODULE(NPU) {
public:
    // Constructor.
    NPU(sc_core::sc_module_name name);
    // Destructor.
    ~NPU();

    // Member functions.
    void write(uint32_t offset, const uint32_t &value);
    void read(uint32_t offset, uint32_t &value) const;
    void configure_ifm(uint32_t width, uint32_t height);
    void configure_ofm(uint32_t width, uint32_t height);
    void configure_weight(uint32_t width, uint32_t height);

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
};

#endif // NPU_H

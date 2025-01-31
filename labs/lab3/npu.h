#ifndef NPU_H
#define NPU_H

#include <systemc.h>
#include <cstdint>
#include <map>
#include "sfr.h"

// Define constants for NPU SFRs with SAM_NPU_ prefix
#define SAM_NPU_CTRL_OFFSET          0x00000000
#define SAM_NPU_STATUS_OFFSET        0x00000004
#define SAM_NPU_IFM_INFO_OFFSET      0x00000010
#define SAM_NPU_WT_INFO_OFFSET       0x00000014
#define SAM_NPU_OFM_INFO_OFFSET      0x00000018

// Define bit masks
#define SAM_NPU_CTRL_START_BIT       0x00000001
#define SAM_NPU_STATUS_DONE_BIT      0x00000001

SC_MODULE(NPU) {
public:
    // Constructor
    SC_CTOR(NPU);

    // Destructor
    ~NPU();

    // Write to a specific SFR based on offset
    void write(uint32_t a_offset, const uint32_t &a_value);

    // Read from a specific SFR based on offset
    void read(uint32_t a_offset, uint32_t &a_value) const;

private:
    // SFR Objects with m_ prefix
    SFR* m_ctrl;
    SFR* m_status;
    SFR* m_ifm_info;
    SFR* m_wt_info;
    SFR* m_ofm_info;

    // Map for offset to SFR pointer
    std::map<uint32_t, SFR*> m_sfr_map;

    // Initiates the NPU operation
    void start_operation();

    // Process to handle operation completion
    void operation_thread();

    // Event to trigger operation completion
    sc_event m_operation_done_event;
};

#endif // NPU_H

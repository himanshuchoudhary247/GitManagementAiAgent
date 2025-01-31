#ifndef SFR_H
#define SFR_H

#include <systemc.h>
#include <cstdint>
#include <iostream>

// Define constants for SFRs with SAM_{module_name}_ prefix
#define SAM_SFR_CTRL_OFFSET          0x00000000
#define SAM_SFR_STATUS_OFFSET        0x00000004
#define SAM_SFR_IFM_INFO_OFFSET      0x00000010
#define SAM_SFR_WT_INFO_OFFSET       0x00000014
#define SAM_SFR_OFM_INFO_OFFSET      0x00000018

#define SAM_SFR_MAX_VALUE            0xFFFFFFFF
#define SAM_SFR_CTRL_START_BIT       0x00000001
#define SAM_SFR_STATUS_DONE_BIT      0x00000001

class SFR : public sc_module {
public:
    // Constructor
    SC_HAS_PROCESS(SFR);
    SFR(sc_module_name name,
        uint32_t a_offset,
        uint32_t a_sw_read_mask,
        uint32_t a_sw_write_mask,
        uint32_t a_hw_read_mask,
        uint32_t a_hw_write_mask,
        uint32_t a_reset_value);

    // Destructor
    ~SFR();

    // External/SW write to SFR
    void write(const uint32_t &a_value);

    // External/SW read of SFR
    void read(uint32_t &a_value) const;

    // Internal/HW write to SFR
    void set(const uint32_t &a_value);

    // Internal/HW read of SFR
    void get(uint32_t &a_value) const;

private:
    // Member Variables with m_ prefix
    uint32_t m_offset;           // SFR Offset
    uint32_t m_value;            // Value of the SFR
    uint32_t m_sw_read_mask;     // SW Read mask
    uint32_t m_sw_write_mask;    // SW Write mask
    uint32_t m_hw_read_mask;     // HW Read mask
    uint32_t m_hw_write_mask;    // HW Write mask
    uint32_t m_reset_value;      // Reset value of the SFR

    // SystemC Processes
    void dummy_thread();

    // Utility
    std::string get_module_name() const;
};

#endif // SFR_H

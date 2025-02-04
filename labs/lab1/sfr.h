// sfr.h

#ifndef SFR_H
#define SFR_H

#include <cstdint>

class SFR {
public:
    // Constructor
    SFR(uint32_t offset,
        uint32_t sw_read_mask,
        uint32_t sw_write_mask,
        uint32_t hw_read_mask,
        uint32_t hw_write_mask,
        uint32_t reset_value);

    // Destructor
    ~SFR();

    // External/SW write to SFR
    void write(const uint32_t& input_value);

    // External/SW read of SFR
    void read(uint32_t& output_value) const;

    // Internal/HW write to SFR
    void set(const uint32_t& input_value);

    // Internal/HW read of SFR
    void get(uint32_t& output_value) const;

private:
    const uint32_t m_offset;           // SFR Offset
    uint32_t m_value;                  // Value of the SFR
    const uint32_t m_sw_read_mask;     // SW Read mask
    const uint32_t m_sw_write_mask;    // SW Write mask
    const uint32_t m_hw_read_mask;     // HW Read mask
    const uint32_t m_hw_write_mask;    // HW Write mask
    const uint32_t m_reset_value;      // Reset value of the SFR
};

#endif // SFR_H

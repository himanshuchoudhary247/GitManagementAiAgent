#ifndef SFR_H
#define SFR_H

#include <cstdint>

class SFR {
private:
    uint32_t m_offset;          // SFR Offset
    uint32_t m_value;           // Value of the SFR
    uint32_t m_sw_read_mask;    // SW Read mask
    uint32_t m_sw_write_mask;   // SW Write mask
    uint32_t m_hw_read_mask;    // HW Read mask
    uint32_t m_hw_write_mask;   // HW Write mask
    uint32_t m_reset_value;     // Reset value of the SFR

public:
    // Constructor
    SFR(uint32_t a_offset, uint32_t a_sw_read_mask, uint32_t a_sw_write_mask,
        uint32_t a_hw_read_mask, uint32_t a_hw_write_mask, uint32_t a_reset_value);

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
};

#endif // SFR_H

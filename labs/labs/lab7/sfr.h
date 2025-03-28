#ifndef SFR_H
#define SFR_H

#include <systemc.h>
#include <cstdint>

// SFR module definition.
// Provides basic register operations (write, read, set, get, reset)
// plus two dummy processes that print messages.
SC_MODULE(SFR) {
public:
    SC_HAS_PROCESS(SFR);

    // Constructor and destructor.
    SFR(sc_module_name name,
        uint32_t offset,
        uint32_t sw_read_mask,
        uint32_t sw_write_mask,
        uint32_t hw_read_mask,
        uint32_t hw_write_mask,
        uint32_t reset_value);
    ~SFR();

    // Basic register operations.
    void write(const uint32_t &input_value);
    void read(uint32_t &output_value) const;
    void set(const uint32_t &input_value);
    void get(uint32_t &output_value) const;
    void reset();

    // Dummy processes.
    void dummy_method();  // SC_METHOD process.
    void dummy_thread();  // SC_THREAD process.

private:
    uint32_t m_offset;           // SFR offset.
    uint32_t m_value;            // Current value.
    uint32_t m_sw_read_mask;     // Software read mask.
    uint32_t m_sw_write_mask;    // Software write mask.
    uint32_t m_hw_read_mask;     // Hardware read mask.
    uint32_t m_hw_write_mask;    // Hardware write mask.
    uint32_t m_reset_value;      // Reset value.
};

#endif // SFR_H

#ifndef SFR_H
#define SFR_H

#include <systemc.h>
#include <cstdint>

/**
 * SFR module representing a Special Function Register.
 *
 * Member Variables:
 *   m_offset: SFR Offset
 *   m_value:  Current register value
 *   m_sw_read_mask, m_sw_write_mask, m_hw_read_mask, m_hw_write_mask, m_reset_value
 *
 * Member Functions:
 *   - Constructor (6 arguments)
 *   - Destructor
 *   - write: external (SW) write
 *   - read:  external (SW) read (value returned via reference)
 *   - set:   internal (HW) write
 *   - get:   internal (HW) read
 *   - reset: resets register to m_reset_value
 *
 * Additionally, two processes are added:
 *   - dummy_method (SC_METHOD)
 *   - dummy_thread (SC_THREAD)
 */
SC_MODULE(SFR) {
public:
    SC_HAS_PROCESS(SFR);

    SFR(sc_module_name name,
        uint32_t offset,
        uint32_t sw_read_mask,
        uint32_t sw_write_mask,
        uint32_t hw_read_mask,
        uint32_t hw_write_mask,
        uint32_t reset_value);
    ~SFR();

    void write(const uint32_t &input_value);
    void read(uint32_t &output_value) const;
    void set(const uint32_t &input_value);
    void get(uint32_t &output_value) const;
    void reset();

    void dummy_method();
    void dummy_thread();

private:
    uint32_t m_offset;
    uint32_t m_value;
    uint32_t m_sw_read_mask;
    uint32_t m_sw_write_mask;
    uint32_t m_hw_read_mask;
    uint32_t m_hw_write_mask;
    uint32_t m_reset_value;
};

#endif // SFR_H

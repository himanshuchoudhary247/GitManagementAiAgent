#ifndef SFR_H
#define SFR_H

#include <systemc.h>
#include <cstdint>

/**
 * SFR module representing a Special Function Register.
 *
 * Member Variables:
 *   m_offset: register offset
 *   m_value: current register value
 *   m_sw_read_mask, m_sw_write_mask: masks for SW read/write
 *   m_hw_read_mask, m_hw_write_mask: masks for HW read/write
 *   m_reset_value: reset value of the register
 *
 * Member Functions:
 *   Constructor (6 arguments), Destructor,
 *   write (SW write), read (SW read via reference),
 *   set (HW write), get (HW read), reset (sets m_value = m_reset_value)
 *
 * Additionally, two processes are provided:
 *   dummy_method (SC_METHOD) and dummy_thread (SC_THREAD)
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

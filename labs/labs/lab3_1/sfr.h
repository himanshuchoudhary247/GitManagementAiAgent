#ifndef SFR_H
#define SFR_H

#include <systemc.h>
#include <cstdint>

/**
 * SFR module representing a Special Function Register.
 *
 * Member Variables:
 *  - m_offset: SFR Offset
 *  - m_value: Current register value
 *  - m_sw_read_mask: Software read mask
 *  - m_sw_write_mask: Software write mask
 *  - m_hw_read_mask: Hardware read mask
 *  - m_hw_write_mask: Hardware write mask
 *  - m_reset_value: Reset value
 *
 * Member Functions:
 *  - Constructor (takes 6 arguments: offset, sw_read_mask, sw_write_mask, hw_read_mask, hw_write_mask, reset_value)
 *  - Destructor
 *  - write: External (SW) write (argument is const)
 *  - read: External (SW) read (returns value through reference)
 *  - set:  Internal (HW) write
 *  - get:  Internal (HW) read
 *  - reset: Resets register to m_reset_value
 *
 * Additional Processes:
 *  - dummy_method (SC_METHOD) prints module name, method name, and simulation time.
 *  - dummy_thread (SC_THREAD) prints method name and simulation time.
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

    void dummy_method();  // SC_METHOD: prints module name, method name, and simulation time.
    void dummy_thread();  // SC_THREAD: prints method name and simulation time.

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

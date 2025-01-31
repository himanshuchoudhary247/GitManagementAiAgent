#include "sfr.h"

// Constructor
SFR::SFR(sc_module_name name,
         uint32_t a_offset,
         uint32_t a_sw_read_mask,
         uint32_t a_sw_write_mask,
         uint32_t a_hw_read_mask,
         uint32_t a_hw_write_mask,
         uint32_t a_reset_value)
    : sc_module(name),
      m_offset(a_offset),
      m_value(a_reset_value),
      m_sw_read_mask(a_sw_read_mask),
      m_sw_write_mask(a_sw_write_mask),
      m_hw_read_mask(a_hw_read_mask),
      m_hw_write_mask(a_hw_write_mask),
      m_reset_value(a_reset_value)
{
    // Register SC_THREAD
    SC_THREAD(dummy_thread);
}

// Destructor
SFR::~SFR() {}

// External/SW write to SFR
void SFR::write(const uint32_t &a_value) {
    // Only modify bits that are allowed by the SW write mask
    m_value = (m_value & ~m_sw_write_mask) | (a_value & m_sw_write_mask);
    std::cout << "[" << sc_time_stamp() << "] " << get_module_name()
              << ": write called with value 0x" << std::hex << a_value
              << ", new m_value = 0x" << m_value << std::dec << "\n";
}

// External/SW read of SFR
void SFR::read(uint32_t &a_value) const {
    // Only allow bits specified by the SW read mask
    a_value = m_value & m_sw_read_mask;
    std::cout << "[" << sc_time_stamp() << "] " << get_module_name()
              << ": read called, returning 0x" << std::hex << a_value << std::dec << "\n";
}

// Internal/HW write to SFR
void SFR::set(const uint32_t &a_value) {
    // Only modify bits that are allowed by the HW write mask
    m_value = (m_value & ~m_hw_write_mask) | (a_value & m_hw_write_mask);
    std::cout << "[" << sc_time_stamp() << "] " << get_module_name()
              << ": set called with value 0x" << std::hex << a_value
              << ", new m_value = 0x" << m_value << std::dec << "\n";
}

// Internal/HW read of SFR
void SFR::get(uint32_t &a_value) const {
    // Only allow bits specified by the HW read mask
    a_value = m_value & m_hw_read_mask;
    std::cout << "[" << sc_time_stamp() << "] " << get_module_name()
              << ": get called, returning 0x" << std::hex << a_value << std::dec << "\n";
}

// // SC_THREAD: dummy_thread
// void SFR::dummy_thread() {
//     // Print a single message and wait indefinitely to prevent infinite looping
//     std::cout << "[" << sc_time_stamp() << "] " << get_module_name()
//               << ": dummy_thread is running.\n";
//     while (true) {
//         wait(); // Wait indefinitely
//     }
// }

// SC_THREAD: dummy_thread
void SFR::dummy_thread() {
    // Print a single message and wait indefinitely to prevent infinite looping
    std::cout << "[" << sc_time_stamp() << "] " << get_module_name()
              << ": dummy_thread is running.\n";
    while (true) {
        wait(); // Wait indefinitely
    }
}


// Utility to get module name
std::string SFR::get_module_name() const {
    return std::string(this->name());
}

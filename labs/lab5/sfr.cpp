#include "sfr.h"
#include <iostream>

SFR::SFR(sc_module_name name,
         uint32_t offset,
         uint32_t sw_read_mask,
         uint32_t sw_write_mask,
         uint32_t hw_read_mask,
         uint32_t hw_write_mask,
         uint32_t reset_value)
    : sc_module(name),
      m_offset(offset),
      m_sw_read_mask(sw_read_mask),
      m_sw_write_mask(sw_write_mask),
      m_hw_read_mask(hw_read_mask),
      m_hw_write_mask(hw_write_mask),
      m_reset_value(reset_value),
      m_value(reset_value)
{
    SC_METHOD(dummy_method);
    SC_THREAD(dummy_thread);
}

SFR::~SFR() {
    // Nothing to free
}

void SFR::write(const uint32_t &input_value) {
    // External SW write => only bits allowed by sw_write_mask
    m_value = (m_value & ~m_sw_write_mask) | (input_value & m_sw_write_mask);
}

void SFR::read(uint32_t &output_value) const {
    // External SW read => only bits allowed by sw_read_mask
    output_value = m_value & m_sw_read_mask;
}

void SFR::set(const uint32_t &input_value) {
    // Internal HW write => only bits allowed by hw_write_mask
    m_value = (m_value & ~m_hw_write_mask) | (input_value & m_hw_write_mask);
}

void SFR::get(uint32_t &output_value) const {
    // Internal HW read => only bits allowed by hw_read_mask
    output_value = m_value & m_hw_read_mask;
}

void SFR::reset() {
    m_value = m_reset_value;
    std::cout << "[" << name() << "] reset() => m_value=" << m_reset_value
              << " at time " << sc_time_stamp() << std::endl;
}

// Additional processes
void SFR::dummy_method() {
    std::cout << "[" << name() << "] dummy_method at " 
              << sc_time_stamp() << std::endl;
}

void SFR::dummy_thread() {
    std::cout << "[" << name() << "] dummy_thread start at "
              << sc_time_stamp() << std::endl;
    wait(10, SC_NS);
    std::cout << "[" << name() << "] dummy_thread resumed at "
              << sc_time_stamp() << std::endl;
}

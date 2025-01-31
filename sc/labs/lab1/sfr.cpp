#include "sfr.h"

// Constructor
SFR::SFR(uint32_t a_offset, uint32_t a_sw_read_mask, uint32_t a_sw_write_mask,
         uint32_t a_hw_read_mask, uint32_t a_hw_write_mask, uint32_t a_reset_value)
    : m_offset(a_offset),
      m_value(a_reset_value),
      m_sw_read_mask(a_sw_read_mask),
      m_sw_write_mask(a_sw_write_mask),
      m_hw_read_mask(a_hw_read_mask),
      m_hw_write_mask(a_hw_write_mask),
      m_reset_value(a_reset_value) {}

// Destructor
SFR::~SFR() {}

// External/SW write to SFR
void SFR::write(const uint32_t &a_value) {
    // Only modify bits that are allowed by the SW write mask
    m_value = (m_value & ~m_sw_write_mask) | (a_value & m_sw_write_mask);
}

// External/SW read of SFR
void SFR::read(uint32_t &a_value) const {
    // Only allow bits specified by the SW read mask
    a_value = m_value & m_sw_read_mask;
}

// Internal/HW write to SFR
void SFR::set(const uint32_t &a_value) {
    // Only modify bits that are allowed by the HW write mask
    m_value = (m_value & ~m_hw_write_mask) | (a_value & m_hw_write_mask);
}

// Internal/HW read of SFR
void SFR::get(uint32_t &a_value) const {
    // Only allow bits specified by the HW read mask
    a_value = m_value & m_hw_read_mask;
}

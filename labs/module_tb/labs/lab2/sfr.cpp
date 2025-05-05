// sfr.cpp

#include "sfr.h"

// Constructor
SFR::SFR(uint32_t offset,
         uint32_t sw_read_mask,
         uint32_t sw_write_mask,
         uint32_t hw_read_mask,
         uint32_t hw_write_mask,
         uint32_t reset_value)
    : m_offset(offset),
      m_sw_read_mask(sw_read_mask),
      m_sw_write_mask(sw_write_mask),
      m_hw_read_mask(hw_read_mask),
      m_hw_write_mask(hw_write_mask),
      m_reset_value(reset_value),
      m_value(reset_value) // Initialize m_value with reset_value
{
}

// Destructor
SFR::~SFR()
{
    // No dynamic memory to clean up
}

// External/SW write to SFR
void SFR::write(const uint32_t& input_value)
{
    // Apply SW write mask: only bits allowed by the mask are modified
    m_value = (m_value & ~m_sw_write_mask) | (input_value & m_sw_write_mask);
}

// External/SW read of SFR
void SFR::read(uint32_t& output_value) const
{
    // Apply SW read mask: only bits allowed by the mask are returned
    output_value = m_value & m_sw_read_mask;
}

// Internal/HW write to SFR
void SFR::set(const uint32_t& input_value)
{
    // Apply HW write mask: only bits allowed by the mask are modified
    m_value = (m_value & ~m_hw_write_mask) | (input_value & m_hw_write_mask);
}

// Internal/HW read of SFR
void SFR::get(uint32_t& output_value) const
{
    // Apply HW read mask: only bits allowed by the mask are returned
    output_value = m_value & m_hw_read_mask;
}

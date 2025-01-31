#include "npu.h"
#include "sfr.h"
#include <iostream>

// Implementation class to hide SFR details from main
class NPU::NPUImpl {
public:
    // SFR Objects
    SFR m_ctrl;
    SFR m_status;
    SFR m_ifm_width;
    SFR m_ifm_height;
    SFR m_ofm_width;
    SFR m_ofm_height;
    SFR m_weight_width;
    SFR m_weight_height;

    // Constructor initializes all SFRs
    NPUImpl()
        : m_ctrl(SAM_NPU_CTRL_OFFSET, 0xFFFFFFFF, SAM_NPU_CTRL_START_BIT,
                 0xFFFFFFFF, SAM_NPU_CTRL_START_BIT, 0x0),
          m_status(SAM_NPU_STATUS_OFFSET, SAM_NPU_STATUS_DONE_BIT, 0x0,
                  SAM_NPU_STATUS_DONE_BIT, 0x0, 0x0),
          m_ifm_width(SAM_NPU_IFM_WIDTH_OFFSET, 0xFFFFFFFF, 0xFFFFFFFF,
                     0xFFFFFFFF, 0xFFFFFFFF, 0x10),
          m_ifm_height(SAM_NPU_IFM_HEIGHT_OFFSET, 0xFFFFFFFF, 0xFFFFFFFF,
                      0xFFFFFFFF, 0xFFFFFFFF, 18),
          m_ofm_width(SAM_NPU_OFM_WIDTH_OFFSET, 0xFFFFFFFF, 0xFFFFFFFF,
                     0xFFFFFFFF, 0xFFFFFFFF, 14),
          m_ofm_height(SAM_NPU_OFM_HEIGHT_OFFSET, 0xFFFFFFFF, 0xFFFFFFFF,
                      0xFFFFFFFF, 0xFFFFFFFF, 16),
          m_weight_width(SAM_NPU_WEIGHT_WIDTH_OFFSET, 0xFFFFFFFF, 0xFFFFFFFF,
                        0xFFFFFFFF, 0xFFFFFFFF, 0x3),
          m_weight_height(SAM_NPU_WEIGHT_HEIGHT_OFFSET, 0xFFFFFFFF, 0xFFFFFFFF,
                         0xFFFFFFFF, 0xFFFFFFFF, 0x3) {}
};

// Constructor
NPU::NPU() : m_impl(new NPUImpl()) {}

// Destructor
NPU::~NPU() {
    delete m_impl;
}

// Write to a specific SFR based on offset
void NPU::write(uint32_t a_offset, const uint32_t &a_value) {
    if (a_offset == SAM_NPU_CTRL_OFFSET) {
        m_impl->m_ctrl.write(a_value);
        // Check if start bit is set
        uint32_t I_read_value = 0;
        m_impl->m_ctrl.read(I_read_value);
        if (I_read_value & SAM_NPU_CTRL_START_BIT) {
            start_operation();
        }
    }
    else if (a_offset == SAM_NPU_IFM_WIDTH_OFFSET) {
        m_impl->m_ifm_width.write(a_value);
    }
    else if (a_offset == SAM_NPU_IFM_HEIGHT_OFFSET) {
        m_impl->m_ifm_height.write(a_value);
    }
    else if (a_offset == SAM_NPU_OFM_WIDTH_OFFSET) {
        m_impl->m_ofm_width.write(a_value);
    }
    else if (a_offset == SAM_NPU_OFM_HEIGHT_OFFSET) {
        m_impl->m_ofm_height.write(a_value);
    }
    else if (a_offset == SAM_NPU_WEIGHT_WIDTH_OFFSET) {
        m_impl->m_weight_width.write(a_value);
    }
    else if (a_offset == SAM_NPU_WEIGHT_HEIGHT_OFFSET) {
        m_impl->m_weight_height.write(a_value);
    }
    else if (a_offset == SAM_NPU_STATUS_OFFSET) {
        // STATUS SFR is typically read-only; ignore or handle accordingly
        std::cout << "Attempt to write to STATUS SFR ignored.\n";
    }
    else {
        std::cout << "Invalid SFR Offset: 0x" << std::hex << a_offset << "\n";
    }
}

// Read from a specific SFR based on offset
void NPU::read(uint32_t a_offset, uint32_t &a_value) const {
    if (a_offset == SAM_NPU_CTRL_OFFSET) {
        m_impl->m_ctrl.read(a_value);
    }
    else if (a_offset == SAM_NPU_STATUS_OFFSET) {
        m_impl->m_status.read(a_value);
    }
    else if (a_offset == SAM_NPU_IFM_WIDTH_OFFSET) {
        m_impl->m_ifm_width.read(a_value);
    }
    else if (a_offset == SAM_NPU_IFM_HEIGHT_OFFSET) {
        m_impl->m_ifm_height.read(a_value);
    }
    else if (a_offset == SAM_NPU_OFM_WIDTH_OFFSET) {
        m_impl->m_ofm_width.read(a_value);
    }
    else if (a_offset == SAM_NPU_OFM_HEIGHT_OFFSET) {
        m_impl->m_ofm_height.read(a_value);
    }
    else if (a_offset == SAM_NPU_WEIGHT_WIDTH_OFFSET) {
        m_impl->m_weight_width.read(a_value);
    }
    else if (a_offset == SAM_NPU_WEIGHT_HEIGHT_OFFSET) {
        m_impl->m_weight_height.read(a_value);
    }
    else {
        std::cout << "Invalid SFR Offset: 0x" << std::hex << a_offset << "\n";
        a_value = 0;
    }
}

// Initiates the NPU operation by setting the STATUS done bit
void NPU::start_operation() {
    // Simulate NPU operation by setting the done bit
    uint32_t I_set_value = SAM_NPU_STATUS_DONE_BIT;
    m_impl->m_status.set(I_set_value);
}


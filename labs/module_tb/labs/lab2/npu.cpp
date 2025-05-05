// npu.cpp

#include "npu.h"

// Define SFR offsets
constexpr uint32_t CTRL_OFFSET     = 0x00000000;
constexpr uint32_t STATUS_OFFSET   = 0x00000004;
constexpr uint32_t IFM_INFO_OFFSET = 0x00000010;
constexpr uint32_t WT_INFO_OFFSET  = 0x00000014;
constexpr uint32_t OFM_INFO_OFFSET = 0x00000018;

// Constructor
NPU::NPU()
    : ctrl(CTRL_OFFSET, 0xFFFFFFFE, 0x00000001, 0xFFFFFFFF, 0x00000000, 0x0),
      status(STATUS_OFFSET, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000003, 0x0),
      ifm_info(IFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF, 0xFFFFFFFF, 0x00000000, 0x0),
      wt_info(WT_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF, 0xFFFFFFFF, 0x00000000, 0x0),
      ofm_info(OFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF, 0xFFFFFFFF, 0x00000000, 0x0)
{
    // Initialize the SFR mapping
    sfr_map[CTRL_OFFSET] = &ctrl;
    sfr_map[STATUS_OFFSET] = &status;
    sfr_map[IFM_INFO_OFFSET] = &ifm_info;
    sfr_map[WT_INFO_OFFSET] = &wt_info;
    sfr_map[OFM_INFO_OFFSET] = &ofm_info;
}

// Destructor
NPU::~NPU()
{
    // No dynamic memory to clean up
}

// Write function: Takes offset and value
void NPU::write(uint32_t offset, const uint32_t& value)
{
    auto it = sfr_map.find(offset);
    if (it != sfr_map.end()) {
        it->second->write(value);

        // If writing to CTRL, check if start bit is set
        if (offset == CTRL_OFFSET) {
            // Check if start bit (bit 0) is set
            uint32_t ctrl_value = 0;
            it->second->read(ctrl_value);
            if (ctrl_value & 0x1) {
                start_operation();
            }
        }
    }
    else {
        // Handle invalid offset if necessary
    }
}

// Read function: Takes offset and reference to store value
void NPU::read(uint32_t offset, uint32_t& value) const
{
    auto it = sfr_map.find(offset);
    if (it != sfr_map.end()) {
        it->second->read(value);
    }
    else {
        // Handle invalid offset if necessary
        value = 0;
    }
}

// Configuration functions
void NPU::configure_ifm(uint32_t width, uint32_t height)
{
    // Assuming IFM Width occupies bits [15:0] and IFM Height occupies bits [31:16]
    uint32_t config_value = (height << 16) | (width & 0xFFFF);
    ifm_info.write(config_value);
}

void NPU::configure_ofm(uint32_t width, uint32_t height)
{
    // Assuming OFM Width occupies bits [15:0] and OFM Height occupies bits [31:16]
    uint32_t config_value = (height << 16) | (width & 0xFFFF);
    ofm_info.write(config_value);
}

void NPU::configure_weight(uint32_t width, uint32_t height)
{
    // Assuming Weight Width occupies bits [15:0] and Weight Height occupies bits [31:16]
    uint32_t config_value = (height << 16) | (width & 0xFFFF);
    wt_info.write(config_value);
}

// Private function to start NPU operation
void NPU::start_operation()
{
    // Simulate NPU operation by setting the STATUS SFR's done bit to 1
    // Assuming done bit is bit 0
    uint32_t status_value = 0;
    status.get(status_value);
    status_value |= 0x1; // Set done bit
    status.set(status_value);
}

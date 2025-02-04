// main.cpp

#include <iostream>
#include "npu.h"

int main()
{
    // Instantiate NPU class
    NPU npu;

    // Define SFR offsets
    constexpr uint32_t STATUS_OFFSET   = 0x00000004;
    constexpr uint32_t IFM_INFO_OFFSET = 0x00000010;
    constexpr uint32_t WT_INFO_OFFSET  = 0x00000014;
    constexpr uint32_t OFM_INFO_OFFSET = 0x00000018;
    constexpr uint32_t CTRL_OFFSET     = 0x00000000;

    // Array of SFR offsets and their names for easy iteration (excluding CTRL)
    struct SFR_Info {
        uint32_t offset;
        const char* name;
        uint32_t expected_mask;
    };

    SFR_Info sfrs[] = {
        { STATUS_OFFSET, "status", 0xFFFFFFFF },
        { IFM_INFO_OFFSET, "ifm_info", 0x0FFF0FFF },
        { WT_INFO_OFFSET, "wt_info", 0x0FFF0FFF },
        { OFM_INFO_OFFSET, "ofm_info", 0x0FFF0FFF }
    };

    const size_t num_sfrs = sizeof(sfrs) / sizeof(sfrs[0]);

    // Value to write
    uint32_t write_value = 0xFFFFFFFF;

    // Perform write to each SFR (excluding CTRL)
    for (size_t i = 0; i < num_sfrs; ++i) {
        npu.write(sfrs[i].offset, write_value);
    }

    // Verify the read values
    for (size_t i = 0; i < num_sfrs; ++i) {
        uint32_t read_value = 0;
        npu.read(sfrs[i].offset, read_value);

        // Determine the expected value based on the SW read mask
        uint32_t expected_value = write_value & sfrs[i].expected_mask;

        // Check if the read value matches the expected value
        if (read_value == expected_value) {
            std::cout << sfrs[i].name << " read value matches expected value: 0x"
                      << std::hex << read_value << std::dec << std::endl;
        }
        else {
            std::cout << sfrs[i].name << " read value does NOT match expected value."
                      << " Expected: 0x" << std::hex << expected_value
                      << ", Got: 0x" << read_value << std::dec << std::endl;
        }
    }

    // Step 4: Configure NPU
    npu.configure_ifm(0x10, 18);      // IFM Width = 0x10, IFM Height = 18
    npu.configure_ofm(14, 16);        // OFM Width = 14, OFM Height = 16
    npu.configure_weight(0x3, 0x3);   // Weight Width = 0x3, Weight Height = 0x3

    // Optionally, verify configurations
    std::cout << "\nConfigured NPU Parameters:" << std::endl;

    // IFM Info
    uint32_t ifm_config = 0;
    npu.read(IFM_INFO_OFFSET, ifm_config);
    std::cout << "IFM Info: 0x" << std::hex << ifm_config << std::dec << std::endl;

    // OFM Info
    uint32_t ofm_config = 0;
    npu.read(OFM_INFO_OFFSET, ofm_config);
    std::cout << "OFM Info: 0x" << std::hex << ofm_config << std::dec << std::endl;

    // Weight Info
    uint32_t wt_config = 0;
    npu.read(WT_INFO_OFFSET, wt_config);
    std::cout << "Weight Info: 0x" << std::hex << wt_config << std::dec << std::endl;

    // Step 5: Write 1 to CTRL SFR start bit to trigger NPU
    uint32_t ctrl_start_bit = 0x1; // Assuming start bit is bit 0
    npu.write(CTRL_OFFSET, ctrl_start_bit);

    // Step 5.2: Read STATUS SFR to check if done bit is set
    uint32_t status_value = 0;
    npu.read(STATUS_OFFSET, status_value);

    // Assuming done bit is bit 0
    if (status_value & 0x1) {
        std::cout << "NPU Operation: SUCCESS (Done bit is set)" << std::endl;
    }
    else {
        std::cout << "NPU Operation: FAILURE (Done bit is not set)" << std::endl;
    }

    return 0;
}

// main.cpp

#include <iostream>
#include "sfr.h"

int main()
{
    // Initialize 5 SFRs with given parameters
    SFR ctrl(0x00000000, 0xFFFFFFFE, 0x00000001, 0xFFFFFFFF, 0x00000000, 0x0);
    SFR status(0x00000004, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000003, 0x0);
    SFR ifm_info(0x00000010, 0xFFFFFFFF, 0x0FFF0FFF, 0xFFFFFFFF, 0x00000000, 0x0);
    SFR wt_info(0x00000014, 0xFFFFFFFF, 0x0FFF0FFF, 0xFFFFFFFF, 0x00000000, 0x0);
    SFR ofm_info(0x00000018, 0xFFFFFFFF, 0x0FFF0FFF, 0xFFFFFFFF, 0x00000000, 0x0);

    // Array of SFR pointers and their names for easy iteration
    SFR* sfrs[] = { &ctrl, &status, &ifm_info, &wt_info, &ofm_info };
    const char* sfr_names[] = { "ctrl", "status", "ifm_info", "wt_info", "ofm_info" };
    const size_t num_sfrs = sizeof(sfrs) / sizeof(sfrs[0]);

    // Value to write
    uint32_t write_value = 0xFFFFFFFF;

    // Perform external/SW write to each SFR
    for (size_t i = 0; i < num_sfrs; ++i) {
        sfrs[i]->write(write_value);
    }

    // Verify the read values
    for (size_t i = 0; i < num_sfrs; ++i) {
        uint32_t read_value = 0;
        sfrs[i]->read(read_value);

        // Determine the expected value based on the SW read mask
        uint32_t expected_value;
        switch (i) {
            case 0: // ctrl
                expected_value = write_value & 0xFFFFFFFE;
                break;
            case 1: // status
                expected_value = write_value & 0xFFFFFFFF;
                break;
            case 2: // ifm_info
            case 3: // wt_info
            case 4: // ofm_info
                expected_value = write_value & 0x0FFF0FFF;
                break;
            default:
                expected_value = 0;
        }

        // Check if the read value matches the expected value
        if (read_value == expected_value) {
            std::cout << sfr_names[i] << " read value matches expected value: 0x" 
                      << std::hex << read_value << std::dec << std::endl;
        }
        else {
            std::cout << sfr_names[i] << " read value does NOT match expected value." 
                      << " Expected: 0x" << std::hex << expected_value 
                      << ", Got: 0x" << read_value << std::dec << std::endl;
        }
    }

    return 0;
}

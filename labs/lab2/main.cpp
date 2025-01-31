#include <iostream>
#include "npu.h"

// Define a test value for writing
#define SAM_NPU_TEST_WRITE_VALUE 0xFFFFFFFF

int main() {
    // Instantiate NPU class
    NPU I_npu;

    uint32_t I_read_value = 0;
    bool I_success = true;

    // Write to IFM Width SFR
    I_npu.write(SAM_NPU_IFM_WIDTH_OFFSET, 0x10);
    I_npu.read(SAM_NPU_IFM_WIDTH_OFFSET, I_read_value);
    if (I_read_value != 0x10) {
        std::cout << "IFM Width Read Mismatch: 0x" << std::hex << I_read_value << " != 0x10\n";
        I_success = false;
    } else {
        std::cout << "IFM Width Read Success: 0x" << std::hex << I_read_value << "\n";
    }

    // Write to IFM Height SFR
    I_npu.write(SAM_NPU_IFM_HEIGHT_OFFSET, 18);
    I_npu.read(SAM_NPU_IFM_HEIGHT_OFFSET, I_read_value);
    if (I_read_value != 18) {
        std::cout << "IFM Height Read Mismatch: 0x" << std::hex << I_read_value << " != 0x12\n";
        I_success = false;
    } else {
        std::cout << "IFM Height Read Success: 0x" << std::hex << I_read_value << "\n";
    }

    // Write to OFM Width SFR
    I_npu.write(SAM_NPU_OFM_WIDTH_OFFSET, 14);
    I_npu.read(SAM_NPU_OFM_WIDTH_OFFSET, I_read_value);
    if (I_read_value != 14) {
        std::cout << "OFM Width Read Mismatch: 0x" << std::hex << I_read_value << " != 0xE\n";
        I_success = false;
    } else {
        std::cout << "OFM Width Read Success: 0x" << std::hex << I_read_value << "\n";
    }

    // Write to OFM Height SFR
    I_npu.write(SAM_NPU_OFM_HEIGHT_OFFSET, 16);
    I_npu.read(SAM_NPU_OFM_HEIGHT_OFFSET, I_read_value);
    if (I_read_value != 16) {
        std::cout << "OFM Height Read Mismatch: 0x" << std::hex << I_read_value << " != 0x10\n";
        I_success = false;
    } else {
        std::cout << "OFM Height Read Success: 0x" << std::hex << I_read_value << "\n";
    }

    // Write to Weight Width SFR
    I_npu.write(SAM_NPU_WEIGHT_WIDTH_OFFSET, 0x3);
    I_npu.read(SAM_NPU_WEIGHT_WIDTH_OFFSET, I_read_value);
    if (I_read_value != 0x3) {
        std::cout << "Weight Width Read Mismatch: 0x" << std::hex << I_read_value << " != 0x3\n";
        I_success = false;
    } else {
        std::cout << "Weight Width Read Success: 0x" << std::hex << I_read_value << "\n";
    }

    // Write to Weight Height SFR
    I_npu.write(SAM_NPU_WEIGHT_HEIGHT_OFFSET, 0x3);
    I_npu.read(SAM_NPU_WEIGHT_HEIGHT_OFFSET, I_read_value);
    if (I_read_value != 0x3) {
        std::cout << "Weight Height Read Mismatch: 0x" << std::hex << I_read_value << " != 0x3\n";
        I_success = false;
    } else {
        std::cout << "Weight Height Read Success: 0x" << std::hex << I_read_value << "\n";
    }

    // Configure NPU with specified parameters
    I_npu.write(SAM_NPU_IFM_WIDTH_OFFSET, 0x10);    // IFM Width = 0x10
    I_npu.write(SAM_NPU_IFM_HEIGHT_OFFSET, 18);     // IFM Height = 18
    I_npu.write(SAM_NPU_OFM_WIDTH_OFFSET, 14);      // OFM Width = 14
    I_npu.write(SAM_NPU_OFM_HEIGHT_OFFSET, 16);     // OFM Height = 16
    I_npu.write(SAM_NPU_WEIGHT_WIDTH_OFFSET, 0x3);  // Weight Width = 0x3
    I_npu.write(SAM_NPU_WEIGHT_HEIGHT_OFFSET, 0x3); // Weight Height = 0x3

    // Write 1 to CTRL SFR start bit to trigger NPU
    I_npu.write(SAM_NPU_CTRL_OFFSET, SAM_NPU_CTRL_START_BIT);

    // Read STATUS SFR to check if done bit is set
    I_npu.read(SAM_NPU_STATUS_OFFSET, I_read_value);
    if (I_read_value & SAM_NPU_STATUS_DONE_BIT) {
        std::cout << "NPU Operation Status: SUCCESS\n";
    } else {
        std::cout << "NPU Operation Status: FAILURE\n";
        I_success = false;
    }

    // Final Success or Failure Message
    if (I_success) {
        std::cout << "All SFR Read/Write Operations Successful.\n";
    } else {
        std::cout << "Some SFR Read/Write Operations Failed.\n";
    }

    return 0;
}

#include "sfr.h"
#include <iostream>

#define SAM_SFR_TEST_VALUE 0xFFFFFFFF

int main() {
    // Creating 5 SFR objects with the specified configurations
    SFR ctrl(0x00000000, 0xFFFFFFFE, 0x00000001, 0xFFFFFFFF, 0x00000000, 0x0);
    SFR status(0x00000004, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000003, 0x0);
    SFR ifm_info(0x00000010, 0xFFFFFFFF, 0x0FFF0FFF0, 0xFFFFFFFF, 0x00000000, 0x0);
    SFR wt_info(0x00000014, 0xFFFFFFFF, 0x0FFF0FFF0, 0xFFFFFFFF, 0x00000000, 0x0);
    SFR ofm_info(0x00000018, 0xFFFFFFFF, 0x0FFF0FFF0, 0xFFFFFFFF, 0x00000000, 0x0);

    uint32_t I_read_value = 0;

    // Writing and verifying ctrl
    ctrl.write(SAM_SFR_TEST_VALUE);
    ctrl.read(I_read_value);
    std::cout << "CTRL Read Value: 0x" << std::hex << I_read_value
              << " (Expected: 0x" << (SAM_SFR_TEST_VALUE & 0xFFFFFFFE) << ")\n";

    // Writing and verifying status
    status.write(SAM_SFR_TEST_VALUE);
    status.read(I_read_value);
    std::cout << "STATUS Read Value: 0x" << std::hex << I_read_value
              << " (Expected: 0x" << (SAM_SFR_TEST_VALUE & 0xFFFFFFFF) << ")\n";

    // Writing and verifying ifm_info
    ifm_info.write(SAM_SFR_TEST_VALUE);
    ifm_info.read(I_read_value);
    std::cout << "IFM_INFO Read Value: 0x" << std::hex << I_read_value
              << " (Expected: 0x" << (SAM_SFR_TEST_VALUE & 0x0FFF0FFF0) << ")\n";

    // Writing and verifying wt_info
    wt_info.write(SAM_SFR_TEST_VALUE);
    wt_info.read(I_read_value);
    std::cout << "WT_INFO Read Value: 0x" << std::hex << I_read_value
              << " (Expected: 0x" << (SAM_SFR_TEST_VALUE & 0x0FFF0FFF0) << ")\n";

    // Writing and verifying ofm_info
    ofm_info.write(SAM_SFR_TEST_VALUE);
    ofm_info.read(I_read_value);
    std::cout << "OFM_INFO Read Value: 0x" << std::hex << I_read_value
              << " (Expected: 0x" << (SAM_SFR_TEST_VALUE & 0x0FFF0FFF0) << ")\n";

    return 0;
}

#ifndef NPU_H
#define NPU_H

#include <cstdint>

// Define constants for NPU SFRs
#define SAM_NPU_CTRL_OFFSET          0x1000
#define SAM_NPU_STATUS_OFFSET        0x1004
#define SAM_NPU_IFM_WIDTH_OFFSET     0x1008
#define SAM_NPU_IFM_HEIGHT_OFFSET    0x100C
#define SAM_NPU_OFM_WIDTH_OFFSET     0x1010
#define SAM_NPU_OFM_HEIGHT_OFFSET    0x1014
#define SAM_NPU_WEIGHT_WIDTH_OFFSET  0x1018
#define SAM_NPU_WEIGHT_HEIGHT_OFFSET 0x101C

// Define bit masks
#define SAM_NPU_CTRL_START_BIT       0x1
#define SAM_NPU_STATUS_DONE_BIT      0x1

class NPU {
public:
    // Constructor
    NPU();

    // Destructor
    ~NPU();

    // Write to a specific SFR based on offset
    void write(uint32_t a_offset, const uint32_t &a_value);

    // Read from a specific SFR based on offset
    void read(uint32_t a_offset, uint32_t &a_value) const;

private:
    // Forward declaration of the implementation class
    class NPUImpl;

    // Pointer to the implementation
    NPUImpl* m_impl;

    // Initiates the NPU operation
    void start_operation();
};

#endif // NPU_H

// npu.h

#ifndef NPU_H
#define NPU_H

#include <cstdint>
#include <unordered_map>
#include "sfr.h"

class NPU {
public:
    // Constructor
    NPU();

    // Destructor
    ~NPU();

    // Write function: Takes offset and value
    void write(uint32_t offset, const uint32_t& value);

    // Read function: Takes offset and reference to store value
    void read(uint32_t offset, uint32_t& value) const;

    // Configuration functions
    void configure_ifm(uint32_t width, uint32_t height);
    void configure_ofm(uint32_t width, uint32_t height);
    void configure_weight(uint32_t width, uint32_t height);

private:
    // SFRs for NPU
    SFR ctrl;
    SFR status;
    SFR ifm_info;
    SFR wt_info;
    SFR ofm_info;

    // Mapping from offset to SFR pointers for easy access
    std::unordered_map<uint32_t, SFR*> sfr_map;

    // Private function to start NPU operation
    void start_operation();
};

#endif // NPU_H

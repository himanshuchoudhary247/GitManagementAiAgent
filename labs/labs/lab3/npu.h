#ifndef NPU_H
#define NPU_H

#include <systemc.h>
#include "sfr.h"

/**
 * NPU module instantiates five SFR modules:
 *  1. ctrl
 *  2. status
 *  3. ifm_info
 *  4. wt_info
 *  5. ofm_info
 *
 * It provides external functions to write to and read from these registers.
 */
SC_MODULE(NPU) {
public:
    SC_HAS_PROCESS(NPU);

    NPU(sc_module_name name);
    ~NPU();

    // External methods: all return void.
    void write(uint32_t offset, const uint32_t &value);
    void read(uint32_t offset, uint32_t &value) const;

private:
    SFR* ctrl;
    SFR* status;
    SFR* ifm_info;
    SFR* wt_info;
    SFR* ofm_info;
};

#endif // NPU_H

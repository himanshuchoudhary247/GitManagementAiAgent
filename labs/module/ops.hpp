#ifndef OPS_HPP
#define OPS_HPP

#include <systemc.h>

/**
 * Single-cycle lane-level floating operations for either BF16 or FP32.
 * Each function does naive alignment, add/sub, normalization in a single call.
 * Real hardware would be more refined, but we keep it “one shot.”
 */

// Lane-level FP32 add (single function, naive)
sc_uint<32> fp32_add_1cycle(sc_uint<32> a, sc_uint<32> b);
// Lane-level FP32 sub (one cycle)
sc_uint<32> fp32_sub_1cycle(sc_uint<32> a, sc_uint<32> b);

// Lane-level BF16 add (single function, naive)
sc_uint<32> bf16_add_1cycle(sc_uint<32> a, sc_uint<32> b);
// Lane-level BF16 sub (one cycle)
sc_uint<32> bf16_sub_1cycle(sc_uint<32> a, sc_uint<32> b);

#endif

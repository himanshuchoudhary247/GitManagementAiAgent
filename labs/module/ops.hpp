#ifndef OPS_HPP
#define OPS_HPP

#include <systemc.h>

/**
 * Single-cycle, combinational operations for FP32 or BF16:
 *   - Add
 *   - Sub
 *   - Mul
 * Each function includes optional:
 *   - Subnormal normalization (enable_subnorm)
 *   - Truncation vs. round half-up (enable_trunc)
 *   - Clamping large exponents or integer range (enable_clamp)
 *   - Exception checks for Inf/NaN (enable_except)
 */

sc_uint<32> fp32_add_1c(sc_uint<32> a, sc_uint<32> b,
                        bool enable_subnorm, bool enable_trunc,
                        bool enable_clamp,   bool enable_except);

sc_uint<32> fp32_sub_1c(sc_uint<32> a, sc_uint<32> b,
                        bool enable_subnorm, bool enable_trunc,
                        bool enable_clamp,   bool enable_except);

sc_uint<32> fp32_mul_1c(sc_uint<32> a, sc_uint<32> b,
                        bool enable_subnorm, bool enable_trunc,
                        bool enable_clamp,   bool enable_except);

sc_uint<32> bf16_add_1c(sc_uint<32> a, sc_uint<32> b,
                        bool enable_subnorm, bool enable_trunc,
                        bool enable_clamp,   bool enable_except);

sc_uint<32> bf16_sub_1c(sc_uint<32> a, sc_uint<32> b,
                        bool enable_subnorm, bool enable_trunc,
                        bool enable_clamp,   bool enable_except);

sc_uint<32> bf16_mul_1c(sc_uint<32> a, sc_uint<32> b,
                        bool enable_subnorm, bool enable_trunc,
                        bool enable_clamp,   bool enable_except);

#endif

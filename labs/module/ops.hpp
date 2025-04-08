/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: ops.hpp
 * Description: Declares single-cycle FP32 and BF16 operations (add, sub, mul).
 *              The function prototypes remain unchanged. A global context
 *              (set via setOpsContext) is used internally for subnorm/trunc/clamp/except.
 **********/
#pragma once
#include <systemc.h>

/**
 * Set the global context from outside (SFR or pipeline).
 * - enableSubnorm: whether subnormal numbers get normalized
 * - enableTrunc:   if true, we truncate (instead of round half-up)
 * - enableClamp:   if true, clamp exponent or integer range on overflow
 * - enableExcept:  if true, treat NaN/Inf more carefully
 */
void setOpsContext(bool enableSubnorm,
                   bool enableTrunc,
                   bool enableClamp,
                   bool enableExcept);

/**
 * Single-cycle FP32 operations, using the global context:
 *   fp32_add_1c(a,b), fp32_sub_1c(a,b), fp32_mul_1c(a,b)
 */
sc_uint<32> fp32_add_1c(sc_uint<32> a, sc_uint<32> b);
sc_uint<32> fp32_sub_1c(sc_uint<32> a, sc_uint<32> b);
sc_uint<32> fp32_mul_1c(sc_uint<32> a, sc_uint<32> b);

/**
 * Single-cycle BF16 operations, using the global context:
 *   bf16_add_1c(a,b), bf16_sub_1c(a,b), bf16_mul_1c(a,b)
 */
sc_uint<32> bf16_add_1c(sc_uint<32> a, sc_uint<32> b);
sc_uint<32> bf16_sub_1c(sc_uint<32> a, sc_uint<32> b);
sc_uint<32> bf16_mul_1c(sc_uint<32> a, sc_uint<32> b);


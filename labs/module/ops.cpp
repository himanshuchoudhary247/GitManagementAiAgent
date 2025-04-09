/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: ops.cpp
 * Description: Implements single‐cycle FP32 and BF16 arithmetic operations (Add, Sub, Mul).
 *              This module handles the implicit “1” for normalized numbers, uses a wider
 *              accumulator for addition to detect carry-out, and applies rounding (truncation
 *              or round‑half‑up), clamping, and exception processing as dictated by the global
 *              context. Debug prints are provided when DEBUG_MODE is enabled.
 **********/
#include "ops.hpp"
#include <iostream>
#include <cstdint>

// Enable debug prints if needed
static const bool DEBUG_MODE = true;

// Global context structure for control flags
static struct {
    bool enable_subnorm;
    bool enable_trunc;
    bool enable_clamp;
    bool enable_except;
} g_ops;

// Set global context; called externally
void setOpsContext(bool subnorm, bool trunc, bool clamp, bool except) {
    g_ops.enable_subnorm = subnorm;
    g_ops.enable_trunc   = trunc;
    g_ops.enable_clamp   = clamp;
    g_ops.enable_except  = except;
}

// ---------------------- FP32 HELPER FUNCTIONS ----------------------

// Decode a 32‐bit floating‐point value into its sign, exponent, and mantissa.
static void decode_fp32(sc_uint<32> val, sc_uint<1>& s, sc_uint<8>& e, sc_uint<23>& m)
{
    s = val[31];
    e = val.range(30, 23);
    m = val.range(22, 0);
}

// Encode sign, exponent, and mantissa into a 32‐bit floating‐point value.
static sc_uint<32> encode_fp32(sc_uint<1> s, sc_uint<8> e, sc_uint<23> m)
{
    sc_uint<32> out = 0;
    out[31] = s;
    out.range(30,23) = e;
    out.range(22,0) = m;
    return out;
}

// Return true if the exponent is 255 and mantissa is 0.
static bool is_fp32_inf(sc_uint<8> e, sc_uint<23> m) {
    return (e == 255 && m == 0);
}

// Return true if the exponent is 255 and mantissa is nonzero.
static bool is_fp32_nan(sc_uint<8> e, sc_uint<23> m) {
    return (e == 255 && m != 0);
}

// Return true if exponent equals 0 and mantissa is nonzero.
static bool is_fp32_subnormal(sc_uint<8> e, sc_uint<23> m) {
    return (e == 0 && m != 0);
}

// Clamp exponent (if too high) and set mantissa to maximal if needed.
static void clamp_exponent_fp32(sc_uint<8>& e, sc_uint<23>& m)
{
    if(e > 254) {
        e = 254;
        m = (1 << 23) - 1;
    }
}

/**
 * finalize_round_fp32: Given a 25‑bit sum (including carry), 
 * apply rounding (truncation or round-half‑up) and return a 23‑bit mantissa.
 */
static sc_uint<23> finalize_round_fp32(sc_uint<25> sum)
{
    if(g_ops.enable_trunc) {
        sum >>= 1;  // Truncate: simply drop the lowest bit
    } else {
        // Round half-up
        bool roundBit = sum[0].to_bool();  // cast the least significant bit to bool
        sum >>= 1;
        if(roundBit)
            sum += 1;
    }
    return sum.range(22, 0);
}

// ---------------------- FP32 ADD ----------------------
sc_uint<32> fp32_add_1c(sc_uint<32> a, sc_uint<32> b)
{
    // Decode FP32 inputs.
    sc_uint<1> sA, sB;
    sc_uint<8> eA, eB;
    sc_uint<23> mA, mB;
    decode_fp32(a, sA, eA, mA);
    decode_fp32(b, sB, eB, mB);

    if(g_ops.enable_except) {
        if(is_fp32_nan(eA, mA) || is_fp32_nan(eB, mB))
            return encode_fp32(0, 255, 1);
        if(is_fp32_inf(eA, mA)) return a;
        if(is_fp32_inf(eB, mB)) return b;
    }

    // If subnormals are enabled, normalize them by forcing an exponent of 1.
    if(g_ops.enable_subnorm) {
        if(is_fp32_subnormal(eA, mA))
            eA = 1;
        if(is_fp32_subnormal(eB, mB))
            eB = 1;
    }

    // If the signs differ, use subtraction.
    if(sA != sB)
        return fp32_sub_1c(a, b);

    // Determine the maximum and minimum exponents.
    sc_uint<8> eMax = (eA > eB) ? eA : eB;
    sc_uint<8> eMin = (eA > eB) ? eB : eA;

    // Restore implicit leading 1 (for normalized numbers) to form 24-bit significands.
    sc_uint<24> bigM = (1 << 23) | ((eA > eB) ? mA : mB);
    sc_uint<24> smlM = (1 << 23) | ((eA > eB) ? mB : mA);

    // Align smaller mantissa: compute exponent difference.
    sc_uint<8> diff = eMax - eMin;
    sc_uint<24> alignedSml = smlM >> diff;

    // Use a 25-bit accumulator to sum the mantissas.
    sc_uint<25> sumVal = (sc_uint<25>)bigM + (sc_uint<25>)alignedSml;

    // Check if there is a carry-out in the 25th bit.
    if(sumVal[24]) {
        sumVal >>= 1;
        eMax++;
    }

    // Finalize by rounding the 25-bit result down to 23 bits.
    sc_uint<23> finalMant = finalize_round_fp32(sumVal);
    if(g_ops.enable_clamp)
        clamp_exponent_fp32(eMax, finalMant);

    sc_uint<32> result = encode_fp32(sA, eMax, finalMant);

    if(DEBUG_MODE) {
        std::cout << "[FP32 ADD] a=0x" << std::hex << a 
                  << ", b=0x" << b 
                  << ", result=0x" << result << std::dec << std::endl;
    }
    return result;
}

// ---------------------- FP32 SUB ----------------------
/*
 * fp32_sub_1c converts subtraction into addition by negating the sign of b.
 * Note: This is a simple method that assumes the numbers are normalized.
 */
sc_uint<32> fp32_sub_1c(sc_uint<32> a, sc_uint<32> b)
{
    sc_uint<32> bNeg = b;
    bNeg[31] = ~b[31];
    if(DEBUG_MODE) {
        std::cout << "[FP32 SUB] a=0x" << std::hex << a 
                  << ", b=0x" << b 
                  << ", bNeg=0x" << bNeg << std::dec << std::endl;
    }
    return fp32_add_1c(a, bNeg);
}

// ---------------------- FP32 MUL ----------------------
sc_uint<32> fp32_mul_1c(sc_uint<32> a, sc_uint<32> b)
{
    sc_uint<1> sA, sB;
    sc_uint<8> eA, eB;
    sc_uint<23> mA, mB;
    decode_fp32(a, sA, eA, mA);
    decode_fp32(b, sB, eB, mB);
    sc_uint<1> outSign = sA ^ sB;

    if(g_ops.enable_except) {
        if(is_fp32_nan(eA, mA) || is_fp32_nan(eB, mB))
            return encode_fp32(0, 255, 1);
        if(is_fp32_inf(eA, mA) || is_fp32_inf(eB, mB))
            return encode_fp32(outSign, 255, 0);
    }
    if(g_ops.enable_subnorm) {
        if(is_fp32_subnormal(eA, mA))
            eA = 1;
        if(is_fp32_subnormal(eB, mB))
            eB = 1;
    }
    
    int eSum = (int)eA + (int)eB - 127;
    if(eSum < 1) eSum = 1;
    else if(eSum > 254) {
        if(g_ops.enable_clamp)
            return encode_fp32(outSign, 254, (1 << 23)-1);
        else
            return encode_fp32(outSign, 255, 0);
    }
    sc_uint<24> bigA = (1 << 23) | mA;
    sc_uint<24> bigB = (1 << 23) | mB;
    uint64_t prod = (uint64_t)bigA * (uint64_t)bigB;
    if((prod >> 47) & 1) {
        prod >>= 1;
        eSum++;
    }
    sc_uint<24> mid = (prod >> 23) & 0xFFFFFF;
    sc_uint<23> finalMant = finalize_round_fp32(mid);
    sc_uint<8> outE = eSum;
    if(g_ops.enable_clamp && outE > 254) {
        outE = 254;
        finalMant = (1 << 23) - 1;
    }
    return encode_fp32(outSign, outE, finalMant);
}

// ---------------------- BF16 HELPER FUNCTIONS ----------------------

// Decode BF16: bits[31] is the sign; bits [30:23] store exponent; bits [22:16] store 7-bit mantissa.
static void decode_bf16(sc_uint<32> val, sc_uint<1>& s, sc_uint<8>& e, sc_uint<7>& m)
{
    s = val[31];
    e = val.range(30,23);
    m = val.range(22,16);
}

// Encode BF16.
static sc_uint<32> encode_bf16(sc_uint<1> s, sc_uint<8> e, sc_uint<7> m)
{
    sc_uint<32> out = 0;
    out[31] = s;
    out.range(30,23) = e;
    out.range(22,16) = m;
    return out;
}

static bool is_bf16_inf(sc_uint<8> e, sc_uint<7> m) {
    return (e == 255 && m == 0);
}
static bool is_bf16_nan(sc_uint<8> e, sc_uint<7> m) {
    return (e == 255 && m != 0);
}
static bool is_bf16_subnormal(sc_uint<8> e, sc_uint<7> m) {
    return (e == 0 && m != 0);
}

static void clamp_exponent_bf16(sc_uint<8>& e, sc_uint<7>& m)
{
    if(e > 254) {
        e = 254;
        m = (1 << 7) - 1;
    }
}

/**
 * finalize_round_bf16: Given an 8-bit value (with one extra rounding bit),
 * perform rounding based on the global context and return a 7-bit mantissa.
 */
static sc_uint<7> finalize_round_bf16(sc_uint<8> sum)
{
    if(g_ops.enable_trunc) {
        return sum.range(6,0);
    } else {
        bool roundBit = sum[0].to_bool();
        sum >>= 1;
        if(roundBit)
            sum += 1;
        return sum.range(6,0);
    }
}

// ---------------------- BF16 ADD ----------------------
sc_uint<32> bf16_add_1c(sc_uint<32> a, sc_uint<32> b)
{
    sc_uint<1> sA, sB;
    sc_uint<8> eA, eB;
    sc_uint<7> mA, mB;
    decode_bf16(a, sA, eA, mA);
    decode_bf16(b, sB, eB, mB);

    if(g_ops.enable_except) {
        if(is_bf16_nan(eA, mA) || is_bf16_nan(eB, mB))
            return encode_bf16(0, 255, 1);
        if(is_bf16_inf(eA, mA))
            return a;
        if(is_bf16_inf(eB, mB))
            return b;
    }

    if(g_ops.enable_subnorm) {
        if(is_bf16_subnormal(eA, mA))
            eA = 1;
        if(is_bf16_subnormal(eB, mB))
            eB = 1;
    }
    if(sA != sB)
        return bf16_sub_1c(a, b);

    sc_uint<8> eMax = (eA > eB) ? eA : eB;
    sc_uint<8> diff = (eA > eB) ? (eA - eB) : (eB - eA);

    // Restore the implicit bit for BF16 (7 bits) by OR-ing with (1 << 7)
    sc_uint<8> bigM = (1 << 7) | ((eA > eB) ? mA : mA); // note: for BF16, both indexes are the same when same sign
    sc_uint<8> smlM = (1 << 7) | ((eA > eB) ? mB : mB);

    // Align the smaller mantissa.
    sc_uint<8> extSml = smlM >> diff;
    sc_uint<8> sumVal = bigM + extSml;
    if(sumVal[7] == 1) {
        sumVal >>= 1;
        eMax++;
    }
    sc_uint<7> finalMant = finalize_round_bf16(sumVal);
    if(g_ops.enable_clamp)
        clamp_exponent_bf16(eMax, finalMant);

    return encode_bf16(sA, eMax, finalMant);
}

// ---------------------- BF16 SUB ----------------------
sc_uint<32> bf16_sub_1c(sc_uint<32> a, sc_uint<32> b)
{
    sc_uint<1> sA, sB;
    sc_uint<8> eA, eB;
    sc_uint<7> mA, mB;
    decode_bf16(a, sA, eA, mA);
    decode_bf16(b, sB, eB, mB);

    if(g_ops.enable_except) {
        if(is_bf16_nan(eA, mA) || is_bf16_nan(eB, mB))
            return encode_bf16(0, 255, 1);
        if(is_bf16_inf(eA, mA) && is_bf16_inf(eB, mB))
            return encode_bf16(0, 255, 1);
        if(is_bf16_inf(eA, mA))
            return a;
        if(is_bf16_inf(eB, mB))
            return encode_bf16(~sB, 255, 0);
    }
    if(g_ops.enable_subnorm) {
        if(is_bf16_subnormal(eA, mA))
            eA = 1;
        if(is_bf16_subnormal(eB, mB))
            eB = 1;
    }

    if(sA != sB) {
        sc_uint<32> bNeg = b;
        bNeg[31] = ~sB;
        return bf16_add_1c(a, bNeg);
    } else {
        sc_uint<8> eMax = (eA > eB) ? eA : eB;
        sc_uint<8> diff = (eA > eB) ? (eA - eB) : (eB - eA);
        sc_uint<8> bigM = (1 << 7) | ((eA > eB) ? mA : mA);
        sc_uint<8> smlM = (1 << 7) | ((eA > eB) ? mB : mB);
        sc_uint<1> outSign = (eA > eB) ? sA : sB;
        sc_uint<8> ext = smlM >> diff;

        int dif = (int)bigM - (int)ext;
        if(dif < 0) {
            outSign = ~outSign;
            dif = -dif;
        }

        sc_uint<8> mag = (sc_uint<8>)dif;
        while(mag[7] == 0 && mag != 0 && eMax > 0) {
            mag <<= 1;
            eMax--;
        }
        sc_uint<7> finalMant = finalize_round_bf16(mag);
        if(g_ops.enable_clamp)
            clamp_exponent_bf16(eMax, finalMant);
        return encode_bf16(outSign, eMax, finalMant);
    }
}

// ---------------------- BF16 MUL ----------------------
sc_uint<32> bf16_mul_1c(sc_uint<32> a, sc_uint<32> b)
{
    sc_uint<1> sA, sB;
    sc_uint<8> eA, eB;
    sc_uint<7> mA, mB;
    decode_bf16(a, sA, eA, mA);
    decode_bf16(b, sB, eB, mB);
    sc_uint<1> outSign = sA ^ sB;

    if(g_ops.enable_except) {
        if(is_bf16_nan(eA, mA) || is_bf16_nan(eB, mB))
            return encode_bf16(0,255,1);
        bool infA = is_bf16_inf(eA, mA);
        bool infB = is_bf16_inf(eB, mB);
        if(infA || infB) {
            if((infA && eB != 0) || (infB && eA != 0))
                return encode_bf16(outSign,255,0);
            else
                return encode_bf16(0,255,1);
        }
    }
    if(g_ops.enable_subnorm) {
        if(is_bf16_subnormal(eA, mA))
            eA = 1;
        if(is_bf16_subnormal(eB, mB))
            eB = 1;
    }
    int eSum = (int)eA + (int)eB - 127;
    if(eSum < 1) eSum = 1;
    else if(eSum > 254) {
        if(g_ops.enable_clamp)
            return encode_bf16(outSign,254,(1<<7)-1);
        else
            return encode_bf16(outSign,255,0);
    }
    sc_uint<8> bigA = (1 << 7) | mA;
    sc_uint<8> bigB = (1 << 7) | mB;
    unsigned int prod = (unsigned int)bigA * (unsigned int)bigB;
    if((prod >> 15) & 1) {
        prod >>= 1;
        eSum++;
    }
    sc_uint<8> mid = (prod >> 7) & 0xFF;
    sc_uint<7> finalMant = finalize_round_bf16(mid);
    sc_uint<8> outE = eSum;
    if(g_ops.enable_clamp && outE > 254) {
        outE = 254;
        finalMant = (1 << 7) - 1;
    } else if(outE >= 255) {
        return encode_bf16(outSign,255,0);
    }
    return encode_bf16(outSign, outE, finalMant);
}
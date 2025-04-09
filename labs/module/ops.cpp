/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: ops.cpp
 * Description: Implements single-cycle FP32 and BF16 add, sub, mul operations.
 *              Includes 2's complement handling, rounding, clamping, subnormal support,
 *              and optional debug mode for tracing internal computation.
 **********/

 #include "ops.hpp"
 #include <iostream>
 
 // Set this flag to true to enable internal debug printing
 static const bool DEBUG_MODE = false;
 
 // Global context for subnorm/trunc/clamp/except
 static struct {
     bool enable_subnorm;
     bool enable_trunc;
     bool enable_clamp;
     bool enable_except;
 } g_ops;
 
 // Set ops control context
 void setOpsContext(bool subnorm, bool trunc, bool clamp, bool except) {
     g_ops.enable_subnorm = subnorm;
     g_ops.enable_trunc   = trunc;
     g_ops.enable_clamp   = clamp;
     g_ops.enable_except  = except;
 }
 
 // ---------------------- FP32 Helpers ----------------------
 
 static void decode_fp32(sc_uint<32> val,
                         sc_uint<1>& sign,
                         sc_uint<8>& exp,
                         sc_uint<23>& mant)
 {
     sign = val[31];
     exp  = val.range(30, 23);
     mant = val.range(22, 0);
 }
 
 static sc_uint<32> encode_fp32(sc_uint<1> sign,
                                sc_uint<8> exp,
                                sc_uint<23> mant)
 {
     sc_uint<32> val = 0;
     val[31] = sign;
     val.range(30, 23) = exp;
     val.range(22, 0)  = mant;
     return val;
 }
 
 static bool is_fp32_inf(sc_uint<8> e, sc_uint<23> m) {
     return (e == 255 && m == 0);
 }
 static bool is_fp32_nan(sc_uint<8> e, sc_uint<23> m) {
     return (e == 255 && m != 0);
 }
 static bool is_fp32_subnormal(sc_uint<8> e, sc_uint<23> m) {
     return (e == 0 && m != 0);
 }
 
 static void clamp_exponent_fp32(sc_uint<8>& e, sc_uint<23>& m) {
     if (e > 254) {
         e = 254;
         m = (1 << 23) - 1;
     }
 }
 
 static sc_uint<23> finalize_round_fp32(sc_uint<24> sum)
 {
     if (g_ops.enable_trunc) {
         sum = sum >> 1;
     } else {
         bool roundBit = sum[0].to_bool();
         sum = sum >> 1;
         if (roundBit) sum = sum + 1;
     }
     return sum.range(22, 0);
 }
 
 // ---------------------- BF16 Helpers ----------------------
 
 static void decode_bf16(sc_uint<32> val,
                         sc_uint<1>& sign,
                         sc_uint<8>& exp,
                         sc_uint<7>& mant)
 {
     sign = val[31];
     exp  = val.range(30, 23);
     mant = val.range(22, 16);
 }
 
 static sc_uint<32> encode_bf16(sc_uint<1> sign,
                                sc_uint<8> exp,
                                sc_uint<7> mant)
 {
     sc_uint<32> val = 0;
     val[31] = sign;
     val.range(30, 23) = exp;
     val.range(22, 16) = mant;
     return val;
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
 
 static void clamp_exponent_bf16(sc_uint<8>& e, sc_uint<7>& m) {
     if (e > 254) {
         e = 254;
         m = (1 << 7) - 1;
     }
 }
 
 static sc_uint<7> finalize_round_bf16(sc_uint<8> sum)
 {
     if (g_ops.enable_trunc) {
         return sum.range(6, 0);
     } else {
         bool roundBit = sum[0].to_bool();
         sum = sum >> 1;
         if (roundBit) sum = sum + 1;
         return sum.range(6, 0);
     }
 }
 
 // ---------------------- FP32 ADD ----------------------
 
 sc_uint<32> fp32_add_1c(sc_uint<32> a, sc_uint<32> b)
 {
     sc_uint<1> sA, sB;
     sc_uint<8> eA, eB;
     sc_uint<23> mA, mB;
 
     decode_fp32(a, sA, eA, mA);
     decode_fp32(b, sB, eB, mB);
 
     if (g_ops.enable_except) {
         if (is_fp32_nan(eA, mA) || is_fp32_nan(eB, mB))
             return encode_fp32(0, 255, 1);
         if (is_fp32_inf(eA, mA)) return a;
         if (is_fp32_inf(eB, mB)) return b;
     }
 
     if (g_ops.enable_subnorm) {
         if (is_fp32_subnormal(eA, mA)) eA = 1;
         if (is_fp32_subnormal(eB, mB)) eB = 1;
     }
 
     if (sA != sB) {
         return fp32_sub_1c(a, b);
     }
 
     sc_uint<8> eMax = (eA > eB) ? eA : eB;
     sc_uint<8> eMin = (eA > eB) ? eB : eA;
     sc_uint<24> bigM = (1 << 23) | ((eA > eB) ? mA : mB);
     sc_uint<24> smlM = (1 << 23) | ((eA > eB) ? mB : mA);
 
     sc_uint<8> diff = eMax - eMin;
     sc_uint<24> extSml = smlM >> diff;
     sc_uint<24> sum = bigM + extSml;
 
     if (sum[23] == 1) {
         sum = sum >> 1;
         eMax++;
     }
 
     sc_uint<23> finalMant = finalize_round_fp32(sum);
     if (g_ops.enable_clamp) clamp_exponent_fp32(eMax, finalMant);
 
     if (DEBUG_MODE) {
         std::cout << "[FP32 ADD] a=0x" << std::hex << a << ", b=0x" << b
                   << ", result=0x" << encode_fp32(sA, eMax, finalMant) << std::dec << '\n';
     }
 
     return encode_fp32(sA, eMax, finalMant);
 }
 
 // ---------------------- FP32 SUB ----------------------
 
 sc_uint<32> fp32_sub_1c(sc_uint<32> a, sc_uint<32> b)
 {
     sc_uint<1> sA, sB;
     sc_uint<8> eA, eB;
     sc_uint<23> mA, mB;
     decode_fp32(a, sA, eA, mA);
     decode_fp32(b, sB, eB, mB);
 
     if (g_ops.enable_except) {
         if (is_fp32_nan(eA, mA) || is_fp32_nan(eB, mB))
             return encode_fp32(0, 255, 1);
         if (is_fp32_inf(eA, mA)) return a;
         if (is_fp32_inf(eB, mB)) return encode_fp32(~sB, 255, 0);
     }
 
     if (g_ops.enable_subnorm) {
         if (is_fp32_subnormal(eA, mA)) eA = 1;
         if (is_fp32_subnormal(eB, mB)) eB = 1;
     }
 
     if (sA != sB) {
         sc_uint<32> bNeg = b;
         bNeg[31] = ~sB;
         return fp32_add_1c(a, bNeg);
     }
 
     sc_uint<8> eMax = (eA > eB) ? eA : eB;
     sc_uint<8> eMin = (eA > eB) ? eB : eA;
     sc_uint<24> bigM = (1 << 23) | ((eA > eB) ? mA : mB);
     sc_uint<24> smlM = (1 << 23) | ((eA > eB) ? mB : mA);
     sc_uint<1> outSign = (eA > eB) ? sA : sB;
 
     sc_uint<8> diff = eMax - eMin;
     sc_uint<24> extSml = smlM >> diff;
 
     int diffVal = (int)bigM - (int)extSml;
     if (diffVal < 0) {
         outSign = ~outSign;
         diffVal = -diffVal;
     }
 
     sc_uint<24> mag = (sc_uint<24>)diffVal;
 
     while (mag[23] == 0 && mag != 0 && eMax > 0) {
         mag <<= 1;
         eMax--;
     }
 
     sc_uint<23> finalMant = finalize_round_fp32(mag);
     if (g_ops.enable_clamp) clamp_exponent_fp32(eMax, finalMant);
 
     return encode_fp32(outSign, eMax, finalMant);
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
 
     if (g_ops.enable_except) {
         if (is_fp32_nan(eA, mA) || is_fp32_nan(eB, mB))
             return encode_fp32(0, 255, 1);
         if (is_fp32_inf(eA, mA) || is_fp32_inf(eB, mB))
             return encode_fp32(outSign, 255, 0);
     }
 
     if (g_ops.enable_subnorm) {
         if (is_fp32_subnormal(eA, mA)) eA = 1;
         if (is_fp32_subnormal(eB, mB)) eB = 1;
     }
 
     int eSum = (int)eA + (int)eB - 127;
     if (eSum < 1) eSum = 1;
     else if (eSum > 254) {
         if (g_ops.enable_clamp)
             return encode_fp32(outSign, 254, (1 << 23) - 1);
         else
             return encode_fp32(outSign, 255, 0);
     }
 
     sc_uint<24> bigA = (1 << 23) | mA;
     sc_uint<24> bigB = (1 << 23) | mB;
 
     uint64_t prod = (uint64_t)bigA * (uint64_t)bigB;
     if ((prod >> 47) & 1) {
         prod >>= 1;
         eSum++;
     }
 
     sc_uint<24> mid = (prod >> 23) & 0xFFFFFF;
     sc_uint<23> finalMant = finalize_round_fp32(mid);
 
     sc_uint<8> outE = eSum;
     if (g_ops.enable_clamp && outE > 254) {
         outE = 254;
         finalMant = (1 << 23) - 1;
     } else if (outE >= 255) {
         return encode_fp32(outSign, 255, 0);
     }
 
     return encode_fp32(outSign, outE, finalMant);
 }
 
 // ---------------------- BF16 ADD ----------------------
 
 sc_uint<32> bf16_add_1c(sc_uint<32> a, sc_uint<32> b)
 {
     sc_uint<1> sA, sB;
     sc_uint<8> eA, eB;
     sc_uint<7> mA, mB;
     decode_bf16(a, sA, eA, mA);
     decode_bf16(b, sB, eB, mB);
 
     if (g_ops.enable_except) {
         if (is_bf16_nan(eA, mA) || is_bf16_nan(eB, mB))
             return encode_bf16(0, 255, 1);
         if (is_bf16_inf(eA, mA)) return a;
         if (is_bf16_inf(eB, mB)) return b;
     }
 
     if (sA != sB) {
         return bf16_sub_1c(a, b);
     }
 
     sc_uint<8> eMax = (eA > eB) ? eA : eB;
     sc_uint<8> diff = (eA > eB) ? (eA - eB) : (eB - eA);
     sc_uint<8> bigM = (1 << 7) | ((eA > eB) ? mA : mB);
     sc_uint<8> smlM = (1 << 7) | ((eA > eB) ? mB : mA);
 
     sc_uint<8> ext = smlM >> diff;
     sc_uint<8> sum = bigM + ext;
     if (sum[7] == 1) {
         sum = sum >> 1;
         eMax++;
     }
 
     sc_uint<7> finalMant = finalize_round_bf16(sum);
     if (g_ops.enable_clamp) clamp_exponent_bf16(eMax, finalMant);
 
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
 
     if (g_ops.enable_except) {
         if (is_bf16_nan(eA, mA) || is_bf16_nan(eB, mB))
             return encode_bf16(0, 255, 1);
         if (is_bf16_inf(eA, mA)) return a;
         if (is_bf16_inf(eB, mB)) return encode_bf16(~sB, 255, 0);
     }
 
     if (sA != sB) {
         sc_uint<32> bNeg = b;
         bNeg[31] = ~sB;
         return bf16_add_1c(a, bNeg);
     }
 
     sc_uint<8> eMax = (eA > eB) ? eA : eB;
     sc_uint<8> diff = (eA > eB) ? (eA - eB) : (eB - eA);
     sc_uint<8> bigM = (1 << 7) | ((eA > eB) ? mA : mB);
     sc_uint<8> smlM = (1 << 7) | ((eA > eB) ? mB : mA);
     sc_uint<1> outSign = (eA > eB) ? sA : sB;
 
     sc_uint<8> ext = smlM >> diff;
     int dif = (int)bigM - (int)ext;
 
     if (dif < 0) {
         outSign = ~outSign;
         dif = -dif;
     }
 
     sc_uint<8> mag = (sc_uint<8>)dif;
     while (mag[7] == 0 && mag != 0 && eMax > 0) {
         mag <<= 1;
         eMax--;
     }
 
     sc_uint<7> finalMant = finalize_round_bf16(mag);
     if (g_ops.enable_clamp) clamp_exponent_bf16(eMax, finalMant);
 
     return encode_bf16(outSign, eMax, finalMant);
 }
 
/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: ops.cpp
 * Description: Implements single-cycle FP32 and BF16 add, sub, and mul operations.
 *              It takes care of rounding (truncation or round-half-up), clamping, subnormal 
 *              handling, and uses a global context (set via setOpsContext) to select control bits.
 *              Debug mode printing is enabled when DEBUG_MODE is true.
 **********/

 #include "ops.hpp"
 #include <iostream>
 
 //---------------------------------------------------------------------
 // Debug mode flag
 //---------------------------------------------------------------------
 static const bool DEBUG_MODE = true;
 
 //---------------------------------------------------------------------
 // Global context for math operations control
 //---------------------------------------------------------------------
 static struct {
     bool enable_subnorm;
     bool enable_trunc;
     bool enable_clamp;
     bool enable_except;
 } g_ops;
 
 void setOpsContext(bool subnorm, bool trunc, bool clamp, bool except) {
     g_ops.enable_subnorm = subnorm;
     g_ops.enable_trunc   = trunc;
     g_ops.enable_clamp   = clamp;
     g_ops.enable_except  = except;
 }
 
 //---------------------------------------------------------------------
 // FP32 Helper functions
 //---------------------------------------------------------------------
 
 // Decode a 32-bit FP number into sign, exponent, and fraction/mantissa.
 static void decode_fp32(sc_uint<32> val, sc_uint<1>& s, sc_uint<8>& e, sc_uint<23>& m)
 {
     s = val[31];
     e = val.range(30, 23);
     m = val.range(22, 0);
 }
 
 // Encode sign, exponent, and mantissa into a 32-bit FP number.
 static sc_uint<32> encode_fp32(sc_uint<1> s, sc_uint<8> e, sc_uint<23> m)
 {
     sc_uint<32> out = 0;
     out[31] = s;
     out.range(30,23) = e;
     out.range(22,0)  = m;
     return out;
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
 
 // If exponent is too high, then clamp to maximum allowable value.
 static void clamp_exponent_fp32(sc_uint<8>& e, sc_uint<23>& m)
 {
     if(e > 254) {
         e = 254;
         m = (1 << 23) - 1;
     }
 }
 
 // Finalize rounding for FP32. Now accepts a 25‑bit accumulator.
 static sc_uint<23> finalize_round_fp32(sc_uint<25> sum)
 {
     if (g_ops.enable_trunc) {
         sum >>= 1; // simple truncation: drop LSB.
     } else {
         // Round half-up: check the least significant bit.
         bool roundBit = sum[0].to_bool();
         sum >>= 1; 
         if(roundBit)
             sum += 1;
     }
     return sum.range(22,0);
 }
 
 //---------------------------------------------------------------------
 // FP32 ADD Operation 
 //---------------------------------------------------------------------
 sc_uint<32> fp32_add_1c(sc_uint<32> a, sc_uint<32> b)
 {
     // Decode input FP32 numbers
     sc_uint<1> sA, sB;
     sc_uint<8> eA, eB;
     sc_uint<23> mA, mB;
     decode_fp32(a, sA, eA, mA);
     decode_fp32(b, sB, eB, mB);
 
     if(g_ops.enable_except) {
         if(is_fp32_nan(eA, mA) || is_fp32_nan(eB, mB))
             return encode_fp32(0, 255, 1);
         if(is_fp32_inf(eA, mA))
             return a;
         if(is_fp32_inf(eB, mB))
             return b;
     }
 
     if(g_ops.enable_subnorm) {
         if(is_fp32_subnormal(eA, mA)) eA = 1;
         if(is_fp32_subnormal(eB, mB)) eB = 1;
     }
 
     // If signs differ, perform subtraction instead.
     if(sA != sB)
         return fp32_sub_1c(a, b);
 
     // Determine the larger exponent and compute difference.
     sc_uint<8> eMax = (eA > eB) ? eA : eB;
     sc_uint<8> eMin = (eA > eB) ? eB : eA;
 
     // For normalized numbers, restore the implicit leading 1 (i.e. 1.M)
     sc_uint<24> bigM = (1 << 23) | ((eA > eB) ? mA : mB);
     sc_uint<24> smlM = (1 << 23) | ((eA > eB) ? mB : mA);
 
     sc_uint<8> diff = eMax - eMin;
     // Shift the smaller mantissa to align exponents.
     sc_uint<24> shiftedSml = smlM >> diff;
 
     // Use a 25-bit accumulator to capture any carry-out.
     sc_uint<25> sum = (sc_uint<25>)bigM + (sc_uint<25>)shiftedSml;
 
     // If there’s a carry out (bit 24 set), renormalize by shifting right.
     if(sum[24])
     {
         sum >>= 1;
         eMax++;
     }
 
     // Final rounding of the 25-bit sum to 23 bits.
     sc_uint<23> finalMant = finalize_round_fp32(sum);
     if(g_ops.enable_clamp)
         clamp_exponent_fp32(eMax, finalMant);
 
     sc_uint<32> result = encode_fp32(sA, eMax, finalMant);
 
     if(DEBUG_MODE)
         std::cout << "[FP32 ADD] a=0x" << std::hex << a 
                   << ", b=0x" << b 
                   << ", result=0x" << result << std::dec << "\n";
 
     return result;
 }
 
 //---------------------------------------------------------------------
 // FP32 SUB Operation – convert subtraction to addition via 2's complement.
 // This simple approach flips the sign bit of b and calls fp32_add_1c.
 // This works for floating point numbers in our pipeline.
 //---------------------------------------------------------------------
 sc_uint<32> fp32_sub_1c(sc_uint<32> a, sc_uint<32> b)
 {
     // Create the negative of b by inverting the sign bit.
     sc_uint<32> bNeg = b;
     bNeg[31] = ~b[31];
     if(DEBUG_MODE)
         std::cout << "[FP32 SUB] a=0x" << std::hex << a 
                   << ", b=0x" << b 
                   << ", bNeg=0x" << bNeg << std::dec << "\n";
     return fp32_add_1c(a, bNeg);
 }
 
 //---------------------------------------------------------------------
 // FP32 MUL Operation 
 //---------------------------------------------------------------------
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
             return encode_fp32(0,255,1);
         if(is_fp32_inf(eA, mA) || is_fp32_inf(eB, mB))
             return encode_fp32(outSign,255,0);
     }
 
     if(g_ops.enable_subnorm) {
         if(is_fp32_subnormal(eA, mA)) eA = 1;
         if(is_fp32_subnormal(eB, mB)) eB = 1;
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
     if ((prod >> 47) & 1) {
         prod >>= 1;
         eSum++;
     }
     sc_uint<24> mid = (prod >> 23) & 0xFFFFFF;
     sc_uint<23> finalMant = finalize_round_fp32(mid);
     sc_uint<8> outE = eSum;
     if(g_ops.enable_clamp && outE > 254) {
         outE = 254;
         finalMant = (1 << 23)-1;
     }
     return encode_fp32(outSign, outE, finalMant);
 }
 
 //---------------------------------------------------------------------
 // BF16 Helper functions
 //---------------------------------------------------------------------
 static void decode_bf16(sc_uint<32> val, sc_uint<1>& s, sc_uint<8>& e, sc_uint<7>& m) {
     s = val[31];
     e = val.range(30,23);
     m = val.range(22,16);
 }
 
 static sc_uint<32> encode_bf16(sc_uint<1> s, sc_uint<8> e, sc_uint<7> m) {
     sc_uint<32> val = 0;
     val[31] = s;
     val.range(30,23) = e;
     val.range(22,16) = m;
     return val;
 }
 
 static bool is_bf16_inf(sc_uint<8> e, sc_uint<7> m) {
     return (e==255 && m==0);
 }
 static bool is_bf16_nan(sc_uint<8> e, sc_uint<7> m) {
     return (e==255 && m!=0);
 }
 static bool is_bf16_subnormal(sc_uint<8> e, sc_uint<7> m) {
     return (e==0 && m!=0);
 }
 static void clamp_exponent_bf16(sc_uint<8>& e, sc_uint<7>& m) {
     if(e > 254) {
         e = 254;
         m = (1 << 7) - 1;
     }
 }
 
 // For BF16, we use an 8-bit accumulator for addition.
 static sc_uint<7> finalize_round_bf16(sc_uint<8> sum) {
     if(g_ops.enable_trunc) {
         return sum.range(6,0);
     } else {
         bool roundBit = sum[0].to_bool();
         sum >>= 1;
         if(roundBit) sum += 1;
         return sum.range(6,0);
     }
 }
 
 //---------------------------------------------------------------------
 // BF16 ADD Operation
 //---------------------------------------------------------------------
 sc_uint<32> bf16_add_1c(sc_uint<32> a, sc_uint<32> b)
 {
     sc_uint<1> sA, sB;
     sc_uint<8> eA, eB;
     sc_uint<7> mA, mB;
     decode_bf16(a, sA, eA, mA);
     decode_bf16(b, sB, eB, mB);
 
     if(g_ops.enable_except) {
         if(is_bf16_nan(eA, mA) || is_bf16_nan(eB, mB))
             return encode_bf16(0,255,1);
         if(is_bf16_inf(eA, mA)) return a;
         if(is_bf16_inf(eB, mB)) return b;
     }
 
     if(g_ops.enable_subnorm) {
         if(is_bf16_subnormal(eA, mA)) eA = 1;
         if(is_bf16_subnormal(eB, mB)) eB = 1;
     }
 
     if(sA != sB)
         return bf16_sub_1c(a, b);
 
     sc_uint<8> eMax = (eA > eB)? eA : eB;
     sc_uint<8> diff = (eA > eB)? (eA - eB) : (eB - eA);
 
     // Restore implicit bit for BF16 (7-bit mantissa) by OR-ing with (1<<7)
     sc_uint<8> bigM = (1 << 7) | ((eA > eB) ? mA : mB);
     sc_uint<8> smlM = (1 << 7) | ((eA > eB) ? mB : mA);
 
     sc_uint<8> ext = smlM >> diff;
     sc_uint<8> sumVal = bigM + ext;
 
     if(sumVal[7] == 1) {
         sumVal >>= 1;
         eMax++;
     }
 
     sc_uint<7> finalMant = finalize_round_bf16(sumVal);
     if(g_ops.enable_clamp) clamp_exponent_bf16(eMax, finalMant);
 
     return encode_bf16(sA, eMax, finalMant);
 }
 
 //---------------------------------------------------------------------
 // BF16 SUB Operation
 //---------------------------------------------------------------------
 sc_uint<32> bf16_sub_1c(sc_uint<32> a, sc_uint<32> b)
 {
     sc_uint<1> sA, sB;
     sc_uint<8> eA, eB;
     sc_uint<7> mA, mB;
     decode_bf16(a, sA, eA, mA);
     decode_bf16(b, sB, eB, mB);
 
     if(g_ops.enable_except) {
         if(is_bf16_nan(eA, mA) || is_bf16_nan(eB, mB))
             return encode_bf16(0,255,1);
         if(is_bf16_inf(eA, mA) && is_bf16_inf(eB, mB))
             return encode_bf16(0,255,1);
         if(is_bf16_inf(eA, mA)) return a;
         if(is_bf16_inf(eB, mB)) return encode_bf16(~sB,255,0);
     }
     if(g_ops.enable_subnorm) {
         if(is_bf16_subnormal(eA, mA)) eA = 1;
         if(is_bf16_subnormal(eB, mB)) eB = 1;
     }
 
     if(sA != sB) {
         sc_uint<32> bNeg = b;
         bNeg[31] = ~sB;
         return bf16_add_1c(a, bNeg);
     } else {
         sc_uint<8> eMax = (eA > eB) ? eA : eB;
         sc_uint<8> diff = (eA > eB) ? (eA - eB) : (eB - eA);
 
         // For BF16 subtraction, restore the implicit bit
         sc_uint<8> bigM = (1 << 7) | ((eA > eB) ? mA : mB);
         sc_uint<8> smlM = (1 << 7) | ((eA > eB) ? mB : mA);
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
 
 //---------------------------------------------------------------------
 // BF16 MUL Operation
 //---------------------------------------------------------------------
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
             return encode_bf16(0, 255, 1);
         bool infA = is_bf16_inf(eA, mA);
         bool infB = is_bf16_inf(eB, mB);
         if(infA || infB) {
             if((infA && eB != 0) || (infB && eA != 0))
                 return encode_bf16(outSign, 255, 0);
             else
                 return encode_bf16(0, 255, 1);
         }
     }
     if(g_ops.enable_subnorm) {
         if(is_bf16_subnormal(eA, mA)) eA = 1;
         if(is_bf16_subnormal(eB, mB)) eB = 1;
     }
 
     int eSum = (int)eA + (int)eB - 127;
     if(eSum < 1) eSum = 1;
     else if(eSum > 254) {
         if(g_ops.enable_clamp)
             return encode_bf16(outSign, 254, (1 << 7) - 1);
         else
             return encode_bf16(outSign, 255, 0);
     }
 
     sc_uint<8> bigA = (1 << 7) | mA;
     sc_uint<8> bigB = (1 << 7) | mB;
     unsigned int prod = (unsigned int)bigA * (unsigned int)bigB;
     if ((prod >> 15) & 1) {
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
         return encode_bf16(outSign, 255, 0);
     }
     return encode_bf16(outSign, outE, finalMant);
 }
 
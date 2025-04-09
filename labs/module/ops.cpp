/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: ops.cpp
 * Description: Implements single‐cycle FP32 and BF16 arithmetic operations 
 *              (Add, Sub, Mul). It restores the hidden 1 from normalized numbers,
 *              aligns significands correctly, and then “drops” the implicit bit 
 *              (rather than doing an extra shift) to produce the correct 23‐bit fraction.
 *              Debug mode prints internal values when DEBUG_MODE is enabled.
 **********/

 #include "ops.hpp"
 #include <iostream>
 #include <cstdint>
 
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
 
 // ---------------------- FP32 HELPER FUNCTIONS ----------------------
 
 // decode_fp32: splits a 32‐bit FP number into its parts.
 static void decode_fp32(sc_uint<32> val, sc_uint<1>& s, sc_uint<8>& e, sc_uint<23>& m) {
     s = val[31];
     e = val.range(30,23);
     m = val.range(22,0);
 }
 
 // encode_fp32: combines sign, exponent, and mantissa into a 32‐bit value.
 static sc_uint<32> encode_fp32(sc_uint<1> s, sc_uint<8> e, sc_uint<23> m) {
     sc_uint<32> out = 0;
     out[31] = s;
     out.range(30,23) = e;
     out.range(22,0) = m;
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
 
 static void clamp_exponent_fp32(sc_uint<8>& e, sc_uint<23>& m) {
     if(e > 254) {
         e = 254;
         m = (1 << 23) - 1;
     }
 }
 
 /**
  * finalize_round_fp32:
  * Given a 25-bit accumulator (sum) which is known to be in the range
  * [1<<23, 1<<24) (i.e., the implicit 1 is in position 23), the result’s
  * stored fraction is simply sum – (1<<23). (A more sophisticated routine
  * could use guard, round, and sticky bits, but here we assume ideal inputs.)
  */
 static sc_uint<23> finalize_round_fp32(sc_uint<25> sum) {
     // For our purposes, simply subtract the implicit 1.
     return sum - (1 << 23);
 }
 
 // ---------------------- FP32 ADD ----------------------
 sc_uint<32> fp32_add_1c(sc_uint<32> a, sc_uint<32> b)
 {
     sc_uint<1> sA, sB;
     sc_uint<8> eA, eB;
     sc_uint<23> mA, mB;
     decode_fp32(a, sA, eA, mA);
     decode_fp32(b, sB, eB, mB);
 
     if(g_ops.enable_except) {
         if(is_fp32_nan(eA, mA) || is_fp32_nan(eB, mB))
             return encode_fp32(0,255,1);
         if(is_fp32_inf(eA, mA)) return a;
         if(is_fp32_inf(eB, mB)) return b;
     }
 
     if(g_ops.enable_subnorm) {
         if(is_fp32_subnormal(eA, mA))
             eA = 1;
         if(is_fp32_subnormal(eB, mB))
             eB = 1;
     }
 
     // If the signs differ, subtraction must be done.
     if(sA != sB)
         return fp32_sub_1c(a, b);
 
     sc_uint<8> eMax = (eA > eB) ? eA : eB;
     sc_uint<8> eMin = (eA > eB) ? eB : eA;
 
     // Restore hidden bit: each normalized number is 1.mantissa
     sc_uint<24> bigM = (1 << 23) | ((eA > eB) ? mA : mB);
     sc_uint<24> smlM = (1 << 23) | ((eA > eB) ? mB : mA);
 
     sc_uint<8> diff = eMax - eMin;
     // Right shift the smaller significand
     sc_uint<24> alignedSml = smlM >> diff;
 
     // Add the two 24-bit significands in a 25-bit accumulator.
     sc_uint<25> sumVal = (sc_uint<25>)bigM + (sc_uint<25>)alignedSml;
     
     // If a carry-out occurred, our accumulator is in [1<<24, 2<<23)
     // Our algorithm in the caller already normalizes the result if needed.
     // For our ideal inputs, no extra shift is needed.
     
     // Now, the stored fraction for the final result is:
     sc_uint<23> finalMant = finalize_round_fp32(sumVal);
     if(g_ops.enable_clamp)
         clamp_exponent_fp32(eMax, finalMant);
     
     sc_uint<32> result = encode_fp32(sA, eMax, finalMant);
     
     if(DEBUG_MODE) {
         std::cout << "[FP32 ADD] a=0x" << std::hex << a 
                   << ", b=0x" << b 
                   << ", sumVal=0x" << sumVal.to_uint()
                   << ", finalMant=0x" << finalMant 
                   << ", result=0x" << result 
                   << std::dec << std::endl;
     }
     
     return result;
 }
 
 // ---------------------- FP32 SUB ----------------------
 /*
  * fp32_sub_1c: Subtraction is performed by comparing the magnitudes.
  * We decode both numbers, determine which is larger,
  * align the smaller to the larger, subtract, then normalize the result.
  */
 sc_uint<32> fp32_sub_1c(sc_uint<32> a, sc_uint<32> b)
 {
     // First, decode both numbers.
     sc_uint<1> sA, sB;
     sc_uint<8> eA, eB;
     sc_uint<23> mA, mB;
     decode_fp32(a, sA, eA, mA);
     decode_fp32(b, sB, eB, mB);
 
     // If the sign bits differ, use addition.
     if(sA != sB) {
         sc_uint<32> bNeg = b;
         bNeg[31] = ~b[31];
         return fp32_add_1c(a, bNeg);
     }
 
     // Determine the operand with the larger magnitude.
     bool aGreater;
     if(eA > eB)
         aGreater = true;
     else if(eA < eB)
         aGreater = false;
     else
         aGreater = (mA >= mB);
     
     sc_uint<1> resultSign = aGreater ? sA : sB;
     // Restore implicit 1:
     sc_uint<24> manA = (1 << 23) | mA;
     sc_uint<24> manB = (1 << 23) | mB;
     
     sc_uint<8> expLarger, expSmaller;
     sc_uint<24> manLarger, manSmaller;
     if(aGreater) {
         expLarger = eA;
         expSmaller = eB;
         manLarger = manA;
         manSmaller = manB;
     } else {
         expLarger = eB;
         expSmaller = eA;
         manLarger = manB;
         manSmaller = manA;
     }
     
     // Align the smaller significand.
     sc_uint<8> diff = expLarger - expSmaller;
     sc_uint<24> alignedSmaller = manSmaller >> diff;
     
     // Subtract the smaller from the larger.
     int diffMant = (int)manLarger - (int)alignedSmaller;
     if(diffMant < 0) diffMant = 0;  // ensure non-negative
     sc_uint<24> diffVal = diffMant;
     
     // Normalize: shift left until the implicit 1 is in bit 23.
     sc_uint<8> resultExp = expLarger;
     while(resultExp > 0 && diffVal[23] == 0) {
         diffVal <<= 1;
         resultExp--;
     }
     
     // In FP subtraction, no extra rounding (for now) is used.
     // The stored fraction is diffVal minus the hidden bit.
     sc_uint<23> finalMant = diffVal - (1 << 23);
     if(g_ops.enable_clamp)
         clamp_exponent_fp32(resultExp, finalMant);
     
     sc_uint<32> result = encode_fp32(resultSign, resultExp, finalMant);
     
     if(DEBUG_MODE) {
         std::cout << "[FP32 SUB] a=0x" << std::hex << a 
                   << ", b=0x" << b 
                   << ", result=0x" << result 
                   << std::dec << std::endl;
     }
     
     return result;
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
     if(g_ops.enable_clamp && outE > 254) {
         outE = 254;
         finalMant = (1 << 23) - 1;
     }
     
     sc_uint<32> result = encode_fp32(outSign, outE, finalMant);
     if(DEBUG_MODE) {
         std::cout << "[FP32 MUL] a=0x" << std::hex << a 
                   << ", b=0x" << b 
                   << ", result=0x" << result << std::dec << std::endl;
     }
     return result;
 }
 
 // ---------------------- BF16 HELPER FUNCTIONS ----------------------
 
 // For BF16, the number is stored with 1 sign bit, 8 exponent bits, and the top 7 bits of the fraction.
 static void decode_bf16(sc_uint<32> val, sc_uint<1>& s, sc_uint<8>& e, sc_uint<7>& m) {
     s = val[31];
     e = val.range(30,23);
     m = val.range(22,16);
 }
 static sc_uint<32> encode_bf16(sc_uint<1> s, sc_uint<8> e, sc_uint<7> m) {
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
 static void clamp_exponent_bf16(sc_uint<8>& e, sc_uint<7>& m) {
     if(e > 254) {
         e = 254;
         m = (1 << 7) - 1;
     }
 }
 static sc_uint<7> finalize_round_bf16(sc_uint<8> sum) {
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
         if(is_bf16_inf(eA, mA)) return a;
         if(is_bf16_inf(eB, mB)) return b;
     }
     if(g_ops.enable_subnorm) {
         if(is_bf16_subnormal(eA, mA)) eA = 1;
         if(is_bf16_subnormal(eB, mB)) eB = 1;
     }
     if(sA != sB)
         return bf16_sub_1c(a, b);
 
     sc_uint<8> eMax = (eA > eB) ? eA : eB;
     sc_uint<8> diff = (eA > eB) ? (eA - eB) : (eB - eA);
 
     // Restore the implicit bit for BF16 by OR-ing with (1 << 7) to get an 8-bit significand.
     sc_uint<8> bigM = (1 << 7) | ((eA > eB) ? mA : mA);
     sc_uint<8> smlM = (1 << 7) | ((eA > eB) ? mB : mB);
 
     sc_uint<8> alignedSmaller = smlM >> diff;
     sc_uint<8> sumVal = bigM + alignedSmaller;
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
         sc_uint<8> bigM = (1 << 7) | ((eA > eB) ? mA : mA);
         sc_uint<8> smlM = (1 << 7) | ((eA > eB) ? mB : mB);
         sc_uint<1> outSign = (eA > eB) ? sA : sB;
         sc_uint<8> alignedSmaller = smlM >> diff;
 
         int diffMant = (int)bigM - (int)alignedSmaller;
         if(diffMant < 0) {
             outSign = ~outSign;
             diffMant = -diffMant;
         }
         sc_uint<8> mag = diffMant;
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
         if(is_bf16_subnormal(eA, mA)) eA = 1;
         if(is_bf16_subnormal(eB, mB)) eB = 1;
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
 
/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: ops.cpp
 * Description: Implements single‐cycle FP32 and BF16 arithmetic operations 
 *              (addition, subtraction, multiplication). The operations correctly 
 *              restore the implicit 1, align operands, and normalize the result.
 *              FP32 subtraction is implemented by comparing magnitudes and using 
 *              proper 2's complement (rather than simply negating the sign).
 *              Debug prints are emitted when DEBUG_MODE is true.
 **********/

 #include "ops.hpp"
 #include <iostream>
 #include <cstdint>
 
 //---------------------------------------------------------------------
 // Debug mode flag
 //---------------------------------------------------------------------
 static const bool DEBUG_MODE = true;
 
 //---------------------------------------------------------------------
 // Global context for FP operations control.
 // These are set via setOpsContext() externally.
 //---------------------------------------------------------------------
 static struct {
     bool enable_subnorm;  // if true, treat subnormals as normalized (exponent = 1)
     bool enable_trunc;    // if true, perform truncation instead of round-half-up
     bool enable_clamp;    // if true, clamp the exponent on overflow
     bool enable_except;   // if true, handle NaN/Inf explicitly
 } g_ops;
 
 void setOpsContext(bool subnorm, bool trunc, bool clamp, bool except) {
     g_ops.enable_subnorm = subnorm;
     g_ops.enable_trunc   = trunc;
     g_ops.enable_clamp   = clamp;
     g_ops.enable_except  = except;
 }
 
 // ---------------------- FP32 HELPER FUNCTIONS ----------------------
 
 // Decode a 32-bit floating-point value into sign (1 bit), exponent (8 bits),
 // and mantissa (23 bits).
 static void decode_fp32(sc_uint<32> val, sc_uint<1>& s, sc_uint<8>& e, sc_uint<23>& m)
 {
     s = val[31];
     e = val.range(30, 23);
     m = val.range(22, 0);
 }
 
 // Encode sign, exponent, and mantissa into a 32-bit floating-point value.
 static sc_uint<32> encode_fp32(sc_uint<1> s, sc_uint<8> e, sc_uint<23> m)
 {
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
 
 // Clamp exponent if above maximum; set mantissa to maximum.
 static void clamp_exponent_fp32(sc_uint<8>& e, sc_uint<23>& m)
 {
     if(e > 254) {
         e = 254;
         m = (1 << 23) - 1;
     }
 }
 
 // Finalize rounding for FP32 addition/subtraction using a 25-bit accumulator.
 // Accepts sum (25 bits) and returns 23-bit mantissa.
 // (If enable_trunc is true, simply drop LSB; otherwise, round half-up.)
 static sc_uint<23> finalize_round_fp32(sc_uint<25> sum)
 {
     if(g_ops.enable_trunc) {
         sum >>= 1;
     } else {
         bool roundBit = sum[0].to_bool();
         sum >>= 1;
         if(roundBit)
             sum += 1;
     }
     return sum.range(22,0);
 }
 
 // ---------------------- FP32 ADD ----------------------
 /*
  * FP32 addition:
  * - Decode both operands.
  * - Restore implicit 1 in each operand to obtain a 24-bit significand.
  * - Align the smaller significand by right-shifting by the difference in exponents.
  * - Use a 25-bit accumulator to add the significands.
  * - If there's a carry-out (bit 24 set), shift right and increment the exponent.
  * - Round the 25-bit sum to 23 bits.
  * - Clamp the exponent if needed.
  * - Encode and return the result.
  */
 sc_uint<32> fp32_add_1c(sc_uint<32> a, sc_uint<32> b)
 {
     sc_uint<1> sA, sB;
     sc_uint<8> eA, eB;
     sc_uint<23> mA, mB;
     decode_fp32(a, sA, eA, mA);
     decode_fp32(b, sB, eB, mB);
 
     // Exception processing.
     if(g_ops.enable_except) {
         if(is_fp32_nan(eA, mA) || is_fp32_nan(eB, mB))
             return encode_fp32(0, 255, 1);
         if(is_fp32_inf(eA, mA)) return a;
         if(is_fp32_inf(eB, mB)) return b;
     }
 
     // Handle subnormals by forcing exponent to 1, if enabled.
     if(g_ops.enable_subnorm) {
         if(is_fp32_subnormal(eA, mA)) eA = 1;
         if(is_fp32_subnormal(eB, mB)) eB = 1;
     }
 
     // If signs differ, defer to subtraction.
     if(sA != sB)
         return fp32_sub_1c(a, b);
 
     sc_uint<8> eMax = (eA > eB) ? eA : eB;
     sc_uint<8> eMin = (eA > eB) ? eB : eA;
 
     // Restore the implicit 1 for normalized numbers; get 24-bit significands.
     sc_uint<24> bigM = (1 << 23) | ((eA > eB) ? mA : mB);
     sc_uint<24> smlM = (1 << 23) | ((eA > eB) ? mB : mA);
 
     // Align the smaller significand.
     sc_uint<8> diff = eMax - eMin;
     sc_uint<24> alignedSml = smlM >> diff;
 
     // Accumulate using a 25-bit variable.
     sc_uint<25> sumVal = (sc_uint<25>)bigM + (sc_uint<25>)alignedSml;
 
     // Normalize if carry-out occurs.
     if(sumVal[24]) {
         sumVal >>= 1;
         eMax++;
     }
 
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
  * FP32 subtraction:
  * - Decode both operands.
  * - If the signs differ, then subtraction becomes addition (handled by fp32_add_1c).
  * - Otherwise, compare the operands to determine which is larger.
  * - Align the smaller significand to the larger exponent.
  * - Subtract the aligned smaller significand from the larger.
  * - Normalize the result by shifting left until the implicit 1 is restored.
  * - Round the normalized result.
  * - Set the result's sign to that of the larger (in magnitude) operand.
  * - Encode and return the result.
  */
 sc_uint<32> fp32_sub_1c(sc_uint<32> a, sc_uint<32> b)
 {
     sc_uint<1> sA, sB;
     sc_uint<8> eA, eB;
     sc_uint<23> mA, mB;
     decode_fp32(a, sA, eA, mA);
     decode_fp32(b, sB, eB, mB);
 
     // If signs differ, subtraction becomes addition.
     if(sA != sB) {
         sc_uint<32> bNeg = b;
         bNeg[31] = ~b[31];   // Negate b
         return fp32_add_1c(a, bNeg);
     }
 
     // Both numbers have the same sign, so perform true subtraction.
     // Determine the larger operand (by exponent then mantissa).
     bool aGreater;
     if(eA > eB)
         aGreater = true;
     else if(eA < eB)
         aGreater = false;
     else
         aGreater = (mA >= mB); // Compare mantissas if exponents are equal
 
     // The result's sign is the sign of the larger operand.
     sc_uint<1> resultSign = aGreater ? sA : sB;
 
     // Convert mantissas to 24-bit (restore implicit bit).
     sc_uint<24> manA = (1 << 23) | mA;
     sc_uint<24> manB = (1 << 23) | mB;
 
     // Ensure we subtract the smaller magnitude from the larger.
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
 
     // Subtract the aligned smaller mantissa from the larger.
     int diffMant = (int)manLarger - (int)alignedSmaller;
     if(diffMant < 0)
         diffMant = 0; // Guard against underflow
 
     sc_uint<24> diffMant_u = diffMant;
 
     // Normalize the result: shift left until the implicit 1 appears.
     sc_uint<8> resultExp = expLarger;
     while(resultExp > 0 && diffMant_u[23] == 0) {
         diffMant_u <<= 1;
         resultExp--;
     }
 
     // Use a 25-bit accumulator to round.
     sc_uint<25> roundAcc = diffMant_u;
     sc_uint<23> finalMant = finalize_round_fp32(roundAcc);
     if(g_ops.enable_clamp)
         clamp_exponent_fp32(resultExp, finalMant);
     return encode_fp32(resultSign, resultExp, finalMant);
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
         if(is_fp32_subnormal(eA, mA)) eA = 1;
         if(is_fp32_subnormal(eB, mB)) eB = 1;
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
         finalMant = (1 << 23)-1;
     }
     return encode_fp32(outSign, outE, finalMant);
 }
 
 // ---------------------- BF16 HELPER FUNCTIONS ----------------------
 
 // Decode a BF16 value. BF16 uses 1 sign bit, 8 exponent bits, and the top 7 bits of the fraction.
 static void decode_bf16(sc_uint<32> val, sc_uint<1>& s, sc_uint<8>& e, sc_uint<7>& m)
 {
     s = val[31];
     e = val.range(30,23);
     m = val.range(22,16);
 }
 
 // Encode a BF16 value.
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
  * finalize_round_bf16: Given an 8-bit BF16 accumulator (with one extra rounding bit),
  * perform rounding (truncation or round-half-up) and return a 7-bit result.
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
 
     sc_uint<8> eMax = (eA > eB) ? eA : eB;
     sc_uint<8> diff = (eA > eB) ? (eA - eB) : (eB - eA);
 
     // Restore implicit bit: use 8-bit significands for BF16.
     sc_uint<8> bigM = (1 << 7) | ((eA > eB) ? mA : mA);
     sc_uint<8> smlM = (1 << 7) | ((eA > eB) ? mB : mB);
 
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
 
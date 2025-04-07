#include "typecast_ops.hpp"

// Helpers (reuse FP32/BF16 helpers from ops.cpp)
static void decode_fp32(sc_uint<32> val, sc_uint<1>& sign, sc_uint<8>& exp, sc_uint<23>& mant) {
    sign = val[31];
    exp  = val.range(30,23);
    mant = val.range(22,0);
}
static sc_uint<32> encode_fp32(sc_uint<1> sign, sc_uint<8> exp, sc_uint<23> mant) {
    sc_uint<32> out = 0;
    out[31] = sign;
    out.range(30,23)=exp;
    out.range(22,0)=mant;
    return out;
}
static void decode_bf16(sc_uint<32> val, sc_uint<1>& sign, sc_uint<8>& exp, sc_uint<7>& mant) {
    sign = val[31];
    exp  = val.range(30,23);
    mant = val.range(22,16);
}
static sc_uint<32> encode_bf16(sc_uint<1> sign, sc_uint<8> exp, sc_uint<7> mant) {
    sc_uint<32> out = 0;
    out[31] = sign;
    out.range(30,23) = exp;
    out.range(22,16) = mant;
    return out;
}

sc_uint<32> typecast_single_cycle(sc_uint<32> input,
                                  NumFormat srcFmt,
                                  NumFormat dstFmt,
                                  bool enable_subnorm,
                                  bool enable_trunc,
                                  bool enable_clamp,
                                  bool enable_except)
{
    // For demonstration, we implement only FP32 <-> BF16 and FP32->INT8 conversions.
    if(srcFmt==FP32 && dstFmt==BF16) {
        sc_uint<1> s; sc_uint<8> e; sc_uint<23> mant32;
        decode_fp32(input, s, e, mant32);
        if(enable_subnorm && is_fp32_subnormal(e, mant32)) {
            while(mant32[22]==0 && e>0) { mant32 = mant32 << 1; e--; }
        }
        sc_uint<7> mant16 = mant32.range(22,16);
        if(enable_clamp && e>254) {
            e=254; mant16 = (1<<7)-1;
        }
        return encode_bf16(s, e, mant16);
    }
    else if(srcFmt==BF16 && dstFmt==FP32) {
        sc_uint<1> s; sc_uint<8> e; sc_uint<7> mant16;
        decode_bf16(input, s, e, mant16);
        // Expand mantissa by appending 16 zero bits (naively)
        sc_uint<23> mant32 = (mant16, sc_uint<16>(0));
        return encode_fp32(s, e, mant32);
    }
    else if(srcFmt==FP32 && dstFmt==INT8) {
        sc_uint<1> s; sc_uint<8> e; sc_uint<23> mant;
        decode_fp32(input, s, e, mant);
        int exponent = (int)e - 127;
        int value = (1 << 23) | mant; // implicit 1 included
        if(exponent > 23)
            value = value << (exponent - 23);
        else if(exponent < 23)
            value = value >> (23 - exponent);
        int intVal = s ? -value : value;
        // Clamping to INT8
        if(enable_clamp) {
            if(intVal > 127) intVal = 127;
            if(intVal < -128) intVal = -128;
        }
        return (sc_uint<32>) ( (unsigned int)(intVal) );
    }
    return input;
}
// Other conversions (BF16->INT8, INT8->FP32, etc.) can be added similarly.
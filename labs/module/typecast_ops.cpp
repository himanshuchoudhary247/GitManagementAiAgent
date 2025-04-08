/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: typecast_ops.cpp
 * Description: Implements the single-cycle typecast operation, referencing the same
 *              global context from ops if needed. Supports partial FP32<->BF16<->INT8.
 **********/
#include "typecast_ops.hpp"
#include "ops.hpp"

// Local decode/encode helpers for demonstration
static void decode_fp32_local(sc_uint<32> val,
                              sc_uint<1>& s,
                              sc_uint<8>& e,
                              sc_uint<23>& m)
{
    s = val[31];
    e = val.range(30,23);
    m = val.range(22,0);
}
static sc_uint<32> encode_fp32_local(sc_uint<1> s,
                                     sc_uint<8> e,
                                     sc_uint<23> m)
{
    sc_uint<32> out=0;
    out[31]=s;
    out.range(30,23)=e;
    out.range(22,0)=m;
    return out;
}
static void decode_bf16_local(sc_uint<32> val,
                              sc_uint<1>& s,
                              sc_uint<8>& e,
                              sc_uint<7>& m)
{
    s= val[31];
    e= val.range(30,23);
    m= val.range(22,16);
}
static sc_uint<32> encode_bf16_local(sc_uint<1> s,
                                     sc_uint<8> e,
                                     sc_uint<7> m)
{
    sc_uint<32> out=0;
    out[31]=s;
    out.range(30,23)=e;
    out.range(22,16)=m;
    return out;
}

/**
 * typecast_single_cycle:
 *   Takes a 32-bit input, interprets it in srcFmt, then
 *   converts to dstFmt in one shot. 
 */
sc_uint<32> typecast_single_cycle(sc_uint<32> input,
                                  NumFormat srcFmt,
                                  NumFormat dstFmt)
{
    if(srcFmt==FP32 && dstFmt==BF16) {
        sc_uint<1> s; sc_uint<8> e; sc_uint<23> m;
        decode_fp32_local(input,s,e,m);
        sc_uint<7> outM = m.range(22,16);
        return encode_bf16_local(s,e,outM);
    } 
    else if(srcFmt==BF16 && dstFmt==FP32) {
        sc_uint<1> s; sc_uint<8> e; sc_uint<7> mm;
        decode_bf16_local(input,s,e,mm);
        sc_uint<23> outM = (mm, sc_uint<16>(0));
        return encode_fp32_local(s,e,outM);
    } 
    else if(srcFmt==FP32 && dstFmt==INT8) {
        sc_uint<1> s; sc_uint<8> e; sc_uint<23> m;
        decode_fp32_local(input,s,e,m);
        int exponent = (int)e - 127;
        int value = (1<<23) | m;
        if(exponent>23)
            value <<= (exponent-23);
        else if(exponent<23)
            value >>= (23-exponent);
        int intVal = (s? -value: value);
        // clamp
        if(intVal>127) intVal=127;
        if(intVal<-128) intVal=-128;
        return (sc_uint<32>)((unsigned int)intVal);
    }
    // default pass
    return input;
}

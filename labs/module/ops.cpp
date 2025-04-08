/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: ops.cpp
 * Description: Implements single-cycle FP32 and BF16 add, sub, mul, referencing
 *              a global context for subnormal/trunc/clamp/exception logic.
 **********/
#include "ops.hpp"

//-------------------------------------------------------------------
// Global struct storing the subnorm/trunc/clamp/except settings.
// This is updated by setOpsContext(...) from outside code.
//-------------------------------------------------------------------
static struct {
    bool enable_subnorm;
    bool enable_trunc;
    bool enable_clamp;
    bool enable_except;
} g_ops;

//-------------------------------------------------------------------
// setOpsContext: updates the global ops context
//-------------------------------------------------------------------
void setOpsContext(bool enableSubnorm,
                   bool enableTrunc,
                   bool enableClamp,
                   bool enableExcept)
{
    g_ops.enable_subnorm = enableSubnorm;
    g_ops.enable_trunc   = enableTrunc;
    g_ops.enable_clamp   = enableClamp;
    g_ops.enable_except  = enableExcept;
}

//================== FP32 HELPERS ==================
// Each function is carefully commented for clarity.

/**
 * decode_fp32: splits a 32-bit input into sign, exponent, mantissa
 */
static void decode_fp32(sc_uint<32> val,
                        sc_uint<1>& sign,
                        sc_uint<8>& exp,
                        sc_uint<23>& mant)
{
    sign = val[31];
    exp  = val.range(30,23);
    mant = val.range(22,0);
}

/**
 * encode_fp32: merges sign, exponent, mantissa into a 32-bit output
 */
static sc_uint<32> encode_fp32(sc_uint<1> sign,
                               sc_uint<8> exp,
                               sc_uint<23> mant)
{
    sc_uint<32> out=0;
    out[31] = sign;
    out.range(30,23) = exp;
    out.range(22,0)  = mant;
    return out;
}

// Checking for Infinity or NaN
static bool is_fp32_inf(sc_uint<8> e, sc_uint<23> m) {
    return (e==255 && m==0);
}
static bool is_fp32_nan(sc_uint<8> e, sc_uint<23> m) {
    return (e==255 && m!=0);
}
static bool is_fp32_subnormal(sc_uint<8> e, sc_uint<23> m) {
    return (e==0 && m!=0);
}

/**
 * clamp_exponent_fp32: if exponent > 254, set exponent=254, mantissa=all-1
 */
static void clamp_exponent_fp32(sc_uint<8>& exp, sc_uint<23>& mant)
{
    if(exp>254) {
        exp=254;
        mant=(1<<23)-1;
    }
}

/**
 * finalize_round_fp32: handles trunc vs round-half-up, with 24-bit sum
 * input (the 24th bit is for rounding), returning the final 23 bits.
 */
static sc_uint<23> finalize_round_fp32(sc_uint<24> sum)
{
    if(g_ops.enable_trunc) {
        // drop the LSB, no half-up
        sum = sum >> 1;
    } else {
        // round half-up
        sc_uint<1> roundBit= sum[0];
        sum = sum >> 1;
        if(roundBit==1)
            sum = sum + 1;
    }
    return sum.range(22,0);
}

//================== BF16 HELPERS ==================
static void decode_bf16(sc_uint<32> val,
                        sc_uint<1>& sign,
                        sc_uint<8>& exp,
                        sc_uint<7>& mant)
{
    sign = val[31];
    exp  = val.range(30,23);
    mant = val.range(22,16);
}
static sc_uint<32> encode_bf16(sc_uint<1> sign,
                               sc_uint<8> exp,
                               sc_uint<7> mant)
{
    sc_uint<32> out=0;
    out[31]=sign;
    out.range(30,23)=exp;
    out.range(22,16)=mant;
    return out;
}
static bool is_bf16_inf(sc_uint<8> e, sc_uint<7> m)
{
    return (e==255 && m==0);
}
static bool is_bf16_nan(sc_uint<8> e, sc_uint<7> m)
{
    return (e==255 && m!=0);
}
static bool is_bf16_subnormal(sc_uint<8> e, sc_uint<7> m)
{
    return (e==0 && m!=0);
}

/**
 * clamp_exponent_bf16: if exponent > 254, set exponent=254, mantissa=all-1
 */
static void clamp_exponent_bf16(sc_uint<8>& exp, sc_uint<7>& mant)
{
    if(exp>254) {
        exp=254;
        mant=(1<<7)-1;
    }
}

/**
 * finalize_round_bf16: handles trunc vs round-half-up for BF16.
 * sum8 has 1 extra bit for rounding in position 0.
 */
static sc_uint<7> finalize_round_bf16(sc_uint<8> sum8)
{
    if(g_ops.enable_trunc) {
        // skip half-up
    } else {
        sc_uint<1> roundBit = sum8[0];
        sum8 = sum8 >> 1;
        if(roundBit==1)
            sum8 = sum8 + 1;
        return sum8.range(6,0);
    }
    return sum8.range(6,0);
}

//-------------------------------------------------------------------
// FP32 ADD
//-------------------------------------------------------------------
sc_uint<32> fp32_add_1c(sc_uint<32> a, sc_uint<32> b)
{
    sc_uint<1> sA,sB;
    sc_uint<8> eA,eB;
    sc_uint<23> mA,mB;
    decode_fp32(a,sA,eA,mA);
    decode_fp32(b,sB,eB,mB);

    // If exceptions are enabled, handle Inf/NaN carefully
    if(g_ops.enable_except) {
        if(is_fp32_nan(eA,mA) || is_fp32_nan(eB,mB))
            return encode_fp32(0,255,1);
        if(is_fp32_inf(eA,mA) && is_fp32_inf(eB,mB)) {
            if(sA==sB) return encode_fp32(sA,255,0);
            else       return encode_fp32(0,255,1);
        }
        if(is_fp32_inf(eA,mA)) return a;
        if(is_fp32_inf(eB,mB)) return b;
    }

    // subnormal check
    if(g_ops.enable_subnorm) {
        if(is_fp32_subnormal(eA,mA)) eA=1;
        if(is_fp32_subnormal(eB,mB)) eB=1;
    }

    // If signs differ => do sub
    if(sA != sB) {
        return fp32_sub_1c(a,b);
    }

    // same sign => straightforward add
    sc_uint<8> eMax=(eA>eB)? eA:eB;
    sc_uint<8> eMin=(eA>eB)? eB:eA;
    sc_uint<23> bigM=(eA>eB)? mA:mB;
    sc_uint<23> smlM=(eA>eB)? mB:mA;
    sc_uint<8> diff = eMax - eMin;
    sc_uint<24> extSml = (sc_uint<24>)smlM;
    extSml = extSml >> diff;
    sc_uint<24> sum = (sc_uint<24>)bigM + extSml;
    if(sum[23]==1) {
        sum = sum >> 1;
        eMax++;
    }
    sc_uint<23> finalMant = finalize_round_fp32(sum);
    if(g_ops.enable_clamp) clamp_exponent_fp32(eMax, finalMant);
    return encode_fp32(sA, eMax, finalMant);
}

//-------------------------------------------------------------------
// FP32 SUB
//-------------------------------------------------------------------
sc_uint<32> fp32_sub_1c(sc_uint<32> a, sc_uint<32> b)
{
    sc_uint<1> sA,sB;
    sc_uint<8> eA,eB;
    sc_uint<23> mA,mB;
    decode_fp32(a,sA,eA,mA);
    decode_fp32(b,sB,eB,mB);

    if(g_ops.enable_except) {
        if(is_fp32_nan(eA,mA)|| is_fp32_nan(eB,mB))
            return encode_fp32(0,255,1);
        if(is_fp32_inf(eA,mA)&& is_fp32_inf(eB,mB))
            return encode_fp32(0,255,1);
        if(is_fp32_inf(eA,mA)) return a;
        if(is_fp32_inf(eB,mB)) return encode_fp32(~sB,255,0);
    }
    if(g_ops.enable_subnorm) {
        if(is_fp32_subnormal(eA,mA)) eA=1;
        if(is_fp32_subnormal(eB,mB)) eB=1;
    }

    if(sA!=sB) {
        // sign mismatch => +X - (-Y) => X+Y
        sc_uint<32> bNeg = b;
        bNeg[31]= ~sB;
        return fp32_add_1c(a,bNeg);
    } else {
        // same sign => do mantissa difference
        sc_uint<8> eMax=(eA>eB)? eA:eB;
        sc_uint<8> eMin=(eA>eB)? eB:eA;
        sc_uint<23> bigM=(eA>eB)? mA:mB;
        sc_uint<23> smlM=(eA>eB)? mB:mA;
        sc_uint<1> outSign=(eA>eB)? sA:sB;
        sc_uint<8> diff=eMax - eMin;
        sc_uint<24> extSml= (sc_uint<24>)smlM;
        extSml = extSml >> diff;

        // use int for difference
        int diffVal = (int)bigM - (int)extSml;
        if(diffVal<0) {
            outSign= ~outSign;
            diffVal= -diffVal;
        }
        sc_uint<24> mag = (sc_uint<24>) diffVal;
        while(mag[23]==0 && mag!=0 && eMax>0) {
            mag= mag<<1;
            eMax--;
        }
        sc_uint<23> finalM = finalize_round_fp32(mag);
        if(g_ops.enable_clamp) clamp_exponent_fp32(eMax, finalM);
        return encode_fp32(outSign, eMax, finalM);
    }
}

//-------------------------------------------------------------------
// FP32 MUL
//-------------------------------------------------------------------
sc_uint<32> fp32_mul_1c(sc_uint<32> a, sc_uint<32> b)
{
    sc_uint<1> sA,sB;
    sc_uint<8> eA,eB;
    sc_uint<23> mA,mB;
    decode_fp32(a,sA,eA,mA);
    decode_fp32(b,sB,eB,mB);
    sc_uint<1> outSign = sA ^ sB;

    if(g_ops.enable_except) {
        bool nA=is_fp32_nan(eA,mA), nB=is_fp32_nan(eB,mB);
        if(nA||nB) return encode_fp32(0,255,1);
        bool iA=is_fp32_inf(eA,mA), iB=is_fp32_inf(eB,mB);
        if(iA||iB) {
            if((iA && eB!=0)||(iB && eA!=0))
                return encode_fp32(outSign,255,0);
            else
                return encode_fp32(0,255,1);
        }
    }
    if(g_ops.enable_subnorm) {
        if(is_fp32_subnormal(eA,mA)) eA=1;
        if(is_fp32_subnormal(eB,mB)) eB=1;
    }

    int eSum= (int)eA + (int)eB -127;
    if(eSum<1) eSum=1;
    else if(eSum>254) {
        if(g_ops.enable_clamp)
            return encode_fp32(outSign,254,(1<<23)-1);
        else
            return encode_fp32(outSign,255,0);
    }
    sc_uint<24> bigA = (1<<23)| mA;
    sc_uint<24> bigB = (1<<23)| mB;

    // use 64-bit for product
    unsigned long long prod = (unsigned long long)bigA * (unsigned long long)bigB;
    // check top bit => shift if needed
    if((prod >> 47) & 1) {
        prod >>= 1;
        eSum++;
    }
    sc_uint<24> mid = (prod >> 23) & 0xFFFFFF;
    sc_uint<23> finalM = finalize_round_fp32(mid);
    sc_uint<8> outE = eSum;
    if(g_ops.enable_clamp && outE>254) {
        outE=254; finalM=(1<<23)-1;
    }
    else if(outE>=255)
        return encode_fp32(outSign,255,0);
    return encode_fp32(outSign, outE, finalM);
}

//-------------------------------------------------------------------
// BF16 ADD
//-------------------------------------------------------------------
sc_uint<32> bf16_add_1c(sc_uint<32> a, sc_uint<32> b)
{
    sc_uint<1> sA,sB; 
    sc_uint<8> eA,eB; 
    sc_uint<7> mA,mB;
    decode_bf16(a,sA,eA,mA);
    decode_bf16(b,sB,eB,mB);

    if(g_ops.enable_except) {
        if(is_bf16_nan(eA,mA) || is_bf16_nan(eB,mB))
            return encode_bf16(0,255,1);
        if(is_bf16_inf(eA,mA) && is_bf16_inf(eB,mB)) {
            if(sA==sB) return encode_bf16(sA,255,0);
            else       return encode_bf16(0,255,1);
        }
        if(is_bf16_inf(eA,mA)) return a;
        if(is_bf16_inf(eB,mB)) return b;
    }
    if(g_ops.enable_subnorm) {
        if(is_bf16_subnormal(eA,mA)) eA=1;
        if(is_bf16_subnormal(eB,mB)) eB=1;
    }

    if(sA!=sB) {
        // sign mismatch => do sub
        return bf16_sub_1c(a,b);
    }
    sc_uint<8> eMax=(eA>eB)? eA:eB;
    sc_uint<8> diff=(eA>eB)? eA-eB : eB-eA;
    sc_uint<7> bigM=(eA>eB)? mA:mB;
    sc_uint<7> smlM=(eA>eB)? mB:mA;
    sc_uint<8> ext= (sc_uint<8>)smlM;
    ext = ext >> diff;
    sc_uint<8> sum= (sc_uint<8>)bigM + ext;
    if(sum[7]==1) {
        sum=sum>>1;
        eMax++;
    }
    sc_uint<7> finalM;
    if(g_ops.enable_trunc) {
        finalM= sum.range(6,0);
    } else {
        sc_uint<1> rbit= sum[0];
        sum= sum>>1;
        if(rbit==1) sum= sum+1;
        finalM= sum.range(6,0);
    }
    if(g_ops.enable_clamp) clamp_exponent_bf16(eMax, finalM);
    return encode_bf16(sA,eMax,finalM);
}

//-------------------------------------------------------------------
// BF16 SUB
//-------------------------------------------------------------------
sc_uint<32> bf16_sub_1c(sc_uint<32> a, sc_uint<32> b)
{
    sc_uint<1> sA,sB;
    sc_uint<8> eA,eB;
    sc_uint<7> mA,mB;
    decode_bf16(a,sA,eA,mA);
    decode_bf16(b,sB,eB,mB);

    if(g_ops.enable_except) {
        if(is_bf16_nan(eA,mA)|| is_bf16_nan(eB,mB))
            return encode_bf16(0,255,1);
        if(is_bf16_inf(eA,mA)&& is_bf16_inf(eB,mB))
            return encode_bf16(0,255,1);
        if(is_bf16_inf(eA,mA)) return a;
        if(is_bf16_inf(eB,mB)) return encode_bf16(~sB,255,0);
    }
    if(g_ops.enable_subnorm) {
        if(is_bf16_subnormal(eA,mA)) eA=1;
        if(is_bf16_subnormal(eB,mB)) eB=1;
    }

    if(sA!=sB) {
        sc_uint<32> bNeg=b; bNeg[31]= ~sB;
        return bf16_add_1c(a,bNeg);
    } else {
        sc_uint<8> eMax=(eA>eB)? eA:eB;
        sc_uint<8> diff=(eA>eB)? eA-eB : eB-eA;
        sc_uint<7> bigM=(eA>eB)? mA:mA;
        sc_uint<7> smlM=(eA>eB)? mB:mB;
        sc_uint<1> outSign=(eA>eB)? sA:sB;
        sc_uint<8> ext= (sc_uint<8>)smlM;
        ext= ext >> diff;

        // Use int for difference
        int dif = (int)bigM - (int)ext;
        if(dif<0) {
            outSign= ~outSign;
            dif= -dif;
        }
        sc_uint<8> mag= (sc_uint<8>)dif;
        while(mag[7]==0 && mag!=0 && eMax>0){
            mag= mag<<1;
            eMax--;
        }
        sc_uint<7> finalM;
        if(g_ops.enable_trunc) {
            finalM= mag.range(6,0);
        } else {
            sc_uint<1> rbit= mag[0];
            mag= mag>>1;
            if(rbit==1) mag= mag+1;
            finalM= mag.range(6,0);
        }
        if(g_ops.enable_clamp) clamp_exponent_bf16(eMax, finalM);
        return encode_bf16(outSign,eMax,finalM);
    }
}

//-------------------------------------------------------------------
// BF16 MUL
//-------------------------------------------------------------------
sc_uint<32> bf16_mul_1c(sc_uint<32> a, sc_uint<32> b)
{
    sc_uint<1> sA,sB;
    sc_uint<8> eA,eB;
    sc_uint<7> mA,mB;
    decode_bf16(a,sA,eA,mA);
    decode_bf16(b,sB,eB,mB);
    sc_uint<1> outSign= sA^sB;

    if(g_ops.enable_except) {
        bool nanA=is_bf16_nan(eA,mA), nanB=is_bf16_nan(eB,mB);
        if(nanA||nanB) return encode_bf16(0,255,1);
        bool infA=is_bf16_inf(eA,mA), infB=is_bf16_inf(eB,mB);
        if(infA||infB) {
            if((infA && eB!=0)||(infB && eA!=0))
                return encode_bf16(outSign,255,0);
            else
                return encode_bf16(0,255,1);
        }
    }
    if(g_ops.enable_subnorm) {
        if(is_bf16_subnormal(eA,mA)) eA=1;
        if(is_bf16_subnormal(eB,mB)) eB=1;
    }
    int eSum = (int)eA + (int)eB -127;
    if(eSum<1) eSum=1;
    else if(eSum>254) {
        if(g_ops.enable_clamp)
            return encode_bf16(outSign,254,(1<<7)-1);
        else
            return encode_bf16(outSign,255,0);
    }
    sc_uint<8> bigA = (1<<7)| mA;
    sc_uint<8> bigB = (1<<7)| mB;
    unsigned int prod = (unsigned int)bigA*(unsigned int)bigB;
    if((prod>>15)&1) {
        prod= prod>>1;
        eSum++;
    }
    sc_uint<8> sum8 = (prod>>7)&0xFF;
    if(!g_ops.enable_trunc) {
        sc_uint<1> rbit= (prod>>6)&1;
        if(rbit==1) sum8=sum8+1;
    }
    sc_uint<7> finalM= sum8.range(6,0);
    sc_uint<8> outE=eSum;
    if(g_ops.enable_clamp && outE>254) {
        outE=254;
        finalM=(1<<7)-1;
    } else if(outE>=255) {
        return encode_bf16(outSign,255,0);
    }
    return encode_bf16(outSign,outE,finalM);
}

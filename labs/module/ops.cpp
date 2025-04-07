#include "ops.hpp"

//------------------------------------------------------------
// FP32 Helpers: decode, encode, infinity/NaN/subnormal checks,
// clamping, and rounding helper for FP32.
//------------------------------------------------------------

static void decode_fp32(sc_uint<32> val, sc_uint<1>& sign, sc_uint<8>& exp, sc_uint<23>& mant) {
    sign = val[31];
    exp  = val.range(30,23);
    mant = val.range(22,0);
}

static sc_uint<32> encode_fp32(sc_uint<1> sign, sc_uint<8> exp, sc_uint<23> mant) {
    sc_uint<32> out = 0;
    out[31] = sign;
    out.range(30,23) = exp;
    out.range(22,0) = mant;
    return out;
}

static bool is_fp32_inf(sc_uint<8> exp, sc_uint<23> mant) {
    return (exp == 255 && mant == 0);
}

static bool is_fp32_nan(sc_uint<8> exp, sc_uint<23> mant) {
    return (exp == 255 && mant != 0);
}

static bool is_fp32_subnormal(sc_uint<8> exp, sc_uint<23> mant) {
    return (exp == 0 && mant != 0);
}

static void clamp_exponent_fp32(sc_uint<8>& exp, sc_uint<23>& mant) {
    // Clamp if exponent exceeds 254 (since 255 is reserved for Inf/NaN)
    if(exp > 254) {
        exp = 254;
        mant = (1 << 23) - 1;  // set mantissa to maximal value
    }
}

static sc_uint<23> finalize_round_fp32(sc_uint<24> sum, bool enable_trunc) {
    // The 24-bit sum includes an extra bit for rounding.
    if(enable_trunc) {
        // Truncate: simply drop the least significant bit.
        sum = sum >> 1;
    } else {
        // Round half-up: check the least significant bit before shifting.
        sc_uint<1> roundBit = sum[0];
        sum = sum >> 1;
        if(roundBit == 1) {
            sum = sum + 1;
        }
    }
    return sum.range(22, 0);
}

//------------------------------------------------------------
// BF16 Helpers: decode, encode, checks, clamping, rounding for BF16.
// BF16 is stored in the top 16 bits (1 sign, 8 exponent, 7 mantissa).
//------------------------------------------------------------
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

static bool is_bf16_inf(sc_uint<8> exp, sc_uint<7> mant) {
    return (exp == 255 && mant == 0);
}

static bool is_bf16_nan(sc_uint<8> exp, sc_uint<7> mant) {
    return (exp == 255 && mant != 0);
}

static bool is_bf16_subnormal(sc_uint<8> exp, sc_uint<7> mant) {
    return (exp == 0 && mant != 0);
}

static void clamp_exponent_bf16(sc_uint<8>& exp, sc_uint<7>& mant) {
    if(exp > 254) {
        exp = 254;
        mant = (1 << 7) - 1;
    }
}

static sc_uint<7> finalize_round_bf16(sc_uint<8> sum, bool enable_trunc) {
    // For BF16, sum is 8 bits; keep the upper 7 bits.
    if(enable_trunc) {
        sum = sum >> 1;
    } else {
        sc_uint<1> roundBit = sum[0];
        sum = sum >> 1;
        if(roundBit == 1)
            sum = sum + 1;
    }
    return sum.range(6, 0);
}

//------------------------------------------------------------
// FP32 Add Implementation
//------------------------------------------------------------
sc_uint<32> fp32_add_1c(sc_uint<32> a, sc_uint<32> b,
                        bool enable_subnorm, bool enable_trunc,
                        bool enable_clamp, bool enable_except)
{
    sc_uint<1> sA, sB;
    sc_uint<8> eA, eB;
    sc_uint<23> mA, mB;
    decode_fp32(a, sA, eA, mA);
    decode_fp32(b, sB, eB, mB);

    // Exception handling for NaN/Infinity:
    if(enable_except) {
        if(is_fp32_nan(eA, mA) || is_fp32_nan(eB, mB))
            return encode_fp32(0, 255, 1);
        if(is_fp32_inf(eA, mA) && is_fp32_inf(eB, mB)) {
            if(sA == sB) return encode_fp32(sA, 255, 0);
            else return encode_fp32(0, 255, 1);
        }
        if(is_fp32_inf(eA, mA))
            return a;
        if(is_fp32_inf(eB, mB))
            return b;
    }

    // Normalize subnormals if enabled:
    if(enable_subnorm) {
        if(is_fp32_subnormal(eA, mA)) eA = 1;
        if(is_fp32_subnormal(eB, mB)) eB = 1;
    }

    // If signs differ, perform subtraction via robust 2's complement.
    if(sA != sB) {
        // Let fp32_sub_1c handle this case robustly.
        // For addition, we assume same-sign inputs.
        return fp32_sub_1c(a, b, enable_subnorm, enable_trunc, enable_clamp, enable_except);
    }

    // Align mantissas
    sc_uint<8> eMax = (eA > eB) ? eA : eB;
    sc_uint<8> eMin = (eA > eB) ? eB : eA;
    sc_uint<23> bigM = (eA > eB) ? mA : mB;
    sc_uint<23> smlM = (eA > eB) ? mB : mA;
    sc_uint<8> diff = eMax - eMin;
    sc_uint<24> extSml = (sc_uint<24>)smlM;
    extSml = extSml >> diff;
    sc_uint<24> sum = (sc_uint<24>)bigM + extSml;
    if(sum[23] == 1) {
        sum = sum >> 1;
        eMax++;
    }
    sc_uint<23> finalMant = finalize_round_fp32(sum, enable_trunc);
    if(enable_clamp) clamp_exponent_fp32(eMax, finalMant);
    return encode_fp32(sA, eMax, finalMant);
}

//------------------------------------------------------------
// FP32 Sub Implementation
//------------------------------------------------------------
sc_uint<32> fp32_sub_1c(sc_uint<32> a, sc_uint<32> b,
                        bool enable_subnorm, bool enable_trunc,
                        bool enable_clamp, bool enable_except)
{
    sc_uint<1> sA, sB;
    sc_uint<8> eA, eB;
    sc_uint<23> mA, mB;
    decode_fp32(a, sA, eA, mA);
    decode_fp32(b, sB, eB, mB);

    if(enable_except) {
        if(is_fp32_nan(eA, mA) || is_fp32_nan(eB, mB))
            return encode_fp32(0, 255, 1);
        if(is_fp32_inf(eA, mA) && is_fp32_inf(eB, mB))
            return encode_fp32(0, 255, 1);
        if(is_fp32_inf(eA, mA))
            return a;
        if(is_fp32_inf(eB, mB))
            return encode_fp32(~sB, 255, 0);
    }

    if(enable_subnorm) {
        if(is_fp32_subnormal(eA, mA)) eA = 1;
        if(is_fp32_subnormal(eB, mB)) eB = 1;
    }

    // Subtraction: if the signs are the same, we subtract mantissas.
    // We compare the magnitudes and use 2's complement arithmetic.
    sc_uint<8> eMax = (eA > eB) ? eA : eB;
    sc_uint<8> eMin = (eA > eB) ? eB : eA;
    sc_uint<23> bigM = (eA > eB) ? mA : mB;
    sc_uint<23> smlM = (eA > eB) ? mB : mA;
    sc_uint<1> bigS = (eA > eB) ? sA : sB;
    sc_uint<1> smlS = (eA > eB) ? sB : sA;
    sc_uint<8> diff = eMax - eMin;
    sc_uint<24> extSml = (sc_uint<24>)smlM;
    extSml = extSml >> diff;

    if(bigS != smlS) {
        // Mixed signs become addition. Delegate to fp32_add_1c.
        return fp32_add_1c(a, b, enable_subnorm, enable_trunc, enable_clamp, enable_except);
    } else {
        sc_int<25> diffVal = (sc_int<25>)bigM - (sc_int<25>)extSml;
        sc_uint<1> outSign = bigS;
        if(diffVal < 0) {
            outSign = ~bigS;
            diffVal = -diffVal;
        }
        sc_uint<25> mag = (sc_uint<25>)diffVal;
        while(mag[24] == 0 && mag != 0 && eMax > 0) {
            mag = mag << 1;
            eMax--;
        }
        sc_uint<24> mag24 = mag;
        sc_uint<23> finalMant = finalize_round_fp32(mag24, enable_trunc);
        if(enable_clamp) clamp_exponent_fp32(eMax, finalMant);
        return encode_fp32(outSign, eMax, finalMant);
    }
}

//------------------------------------------------------------
// FP32 Mul Implementation
//------------------------------------------------------------
sc_uint<32> fp32_mul_1c(sc_uint<32> a, sc_uint<32> b,
                        bool enable_subnorm, bool enable_trunc,
                        bool enable_clamp, bool enable_except)
{
    sc_uint<1> sA, sB;
    sc_uint<8> eA, eB;
    sc_uint<23> mA, mB;
    decode_fp32(a, sA, eA, mA);
    decode_fp32(b, sB, eB, mB);

    sc_uint<1> outSign = sA ^ sB;

    if(enable_except) {
        if(is_fp32_nan(eA, mA) || is_fp32_nan(eB, mB))
            return encode_fp32(0, 255, 1);
        bool infA = is_fp32_inf(eA, mA), infB = is_fp32_inf(eB, mB);
        if(infA || infB) {
            if((infA && eB != 0) || (infB && eA != 0))
                return encode_fp32(outSign, 255, 0);
            else
                return encode_fp32(0, 255, 1);
        }
    }
    if(enable_subnorm) {
        if(is_fp32_subnormal(eA, mA)) eA = 1;
        if(is_fp32_subnormal(eB, mB)) eB = 1;
    }

    sc_int<9> eSum = (sc_int<9>)eA + (sc_int<9>)eB - 127;
    if(eSum < 1) eSum = 1;
    else if(eSum > 254 && enable_clamp) {
        return encode_fp32(outSign, 254, (1<<23)-1);
    } else if(eSum > 254) {
        return encode_fp32(outSign, 255, 0);
    }

    sc_uint<24> bigA = (1 << 23) | mA;
    sc_uint<24> bigB = (1 << 23) | mB;
    sc_uint<48> prod = (sc_uint<48>)bigA * (sc_uint<48>)bigB;
    if(prod[47] == 1) {
        prod = prod >> 1;
        eSum++;
    }
    sc_uint<24> prod24 = prod.range(46,23);
    sc_uint<23> finalMant = finalize_round_fp32(prod24, enable_trunc);
    sc_uint<8> outExp = (sc_uint<8>)eSum;
    if(enable_clamp && outExp > 254) {
        outExp = 254;
        finalMant = (1 << 23) - 1;
    } else if(outExp >= 255) {
        return encode_fp32(outSign, 255, 0);
    }
    return encode_fp32(outSign, outExp, finalMant);
}

//------------------------------------------------------------
// BF16 Add Implementation
//------------------------------------------------------------
sc_uint<32> bf16_add_1c(sc_uint<32> a, sc_uint<32> b,
                        bool enable_subnorm, bool enable_trunc,
                        bool enable_clamp, bool enable_except)
{
    sc_uint<1> sA, sB;
    sc_uint<8> eA, eB;
    sc_uint<7> mA, mB;
    decode_bf16(a, sA, eA, mA);
    decode_bf16(b, sB, eB, mB);

    if(enable_except) {
        if(is_bf16_nan(eA, mA) || is_bf16_nan(eB, mB))
            return encode_bf16(0,255,1);
        if(is_bf16_inf(eA, mA) && is_bf16_inf(eB, mB)) {
            if(sA==sB) return encode_bf16(sA,255,0);
            else return encode_bf16(0,255,1);
        }
        if(is_bf16_inf(eA, mA)) return a;
        if(is_bf16_inf(eB, mB)) return b;
    }
    if(enable_subnorm) {
        if(is_bf16_subnormal(eA, mA)) eA = 1;
        if(is_bf16_subnormal(eB, mB)) eB = 1;
    }
    if(sA != sB) return 0;
    sc_uint<8> eMax = (eA > eB)? eA : eB;
    sc_uint<8> eMin = (eA > eB)? eB : eA;
    sc_uint<7> bigM = (eA > eB)? mA : mB;
    sc_uint<7> smlM = (eA > eB)? mB : mA;
    sc_uint<8> diff = eMax - eMin;
    sc_uint<8> extSml = (sc_uint<8>)smlM;
    extSml = extSml >> diff;
    sc_uint<8> sum = (sc_uint<8>)bigM + extSml;
    if(sum[7] == 1) {
        sum = sum >> 1;
        eMax++;
    }
    sc_uint<7> finalM = finalize_round_bf16(sum, enable_trunc);
    if(enable_clamp) clamp_exponent_bf16(eMax, finalM);
    return encode_bf16(sA, eMax, finalM);
}

//------------------------------------------------------------
// BF16 Sub Implementation
//------------------------------------------------------------
sc_uint<32> bf16_sub_1c(sc_uint<32> a, sc_uint<32> b,
                        bool enable_subnorm, bool enable_trunc,
                        bool enable_clamp, bool enable_except)
{
    sc_uint<1> sA, sB;
    sc_uint<8> eA, eB;
    sc_uint<7> mA, mB;
    decode_bf16(a, sA, eA, mA);
    decode_bf16(b, sB, eB, mB);

    if(enable_except) {
        if(is_bf16_nan(eA, mA) || is_bf16_nan(eB, mB))
            return encode_bf16(0,255,1);
        if(is_bf16_inf(eA, mA) && is_bf16_inf(eB, mB))
            return encode_bf16(0,255,1);
        if(is_bf16_inf(eA, mA)) return a;
        if(is_bf16_inf(eB, mB)) return encode_bf16(~sB,255,0);
    }
    if(enable_subnorm) {
        if(is_bf16_subnormal(eA, mA)) eA = 1;
        if(is_bf16_subnormal(eB, mB)) eB = 1;
    }
    sc_uint<8> eMax = (eA > eB) ? eA : eB;
    sc_uint<8> eMin = (eA > eB) ? eB : eA;
    sc_uint<7> bigM = (eA > eB) ? mA : mA;
    sc_uint<7> smlM = (eA > eB) ? mB : mB;
    sc_uint<1> bigS = (eA > eB) ? sA : sB;
    sc_uint<1> smlS = (eA > eB) ? sB : sA;
    sc_uint<8> diff = eMax - eMin;
    sc_uint<8> ext = (sc_uint<8>)smlM;
    ext = ext >> diff;
    if(bigS != smlS) {
        // treat as addition
        sc_uint<8> sum = (sc_uint<8>)bigM + ext;
        if(sum[7]==1) {
            sum = sum >> 1;
            eMax++;
        }
        sc_uint<7> finalM = finalize_round_bf16(sum, enable_trunc);
        if(enable_clamp) clamp_exponent_bf16(eMax, finalM);
        return encode_bf16(bigS, eMax, finalM);
    } else {
        sc_int<8> dif = (sc_int<8>)bigM - (sc_int<8>)ext;
        sc_uint<1> outSign = bigS;
        if(dif < 0) {
            outSign = ~bigS;
            dif = -dif;
        }
        sc_uint<8> mag = (sc_uint<8>)dif;
        while(mag[7]==0 && mag != 0 && eMax > 0) {
            mag = mag << 1;
            eMax--;
        }
        sc_uint<8> sumU = mag;
        sc_uint<7> finalM = finalize_round_bf16(sumU, enable_trunc);
        if(enable_clamp) clamp_exponent_bf16(eMax, finalM);
        return encode_bf16(outSign, eMax, finalM);
    }
}

//------------------------------------------------------------
// BF16 Mul Implementation
//------------------------------------------------------------
sc_uint<32> bf16_mul_1c(sc_uint<32> a, sc_uint<32> b,
                        bool enable_subnorm, bool enable_trunc,
                        bool enable_clamp, bool enable_except)
{
    sc_uint<1> sA, sB;
    sc_uint<8> eA, eB;
    sc_uint<7> mA, mB;
    decode_bf16(a, sA, eA, mA);
    decode_bf16(b, sB, eB, mB);

    sc_uint<1> outSign = sA ^ sB;
    if(enable_except) {
        bool nanA = is_bf16_nan(eA, mA), nanB = is_bf16_nan(eB, mB);
        if(nanA || nanB) return encode_bf16(0,255,1);
        bool infA = is_bf16_inf(eA, mA), infB = is_bf16_inf(eB, mB);
        if(infA || infB) {
            if((infA && eB != 0) || (infB && eA != 0))
                return encode_bf16(outSign,255,0);
            else
                return encode_bf16(0,255,1);
        }
    }
    if(enable_subnorm) {
        if(is_bf16_subnormal(eA, mA)) eA = 1;
        if(is_bf16_subnormal(eB, mB)) eB = 1;
    }
    sc_int<9> eSum = (sc_int<9>)eA + (sc_int<9>)eB - 127;
    if(eSum < 1) eSum = 1;
    else if(eSum > 254 && enable_clamp)
        return encode_bf16(outSign,254,(1<<7)-1);
    else if(eSum > 254)
        return encode_bf16(outSign,255,0);

    sc_uint<8> bigA = (1 << 7) | mA;
    sc_uint<8> bigB = (1 << 7) | mB;
    sc_uint<16> prod = (sc_uint<16>)bigA * (sc_uint<16>)bigB;
    if(prod[15] == 1) {
        prod = prod >> 1;
        eSum++;
    }
    sc_uint<8> sumU = prod.range(14,7);
    if(enable_trunc) {
        // do nothing (truncate)
    } else {
        sc_uint<1> rbit = prod[6];
        if(rbit == 1) sumU = sumU + 1;
    }
    sc_uint<7> finalM = sumU.range(6,0);
    sc_uint<8> outExp = (sc_uint<8>)eSum;
    if(enable_clamp && outExp > 254) {
        outExp = 254;
        finalM = (1<<7)-1;
    } else if(outExp >= 255)
        return encode_bf16(outSign,255,0);
    return encode_bf16(outSign, outExp, finalM);
}

#include "ops.hpp"

//------------------------------------------
// Helpers to decode / encode FP32
//------------------------------------------
static void decode_fp32(sc_uint<32> inVal,
                        sc_uint<1> &sign,
                        sc_uint<8> &exp,
                        sc_uint<23> &mant)
{
    sign = inVal[31];
    exp  = inVal.range(30,23);
    mant = inVal.range(22,0);
}

static sc_uint<32> encode_fp32(sc_uint<1> sign,
                               sc_uint<8> exp,
                               sc_uint<23> mant)
{
    sc_uint<32> out = 0;
    out[31] = sign;
    for(int i=0;i<8;i++){
        out[23+i] = exp[i];
    }
    for(int i=0;i<23;i++){
        out[i] = mant[i];
    }
    return out;
}

//------------------------------------------
// Helpers to decode / encode BF16
//------------------------------------------
static void decode_bf16(sc_uint<32> inVal,
                        sc_uint<1> &sign,
                        sc_uint<8> &exp,
                        sc_uint<7> &mant)
{
    sign = inVal[31];
    exp  = inVal.range(30,23);
    mant = inVal.range(22,16);
}

static sc_uint<32> encode_bf16(sc_uint<1> sign,
                               sc_uint<8> exp,
                               sc_uint<7> mant)
{
    sc_uint<32> out=0;
    out[31]=sign;
    for(int i=0;i<8;i++){
        out[23+i]=exp[i];
    }
    for(int i=0;i<7;i++){
        out[16+i]=mant[i];
    }
    return out;
}

//------------------------------------------
// 1) FP32 Add (one-cycle, naive big combinational approach)
//------------------------------------------
sc_uint<32> fp32_add_1cycle(sc_uint<32> a, sc_uint<32> b)
{
    // decode
    sc_uint<1> sA, sB;
    sc_uint<8> eA, eB;
    sc_uint<23> mA, mB;
    decode_fp32(a, sA, eA, mA);
    decode_fp32(b, sB, eB, mB);

    // if sign differs => skip => return 0
    if(sA != sB) {
        return 0;
    }

    // naive alignment
    sc_uint<8> eMax = (eA>eB)? eA : eB;
    sc_uint<8> eMin = (eA>eB)? eB : eA;
    sc_uint<23> mBig = (eA>eB)? mA : mB;
    sc_uint<23> mSml = (eA>eB)? mB : mA;

    sc_uint<8> diff = eMax - eMin;
    sc_uint<24> extSml = (sc_uint<24>)mSml;
    extSml = extSml >> diff; // shift

    // add mantissas
    sc_uint<24> sum = (sc_uint<24>)mBig + extSml;

    // naive normalize => if sum[23]==1 => shift
    sc_uint<8> eOut = eMax;
    if(sum[23]==1){
        // overflow => shift right
        sum = sum >> 1;
        eOut = eOut + 1;
    }
    // store final mant
    sc_uint<23> mOut = sum.range(22,0);

    return encode_fp32(sA, eOut, mOut);
}

//------------------------------------------
// 2) FP32 Sub
//------------------------------------------
sc_uint<32> fp32_sub_1cycle(sc_uint<32> a, sc_uint<32> b)
{
    // decode
    sc_uint<1> sA, sB;
    sc_uint<8> eA, eB;
    sc_uint<23> mA, mB;
    decode_fp32(a, sA, eA, mA);
    decode_fp32(b, sB, eB, mB);

    // if sign differs => skip => 0
    if(sA != sB){
        return 0;
    }

    // alignment
    sc_uint<8> eMax = (eA>eB)? eA : eB;
    sc_uint<8> eMin = (eA>eB)? eB : eA;
    sc_uint<23> mBig = (eA>eB)? mA : mB;
    sc_uint<23> mSml = (eA>eB)? mB : mA;
    sc_uint<8> diff = eMax - eMin;
    sc_uint<24> extSml = (sc_uint<24>)mSml;
    extSml = extSml >> diff;

    // subtract
    sc_int<25> subval = (sc_int<25>)mBig - (sc_int<25>)extSml;
    sc_uint<24> sum=0;
    sc_uint<1> sOut=sA;
    if(subval<0){
        sOut=~sA; 
        subval=-subval;
    }
    sum=(sc_uint<24>)subval;

    // naive normalize => shift left if leading zeros
    sc_uint<8> eOut=eMax;
    while(sum[23]==0 && sum!=0 && eOut>0){
        sum=sum<<1;
        eOut=eOut-1;
    }
    sc_uint<23> mOut=sum.range(22,0);
    return encode_fp32(sOut, eOut, mOut);
}

//------------------------------------------
// 3) BF16 Add
//------------------------------------------
sc_uint<32> bf16_add_1cycle(sc_uint<32> a, sc_uint<32> b)
{
    sc_uint<1> sA,sB;
    sc_uint<8> eA,eB;
    sc_uint<7> mA,mB;
    decode_bf16(a, sA, eA, mA);
    decode_bf16(b, sB, eB, mB);

    if(sA!=sB) {
        return 0;
    }

    sc_uint<8> eMax=(eA>eB)?eA:eB;
    sc_uint<8> eMin=(eA>eB)?eB:eA;
    sc_uint<7> bigM=(eA>eB)?mA:mB;
    sc_uint<7> smlM=(eA>eB)?mB:mA;
    sc_uint<8> diff=eMax-eMin;
    sc_uint<8> extSml=(sc_uint<8>)smlM;
    extSml=extSml >> diff;

    sc_uint<8> sum=(sc_uint<8>)bigM + extSml;
    // overflow?
    if(sum[7]==1){
        sum=sum>>1; 
        eMax=eMax+1;
    }
    sc_uint<7> mOut=sum.range(6,0);
    return encode_bf16(sA,eMax,mOut);
}

//------------------------------------------
// 4) BF16 Sub
//------------------------------------------
sc_uint<32> bf16_sub_1cycle(sc_uint<32> a, sc_uint<32> b)
{
    sc_uint<1> sA,sB;
    sc_uint<8> eA,eB;
    sc_uint<7> mA,mB;
    decode_bf16(a,sA,eA,mA);
    decode_bf16(b,sB,eB,mB);

    if(sA!=sB){
        return 0;
    }

    sc_uint<8> eMax=(eA>eB)?eA:eB;
    sc_uint<8> eMin=(eA>eB)?eB:eA;
    sc_uint<7> bigM=(eA>eB)?mA:mB;
    sc_uint<7> smlM=(eA>eB)?mB:mA;
    sc_uint<8> diff=eMax-eMin;
    sc_uint<8> extSml=(sc_uint<8>)smlM;
    extSml=extSml >> diff;

    sc_int<9> subval=(sc_int<9>)bigM - (sc_int<9>)extSml;
    sc_uint<8> sum=0;
    sc_uint<1> sOut=sA;
    if(subval<0){
        sOut=~sA;
        subval=-subval;
    }
    sum=(sc_uint<8>)subval;

    // naive left shift if zero
    while(sum[7]==0 && sum!=0 && eMax>0){
        sum=sum<<1;
        eMax=eMax-1;
    }
    sc_uint<7> mOut=sum.range(6,0);
    return encode_bf16(sOut,eMax,mOut);
}

#include "bf16_add_unit.hpp"

/**
 * Similar multi-cycle approach for BF16:
 *   - top 16 bits => sign(1), exponent(8), mantissa(7)
 */

bf16_add_unit::bf16_add_unit(sc_core::sc_module_name name)
 : sc_module(name)
{
    SC_CTHREAD(add_fsm_thread, clk.pos());
    reset_signal_is(reset, true);
}

void bf16_add_unit::decode_bf16(sc_uint<32> inWord,
                                sc_uint<1>& sign,
                                sc_uint<8>& exp,
                                sc_uint<7>& mant)
{
    sign = inWord[31];
    exp  = inWord.range(30,23);
    mant = inWord.range(22,16);
}

sc_uint<32> bf16_add_unit::encode_bf16(sc_uint<1> sign,
                                       sc_uint<8> exp,
                                       sc_uint<7> mant)
{
    sc_uint<32> out=0;
    out[31] = sign;
    for(int i=0; i<8; i++){
        out[23+i] = exp[i];
    }
    for(int i=0; i<7; i++){
        out[16+i] = mant[i];
    }
    return out;
}

void bf16_add_unit::add_fsm_thread()
{
    done.write(false);
    result.write(0);
    st_reg = ST_IDLE;
    sA=0; sB=0; eA=0; eB=0; mA=0; mB=0;
    sOut=0; eOut=0; manTmp=0;
    wait();

    while(true){
        AddState st = st_reg.read();
        AddState st_next = st;

        if(st == ST_IDLE){
            if(start.read()==true){
                // decode
                sc_uint<1> ssA, ssB;
                sc_uint<8> eeA, eeB;
                sc_uint<7> mmA, mmB;
                decode_bf16(a.read(), ssA, eeA, mmA);
                decode_bf16(b.read(), ssB, eeB, mmB);
                sA=ssA; sB=ssB; eA=eeA; eB=eeB; mA=mmA; mB=mmB;
                st_next = ST_ALIGN;
            }
        }
        else if(st == ST_ALIGN){
            sc_uint<8> ea=eA.read(), eb=eB.read();
            sc_uint<7> ma=mA.read(), mb=mB.read();
            if(ea==0 && ma!=0) ea=1;
            if(eb==0 && mb!=0) eb=1;

            if(ea>eb){
                sc_uint<8> diff=ea-eb;
                sc_uint<8> extB=(sc_uint<8>)mb;
                extB = extB >> diff;
                mB=extB.range(6,0);
            }
            else if(eb>ea){
                sc_uint<8> diff=eb-ea;
                sc_uint<8> extA=(sc_uint<8>)ma;
                extA = extA >> diff;
                mA=extA.range(6,0);
            }
            sOut=sA.read(); // naive
            st_next=ST_ADD;
        }
        else if(st==ST_ADD){
            sc_uint<8> bigA=(sc_uint<8>)mA.read();
            sc_uint<8> bigB=(sc_uint<8>)mB.read();
            sc_uint<8> sum= bigA + bigB;
            manTmp=sum;
            eOut = (eA.read() > eB.read()) ? eA.read() : eB.read();
            st_next=ST_NORMALIZE;
        }
        else if(st==ST_NORMALIZE){
            sc_uint<8> sumtmp=manTmp.read();
            sc_uint<8> ecurr=eOut.read();
            if(sumtmp[7]==1){
                // overflow => shift right
                sumtmp=sumtmp >> 1;
                ecurr=ecurr+1;
            }
            manTmp=sumtmp;
            eOut=ecurr;
            st_next=ST_ROUND;
        }
        else if(st==ST_ROUND){
            // BF16 rounding => keep 7 bits
            // manTmp is 8 bits
            sc_uint<1> roundBit=manTmp.read()[0];
            sc_uint<7> main7=manTmp.read().range(7,1);
            if(roundBit==1){
                main7=main7+1;
            }
            manTmp=main7;
            st_next=ST_PACK;
        }
        else if(st==ST_PACK){
            sc_uint<1> sfinal=sOut.read();
            sc_uint<8> efinal=eOut.read();
            sc_uint<7> mfinal=manTmp.read().range(6,0);
            sc_uint<32> outVal=encode_bf16(sfinal, efinal, mfinal);
            result.write(outVal);
            st_next=ST_DONE;
        }
        else if(st==ST_DONE){
            done.write(true);
            st_next=ST_IDLE;
        }

        if(st!=st_next && st==ST_DONE){
            done.write(false);
        }
        st_reg.write(st_next);
        wait();
    }
}

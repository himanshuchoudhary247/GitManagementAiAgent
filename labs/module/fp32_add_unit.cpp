#include "fp32_add_unit.hpp"

/**
 * Basic multi-cycle FP32 add approach:
 *   - IDLE: wait for start=1
 *   - ALIGN: compare exponents, shift smaller mantissa
 *   - ADD: add or subtract mantissas
 *   - NORMALIZE: shift result if needed
 *   - ROUND: naive rounding
 *   - PACK: assemble bits
 *   - DONE: set done=1 for 1 cycle, go back to IDLE
 */

fp32_add_unit::fp32_add_unit(sc_core::sc_module_name name)
 : sc_module(name)
 , clk("clk")
 , reset("reset")
 , start("start")
 , a("a")
 , b("b")
 , done("done")
 , result("result")
{
    SC_CTHREAD(add_fsm_thread, clk.pos());
    reset_signal_is(reset, true);
}

void fp32_add_unit::decode_fp32(sc_uint<32> inWord,
                                sc_uint<1>& sign,
                                sc_uint<8>& exp,
                                sc_uint<23>& mant)
{
    sign = inWord[31];
    exp  = inWord.range(30,23);
    mant = inWord.range(22,0);
}

sc_uint<32> fp32_add_unit::encode_fp32(sc_uint<1> sign,
                                       sc_uint<8> exp,
                                       sc_uint<23> mant)
{
    sc_uint<32> out=0;
    out[31] = sign;
    for(int i=0; i<8; i++){
        out[23+i] = exp[i];
    }
    for(int i=0; i<23; i++){
        out[i] = mant[i];
    }
    return out;
}

void fp32_add_unit::add_fsm_thread()
{
    // On reset
    done.write(false);
    result.write(0);
    st_reg = ST_IDLE;
    sA = 0; sB = 0;
    eA = 0; eB = 0;
    mA = 0; mB = 0;
    sOut=0; eOut=0; manTmp=0;
    wait();

    while(true) {
        AddState st = st_reg.read();
        AddState st_next = st;

        if(st == ST_IDLE) {
            // Wait for start=1
            if(start.read()==true) {
                // decode inputs a,b
                sc_uint<32> av = a.read();
                sc_uint<32> bv = b.read();
                sc_uint<1> ssA, ssB;
                sc_uint<8> eeA, eeB;
                sc_uint<23> mmA, mmB;
                decode_fp32(av, ssA, eeA, mmA);
                decode_fp32(bv, ssB, eeB, mmB);

                sA = ssA; sB = ssB;
                eA = eeA; eB = eeB;
                mA = mmA; mB = mmB;

                st_next = ST_ALIGN;
            }
        }
        else if(st == ST_ALIGN) {
            // Compare exponents eA,eB => shift smaller mant by exponent difference
            sc_uint<8> ea = eA.read();
            sc_uint<8> eb = eB.read();
            sc_uint<23> ma = mA.read();
            sc_uint<23> mb = mB.read();

            // if exp=0 => subnormal => we treat mant as 0, exponent=1 in naive approach
            if(ea==0 && ma!=0) ea=1;
            if(eb==0 && mb!=0) eb=1;

            if(ea>eb){
                sc_uint<8> diff = ea-eb;
                sc_uint<24> extended = (sc_uint<24>)mb;
                extended = extended >> diff;
                mB = extended.range(22,0);
                eB = eb; // remains
                eA = ea; // bigger
            }
            else if(eb>ea) {
                sc_uint<8> diff = eb-ea;
                sc_uint<24> extended = (sc_uint<24>)ma;
                extended = extended >> diff;
                mA = extended.range(22,0);
                eA = ea; // remains
                eB = eb; // bigger
            }
            // sign out => if same sign or big exponent sign
            sOut = sA.read(); // naive
            st_next = ST_ADD;
        }
        else if(st == ST_ADD) {
            // simple add mantissas
            sc_uint<24> bigA = (sc_uint<24>)mA.read();
            sc_uint<24> bigB = (sc_uint<24>)mB.read();
            sc_uint<24> sum = bigA + bigB;
            manTmp = sum;
            // choose exponent => bigger of eA,eB
            eOut = (eA.read()>eB.read()) ? eA.read() : eB.read();
            st_next = ST_NORMALIZE;
        }
        else if(st == ST_NORMALIZE) {
            // if sum overflow bit => shift right
            sc_uint<24> sumtmp = manTmp.read();
            sc_uint<8> ecur = eOut.read();
            if(sumtmp[23]==1) {
                // overflow => shift right 1
                sumtmp = sumtmp >> 1;
                ecur = ecur + 1; // inc exponent
            }
            // if the top bits are 0 => subnormal => shift?
            // For demonstration, we do minimal
            manTmp = sumtmp;
            eOut = ecur;
            st_next = ST_ROUND;
        }
        else if(st == ST_ROUND) {
            // naive rounding => just keep 23 bits
            // manTmp is 24 bits
            sc_uint<24> tmp = manTmp.read();
            // Check bit0 for naive round half up => ignoring real hardware rounding modes
            sc_uint<1> roundBit = tmp[0];
            sc_uint<23> main23 = tmp.range(23,1);

            if(roundBit==1) {
                main23 = main23 + 1;
            }

            manTmp = main23;
            st_next = ST_PACK;
        }
        else if(st == ST_PACK) {
            // pack sign, exponent, mant
            sc_uint<1> sfinal = sOut.read();
            sc_uint<8> efinal = eOut.read();
            sc_uint<23> mfinal = manTmp.read().range(22,0);
            sc_uint<32> outVal = encode_fp32(sfinal, efinal, mfinal);
            result.write(outVal);
            st_next = ST_DONE;
        }
        else if(st == ST_DONE) {
            // pulse done for 1 cycle
            done.write(true);
            // next cycle => done=0 => back to IDLE
            st_next = ST_IDLE;
        }

        // state update
        if(st != st_next && st==ST_DONE) {
            // leaving DONE => drop done
            done.write(false);
        }

        st_reg.write(st_next);
        wait();
    }
}

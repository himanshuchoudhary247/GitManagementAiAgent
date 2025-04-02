#ifndef BF16_ADD_UNIT_HPP
#define BF16_ADD_UNIT_HPP

#include <systemc.h>

struct bf16_add_unit : sc_core::sc_module {
    // Ports
    sc_in<bool> clk;
    sc_in<bool> reset;
    sc_in<bool> start;
    sc_in< sc_uint<32> > a; // top 16 bits => BF16
    sc_in< sc_uint<32> > b;
    sc_out<bool> done;
    sc_out< sc_uint<32> > result;

    enum AddState {
        ST_IDLE,
        ST_ALIGN,
        ST_ADD,
        ST_NORMALIZE,
        ST_ROUND,
        ST_PACK,
        ST_DONE
    };

    sc_signal<AddState> st_reg;

    // decode latches
    sc_signal< sc_uint<1> > sA, sB;
    sc_signal< sc_uint<8> > eA, eB;
    sc_signal< sc_uint<7> > mA, mB;

    sc_signal< sc_uint<1> > sOut;
    sc_signal< sc_uint<8> > eOut;
    sc_signal< sc_uint<8> > manTmp; // 1 extra bit for overflow

    SC_HAS_PROCESS(bf16_add_unit);
    bf16_add_unit(sc_core::sc_module_name name);

    void add_fsm_thread();

private:
    void decode_bf16(sc_uint<32> inWord,
                     sc_uint<1>& sign,
                     sc_uint<8>& exp,
                     sc_uint<7>& mant);
    sc_uint<32> encode_bf16(sc_uint<1> sign,
                            sc_uint<8> exp,
                            sc_uint<7> mant);
};

#endif

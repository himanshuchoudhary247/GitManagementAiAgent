#ifndef FP32_ADD_UNIT_HPP
#define FP32_ADD_UNIT_HPP

#include <systemc.h>

/**
 * A multi-cycle FP32 add unit with alignment, normalization, rounding, etc.
 * No switch statements, using an if/else FSM instead.
 */
struct fp32_add_unit : sc_core::sc_module {
    // Ports
    sc_in<bool> clk;
    sc_in<bool> reset;

    sc_in<bool> start;       // handshake: start operation
    sc_in< sc_uint<32> > a;  // FP32 in
    sc_in< sc_uint<32> > b;  // FP32 in

    sc_out<bool> done;       // handshake: signals result is ready
    sc_out< sc_uint<32> > result;

    // Pipeline states
    enum AddState {
        ST_IDLE,
        ST_ALIGN,
        ST_ADD,
        ST_NORMALIZE,
        ST_ROUND,
        ST_PACK,
        ST_DONE
    };

    // Current state
    sc_signal<AddState> st_reg;

    // Internal latches
    // decode: sign, exponent, mantissa
    sc_signal< sc_uint<1> > sA, sB;
    sc_signal< sc_uint<8> > eA, eB;
    sc_signal< sc_uint<23> > mA, mB;

    sc_signal< sc_uint<1> > sOut;
    sc_signal< sc_uint<8> > eOut;
    sc_signal< sc_uint<24> > manTmp;  // 1 extra bit for potential overflow

    SC_HAS_PROCESS(fp32_add_unit);
    fp32_add_unit(sc_core::sc_module_name name);

    void add_fsm_thread();

private:
    // decode helpers
    void decode_fp32(sc_uint<32> inWord,
                     sc_uint<1>& sign,
                     sc_uint<8>& exp,
                     sc_uint<23>& mant);

    sc_uint<32> encode_fp32(sc_uint<1> sign,
                            sc_uint<8> exp,
                            sc_uint<23> mant);
};

#endif

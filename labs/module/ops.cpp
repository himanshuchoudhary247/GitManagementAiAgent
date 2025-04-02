#include "ops.hpp"

sc_uint<32> hw_int_add(sc_uint<32> a, sc_uint<32> b)
{
    sc_uint<33> tmp = (sc_uint<33>)a + (sc_uint<33>)b;
    return tmp.range(31,0);
}

sc_uint<32> hw_int_sub(sc_uint<32> a, sc_uint<32> b)
{
    sc_int<33> tmp = (sc_int<33>)a - (sc_int<33>)b;
    return (sc_uint<32>)tmp.range(31,0);
}

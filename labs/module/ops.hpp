#ifndef OPS_HPP
#define OPS_HPP

#include <systemc.h>

/**
 * ops.hpp:
 *   Possibly leftover or alternative simpler ops, 
 *   e.g. integer-based sub or reduce if you want
 *   or stubs for multi-cycle later.
 */

sc_uint<32> hw_int_add(sc_uint<32> a, sc_uint<32> b);
sc_uint<32> hw_int_sub(sc_uint<32> a, sc_uint<32> b);

#endif

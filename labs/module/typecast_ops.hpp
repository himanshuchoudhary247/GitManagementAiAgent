/**********
 * Author: Abcd at abcd
 * Project: Project name
 * File: typecast_ops.hpp
 * Description: Declares the single-cycle typecast function, preserving signature.
 **********/
#pragma once
#include <systemc.h>

enum NumFormat { FP32, BF16, INT8 };

// Single-cycle type-cast operation, no change in signature
sc_uint<32> typecast_single_cycle(sc_uint<32> input,
                                  NumFormat srcFmt,
                                  NumFormat dstFmt);

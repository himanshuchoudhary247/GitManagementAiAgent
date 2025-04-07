#ifndef TYPECAST_OPS_HPP
#define TYPECAST_OPS_HPP

#include <systemc.h>

enum NumFormat { FP32, BF16, INT8 };

sc_uint<32> typecast_single_cycle(sc_uint<32> input,
                                  NumFormat srcFmt,
                                  NumFormat dstFmt,
                                  bool enable_subnorm,
                                  bool enable_trunc,
                                  bool enable_clamp,
                                  bool enable_except);

#endif

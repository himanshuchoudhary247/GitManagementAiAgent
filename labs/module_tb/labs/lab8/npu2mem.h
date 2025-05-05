#ifndef NPU2MEM_H
#define NPU2MEM_H

struct npu2mem {
    unsigned int address;
    bool is_read;
    unsigned int data;
};

#include <ostream>
inline std::ostream& operator<<(std::ostream& os, const npu2mem& trans) {
    os << "Address=" << trans.address 
       << ", is_read=" << trans.is_read 
       << ", data=0x" << std::hex << trans.data << std::dec;
    return os;
}

#endif // NPU2MEM_H

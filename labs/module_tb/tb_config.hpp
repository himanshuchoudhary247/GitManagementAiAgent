#pragma once
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include "sfr/unique_registers.h"

class tb_config {
public:
    static tb_config& instance() {
        static tb_config inst;
        return inst;
    }
    void load(int argc,char* argv[]) {
        if(loaded_) return;
        loaded_ = true;
        if(argc<2) return;
        std::ifstream in(argv[1]);
        if(!in) { std::cerr<<"[tb_config] cannot open "<<argv[1]<<"\n"; return; }
        std::string line;
        while(std::getline(in,line)) parse_line(line);
    }
    void get_cfg_registers(common_register_map& out) {
        out = reg_;
    }
private:
    tb_config()=default;
    bool loaded_{false};
    common_register_map reg_{};
    static std::string trim(std::string s) {
        auto p = s.find_first_not_of(" \t\""); if(p!=std::string::npos) s.erase(0,p);
        p = s.find_last_not_of(" \t\","); if(p!=std::string::npos) s.erase(p+1);
        return s;
    }
    void parse_line(std::string l) {
        auto c=l.find(':'); if(c==std::string::npos) return;
        auto key=trim(l.substr(0,c)), val=trim(l.substr(c+1));
        if(!val.empty()&&val.back()==',') val.pop_back();
        val=trim(val);
        uint32_t num = (val.rfind("0x",0)==0) ? std::strtoul(val.c_str(),nullptr,16)
                                              : std::strtoul(val.c_str(),nullptr,10);
        #define SET(f) if(key==#f){ reg_.f = num; return; }
        SET(option_tensor_size_size_m)
        SET(option_tensor_size_size_k)
        SET(option_tensor_size_size_n)
        SET(addr_tensor_matrix_c_base_matrix_c_base_addr)
        SET(addr_tensor_matrix_c_stride_matrix_c_row_stride)
        SET(addr_tensor_matrix_c_stride_matrix_c_col_stride)
        #undef SET
    }
};

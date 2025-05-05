#ifndef TB_CONFIG_HPP
#define TB_CONFIG_HPP
/**********************************************************************
* tb_config.hpp
*
* Very small, header‑only helper that lets any test‑bench grab a single
* `common_register_map` populated from an optional JSON‑like text file.
*
*  – Usage -----------------------------------------------------------
*      int sc_main(int argc,char* argv[])
*      {
*          tb_config::instance().load(argc,argv);     // once in top‑level
*          ...
*      }
*
*      common_register_map cfg;
*      tb_config::instance().get_cfg_registers(cfg);  // anywhere
*
*  – File format (trivial) -------------------------------------------
*      { "option_tensor_size_size_m": 4,
*        "addr_tensor_matrix_c_stride_matrix_c_row_stride": 0x800 }
*    Keys must exactly match the canonical field names in
*    `sfr/unique_registers.h`.  Numbers may be decimal or 0x‑hex.
*********************************************************************/
#include <fstream>
#include <sstream>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <iostream>

#include "sfr/unique_registers.h"   // brings in common_register_map

class tb_config
{
public:
    /*---------------------------------------------------------------
     * Singleton accessor
     *-------------------------------------------------------------*/
    static tb_config& instance()
    {
        static tb_config inst;
        return inst;
    }

    /*---------------------------------------------------------------
     * Load once – pass original argc/argv from sc_main.
     * If argv[1] is a filename, we parse it; otherwise defaults stay.
     *-------------------------------------------------------------*/
    void load(int argc, char* argv[])
    {
        if(loaded_) return;             // guard against double‑load
        loaded_ = true;

        if(argc < 2) return;            // no file, keep defaults

        std::ifstream in(argv[1]);
        if(!in) {
            std::cerr << "[tb_config] Warning: cannot open config file "
                      << argv[1] << ", using defaults.\n";
            return;
        }
        std::string line;
        while(std::getline(in,line))
            parse_line(line);
    }

    /*---------------------------------------------------------------
     * Copy current register map to caller
     *-------------------------------------------------------------*/
    void get_cfg_registers(common_register_map& out) const
    {
        out = reg_;
    }

private:
    tb_config() = default;                      // private ctor
    tb_config(const tb_config&)            = delete;
    tb_config& operator=(const tb_config&) = delete;

    /*---------------------------------------------------------------
     * Very small “key : value” parser (JSON‑lite).
     *-------------------------------------------------------------*/
    void parse_line(std::string l)
    {
        auto colon = l.find(':');
        if(colon == std::string::npos) return;

        std::string key = trim(l.substr(0,colon));
        std::string val = trim(l.substr(colon+1));

        // strip trailing ',' or comments
        if(!val.empty() && (val.back()==',' || val.back()=='/'))
            val.pop_back();
        val = trim(val);

        uint32_t num = (val.rfind("0x",0)==0) ? std::strtoul(val.c_str(),nullptr,16)
                                              : std::strtoul(val.c_str(),nullptr,10);

#define SET_FIELD(field)               \
        if(key == #field){ reg_.field = num; return; }

        /* ---- list all fields you want to override ---------------- */
        SET_FIELD(option_tensor_size_size_m)
        SET_FIELD(option_tensor_size_size_k)
        SET_FIELD(option_tensor_size_size_n)

        SET_FIELD(addr_tensor_matrix_c_base_matrix_c_base_addr)
        SET_FIELD(addr_tensor_matrix_c_stride_matrix_c_row_stride)
        SET_FIELD(addr_tensor_matrix_c_stride_matrix_c_col_stride)
        /* add more fields here if needed */

#undef  SET_FIELD
    }

    static std::string trim(std::string s)
    {
        auto notspace = [](int ch){ return !std::isspace(ch); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), notspace));
        s.erase(std::find_if(s.rbegin(), s.rend(), notspace).base(), s.end());
        s.erase(std::remove(s.begin(), s.end(), '"'), s.end());
        return s;
    }

private:
    bool loaded_{false};
    common_register_map reg_{};         // default‑initialised register map
};

#endif /* TB_CONFIG_HPP */

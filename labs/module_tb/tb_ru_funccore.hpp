#pragma once
#include "npucommon.hpp"
#include "npudefine.hpp"

#include "ports/npuc2mmu.hpp"
#include "ports/mmu2npuc.hpp"
#include "ports/mmu2ru.hpp"
#include "ports/ru2tcm.hpp"
#include "ports/ru2mlsu.hpp"
#include "sfr/unique_registers.h"
#include "tb_config.hpp"

#include <queue>

#define NUM_PORTS 16
#define NUM_LINES  4

class tb_ru_funccore : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(tb_ru_funccore);
    explicit tb_ru_funccore(sc_core::sc_module_name name);

    sc_in<bool>                        clk;
    sc_in<bool>                        reset;

    sc_fifo_out<npuc2mmu_PTR>          o_npuc2mmu;
    sc_fifo_out<mmu2npuc_PTR>          o_mmu2npuc;

    sc_vector< sc_fifo_out<mmu2ru_PTR> > o_mmu2ru;
    sc_vector< sc_fifo_in <ru2tcm_PTR> > i_ru2tcm;
    sc_vector< sc_fifo_in <ru2mlsu_PTR>> i_ru2mlsu;

    sc_fifo_out<sfr_PTR>               o_reg_map;

    void set_id(int id){ id_ = id; }
    void set_args(int argc,char*argv[]){argc_=argc;argv_=argv;}

private:
    void driver_thread();
    void monitor_tcm();
    void monitor_mlsu();
    void fill_golden();

    mmu2ru_PTR make_pkt(uint32_t r,uint32_t c,bool last=false);
    sfr_PTR    make_sfr(bool fused);

    common_register_map                cfg_;
    std::queue<uint64_t>               golden_tcm_, golden_mlsu_;
    int                                id_{0}, argc_{0};
    char**                             argv_{nullptr};
};

#pragma once
#include "npucommon.hpp"
#include "npudefine.hpp"
#include "sfr/unique_registers.h"
#include "tb_config.hpp"
#include <queue>
#include <tuple>

constexpr int PORTS  = 16;
constexpr int LINES  = 4;

class tb_ru_funccore : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(tb_ru_funccore);
    explicit tb_ru_funccore(sc_core::sc_module_name);

    sc_in <bool>              clk;
    sc_in <bool>              reset;
    sc_fifo_out<npuc2mmu_PTR> o_npuc2mmu;
    sc_fifo_out<mmu2npuc_PTR> o_mmu2npuc;
    sc_vector< sc_fifo_out<mu2ru_PTR> >  o_mmu2ru;
    sc_vector< sc_fifo_in <ru2tcm_PTR> > i_ru2tcm;
    sc_vector< sc_fifo_in <ru2mlsu_PTR> >i_ru2mlsu;
    sc_fifo_out<sfr_PTR>      o_reg_map;

private:
    void handler();
    void resp_tcm();
    void resp_mlsu();
    void receive_done();

    void run_testcase(const common_register_map&);
    mu2ru_PTR make_pkt(bool last);

    std::queue<uint16_t> gold_tcm_;
    std::size_t          gold_mlsu_=0;
    bool done_flag=false;
};
#pragma once
#include "npucommon.hpp"
#include "npudefine.hpp"
#include "mmu2ru.hpp"
#include "ru2tcm.hpp"
#include "ru2mlsu.hpp"
#include "sfr/unique_registers.h"   // common_register_map + sfr_PTR
#include "tb_config.hpp"
#include <queue>

class tb_ru_funccore : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(tb_ru_funccore);
    explicit tb_ru_funccore(sc_core::sc_module_name);

    sc_in <bool>              clk, reset;
    sc_fifo_out<npuc2mmu_PTR> o_npuc2mmu;
    sc_fifo_out<mmu2npuc_PTR> o_mmu2npuc;
    sc_vector< sc_fifo_out<mmu2ru_PTR> >  o_mmu2ru;
    sc_vector< sc_fifo_in <ru2tcm_PTR> >  i_ru2tcm;
    sc_vector< sc_fifo_in <ru2mlsu_PTR> > i_ru2mlsu;
    sc_fifo_out<sfr_PTR>      o_reg_map;

private:
    void handler();           // SC_THREAD entry
    void resp_tcm();
    void resp_mlsu();

    void run_testcase(const common_register_map&);
    mmu2ru_PTR make_pkt(bool last);
    uint16_t   addr16(uint32_t r,uint32_t c);

    std::queue<uint16_t> gold_tcm_;
    std::size_t          gold_mlsu_{0};
};

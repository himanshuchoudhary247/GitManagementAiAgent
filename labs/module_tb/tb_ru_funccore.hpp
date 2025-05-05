#pragma once
#include "npucommon.hpp"
#include "npudefine.hpp"
#include "sfr/unique_registers.h"      // common_register_map
#include "tb_config.hpp"

#include <queue>
#include <tuple>

#define NUM_PORTS 16
#define NUM_LINES 4
#define DEBUG_LOG_SEVERITY_TB 1

class tb_ru_funccore : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(tb_ru_funccore);
    explicit tb_ru_funccore(sc_core::sc_module_name);

    sc_in <bool>              clk;
    sc_in <bool>              reset;
    sc_fifo_out<npuc2ru_PTR>  o_npuc2ru;
    sc_fifo_in <ru2npuc_PTR>  i_ru2npuc;
    sc_vector< sc_fifo_out<mmu2ru_PTR> >  o_mmu2ru;
    sc_vector< sc_fifo_in <ru2tcm_PTR> >  i_ru2tcm;
    sc_vector< sc_fifo_in <ru2mlsu_PTR> > i_ru2mlsu;
    sc_fifo_out<sfr_PTR>      o_reg_map;

private:
    void handlerTestMain();
    void resp_tcm();
    void resp_mlsu();
    void receive_done_micro();   /* handles ru2npuc.done */

    void golden_gen(const common_register_map&);
    void send_phase(bool fused,const common_register_map&);

    /* queues & state */
    std::queue<std::tuple<uint64_t,bool>> gold_;
    bool done_micro{false};
};

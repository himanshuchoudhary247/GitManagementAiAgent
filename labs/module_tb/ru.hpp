#pragma once
#include "npucommon.hpp"
#include "npudefine.hpp"
#include "ru_funccore.hpp"

class ru: public sc_core::sc_module {
public:
SC_HAS_PROCESS(ru);
ru(sc_core::sc_module_name name, int id);
    sc_in<bool> clk;
    sc_in<bool> reset;
    sc_fifo_inenpuc2mmu_PTR > i_npuc2mu;
    sc_fifo_incmmu2npuc PTR > immu2npuc;
    sc_vector sc_fifo_ine mmu2ru PTR > > immu2ru;
    sc_vector< sc_fifo_out< ru2tcm_PTR > > o_ru2tcm;
    sc_vector sc_fifo_out< ru2mlsu PTR > > o_ru2mlsu;
    sc_fifo_int sfr_PTR > i_reg_map;

void set_id(int set_id);
private:
    ru_funccore funccore;
        int id;
};
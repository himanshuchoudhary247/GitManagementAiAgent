#pragma once
#include "systemc.h"
#include <memory>
#include "npudefine.hpp"
#include "mmu2ru.hpp"
#include "ru2tcm.hpp"
#include "ru2mlsu.hpp"

using npuc2mmu_PTR = std::shared_ptr<npuc2mmu>;
using mmu2npuc_PTR = std::shared_ptr<mmu2npuc>;
using sfr_PTR      = std::shared_ptr<uint32_t>;

class ru : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(ru);
    ru(sc_core::sc_module_name name, int id);

    sc_in<bool>                   clk, reset;
    sc_fifo_in<npuc2mmu_PTR>      i_npuc2mu;
    sc_fifo_in<mmu2npuc_PTR>      i_mmu2npuc;
    sc_vector< sc_fifo_in<mmu2ru_PTR> >  i_mmu2ru;
    sc_vector< sc_fifo_out<ru2tcm_PTR> > o_ru2tcm;
    sc_vector< sc_fifo_out<ru2mlsu_PTR> >o_ru2mlsu;
    sc_fifo_in<sfr_PTR>           i_reg_map;

    void set_id(int id);

private:
    void main_thread();

    int id;
};

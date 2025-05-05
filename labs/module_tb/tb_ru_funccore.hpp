#pragma once
/*********************************************************************
*  tb_ru_funccore.hpp  –  Routing‑Unit TB (C++11, no new _PTR types)
*********************************************************************/
#include "npucommon.hpp"
#include "npudefine.hpp"
#include "sfr/unique_registers.h"
#include "tb_config.hpp"

#include <queue>
#include <utility>   // std::pair

/*───────────────── fallback packet defs (guarded) ─────────────────*/
#ifndef MMU2RU_PTR
struct mmu2ru_t { uint32_t row, col, C_data[16]; bool done; };
typedef std::shared_ptr<mmu2ru_t> mmu2ru_PTR;
#endif
#ifndef RU2TCM_PTR
struct ru2tcm_t { uint16_t address[4]; sc_bv<512> data[4]; bool done; };
typedef std::shared_ptr<ru2tcm_t> ru2tcm_PTR;
#endif
#ifndef RU2MLSU_PTR
struct ru2mlsu_t { sc_bv<512> data[4]; bool done; };
typedef std::shared_ptr<ru2mlsu_t> ru2mlsu_PTR;
#endif
/*──────────────────────────────────────────────────────────────────*/

#define NUM_PORTS 16
#define NUM_LINES 4
#define DBG_TB    1

class tb_ru_funccore : public sc_core::sc_module
{
public:
    SC_HAS_PROCESS(tb_ru_funccore);
    explicit tb_ru_funccore(sc_core::sc_module_name name);

    sc_in<bool> clk;
    sc_in<bool> reset;

    sc_vector< sc_fifo_out<mmu2ru_PTR> >    o_mmu2ru;
    sc_vector< sc_fifo_in <ru2tcm_PTR> >    i_ru2tcm;
    sc_vector< sc_fifo_in <ru2mlsu_PTR> >   i_ru2mlsu;

    sc_fifo_out<sfr_PTR>                    o_reg_map;

private:
    /* threads */
    void driver_thread();
    void monitor_tcm();
    void monitor_mlsu();

    /* helpers */
    void build_golden(const common_register_map& cfg);
    mmu2ru_PTR make_pkt(uint32_t r,uint32_t c,bool last=false);
    sfr_PTR    make_sfr(bool fused);

    /* state */
    std::queue< std::pair<uint64_t,bool> > golden_;   // <addr,fused?>
};

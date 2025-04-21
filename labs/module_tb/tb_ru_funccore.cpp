/*********************
* Author: ABC at abc
* Project: Pname
* File: tb_ru_funccore.cpp
* Description: Provides simple stimuli to exercise RU routing for both  
*              fused and non‑fused modes.  
*********************/
#define DEBUG_LOG_SEVERITY_TB 0
#include "tb_ru_funccore.hpp"
#include <cstdlib>
#include <cstdint>
#include <cstring>

SC_HAS_PROCESS(tb_ru_funccore);

/*----------------------------------------------------------------------*/
tb_ru_funccore::tb_ru_funccore(sc_core::sc_module_name name)
    : sc_module(name)
    , clk("clk")
    , reset("reset")
    , o_npuc2mmu("o_npuc2mmu")
    , o_mmu2npuc("o_mmu2npuc")
    , o_mmu2ru("o_mmu2ru", 16)
    , i_ru2tcm("i_ru2tcm", 4)
    , i_ru2mlsu("i_ru2mlsu", 4)
    , o_reg_map("o_reg_map")
{
    SC_THREAD(gen_reset);
    sensitive << clk.pos();

    SC_THREAD(stimulus_proc);
    sensitive << clk.pos();
}

/*----------------------------------------------------------------------*/
void tb_ru_funccore::gen_reset()
{
    /* Generate a short active‑high reset pulse */
    reset.write(true);
    wait(5);               /* 5 cycles */
    reset.write(false);
    wait();                /* remain low indefinitely */
}

/*----------------------------------------------------------------------*/
static inline sfr_PTR make_sfr(std::uint32_t value)
{
    auto* v = new std::uint32_t;
    *v = value;
    return reinterpret_cast<sfr_PTR>(v);
}

static inline mmu2ru_PTR make_dummy_result(std::uint32_t id)
{
    /* Allocate 4‑byte dummy payload—content is irrelevant for routing */
    auto* p = new std::uint32_t;
    *p = 0xDEAD0000 | (id & 0xFFFF);
    return reinterpret_cast<mmu2ru_PTR>(p);
}

/*----------------------------------------------------------------------*/
void tb_ru_funccore::stimulus_proc()
{
    /* Wait until reset de‑asserts */
    while (reset.read()) wait();

    /*-------------------------------------------------------*/
    /* Test‑1:  NOT FUSED  (fused=0) */
    constexpr std::uint32_t MODE_NOT_FUSED =
        (0u << 20) |      /* fused=0 */
        1u;               /* NUMBER_FORMAT=FP16 */

    o_reg_map.write(make_sfr(MODE_NOT_FUSED));
    wait();

    /* Send eight dummy results down different MMU→RU lanes */
    for (unsigned i = 0; i < 8; ++i) {
        o_mmu2ru.at(i).write(make_dummy_result(i));
    }

    wait(20);   /* let RU drain */

    /*-------------------------------------------------------*/
    /* Test‑2:  FUSED (fused=1, FP8) */
    constexpr std::uint32_t MODE_FUSED =
        (1u << 20) |      /* fused=1 */
        3u;               /* NUMBER_FORMAT=FP8 */

    o_reg_map.write(make_sfr(MODE_FUSED));
    wait();

    for (unsigned i = 0; i < 8; ++i) {
        o_mmu2ru.at(i).write(make_dummy_result(i + 100));
    }

    wait(20);

#if DEBUG_LOG_SEVERITY_TB > 0
    std::cout << "\n[TB] Completed both scenarios – stopping simulation.\n";
#endif
    sc_stop();
}

/*----------------------------------------------------------------------*/
void tb_ru_funccore::set_id(int id)         { id_   = id; }
void tb_ru_funccore::set_args(int a, char** v) { argc_ = a; argv_ = v; }

#include "tb_ru_funccore.hpp"
#include <cassert>
#include <iostream>

/* helper: build 16‑bit bank+offset address */
uint16_t tb_ru_funccore::addr16(uint32_t r,uint32_t c)
{
    uint16_t bank   = ((r & 0xF) << 1) | (c >> 1);
    uint16_t offset = (r >> 4) * 64 + ((c & 1) ? 32 : 0);
    return uint16_t((bank << 11) | offset);
}

/* build one packet for MMU→RU */
mmu2ru_PTR tb_ru_funccore::make_pkt(bool last)
{
    mmu2ru_PTR p(new mmu2ru);
    for (int i = 0; i < 16; ++i) p->C_data[i] = sc_dt::sc_bv<8>(i);
    p->done = last;
    return p;
}

/* ── ctor ──────────────────────────────────────────────────────────────*/
tb_ru_funccore::tb_ru_funccore(sc_core::sc_module_name n)
: sc_module(n)
, clk("clk"), reset("reset")
, o_npuc2mmu("o_npuc2mmu"), o_mmu2npuc("o_mmu2npuc")
, o_mmu2ru("o_mmu2ru",16),  i_ru2tcm("i_ru2tcm",4)
, i_ru2mlsu("i_ru2mlsu",4), o_reg_map("o_reg_map")
{
    SC_THREAD(handler); sensitive << clk.pos();

    SC_THREAD(resp_tcm);
    for (int i = 0; i < 4; ++i)
        sensitive << i_ru2tcm[i].data_written_event();

    SC_THREAD(resp_mlsu);
    for (int i = 0; i < 4; ++i)
        sensitive << i_ru2mlsu[i].data_written_event();
}

/* ── main control thread (reads config & drives testcase) ─────────────*/
void tb_ru_funccore::handler()
{
    common_register_map cfg;
    tb_config::instance().get_cfg_registers(cfg);   // read JSON

    /* push MODE_TENSOR (FUSED_OPERATION = 0) */
    o_reg_map.write(std::make_shared<uint32_t>(0u));

    run_testcase(cfg);

    /* stop simulation when queues empty & checks pass */
    sc_core::sc_stop();
}

/* ── run single testcase: non‑fused then fused ────────────────────────*/
void tb_ru_funccore::run_testcase(const common_register_map& cfg)
{
    uint32_t M = cfg.option_tensor_size_size_m * 8;
    uint32_t N = cfg.option_tensor_size_size_n * 32;

    /* --- phase‑1 : results to TCM --- */
    for (uint32_t r=0; r<M; ++r)
        for (uint32_t c=0; c<N; ++c) {
            bool last = (r==M-1) && (c==N-1);
            o_mmu2ru[r & 0xF].write(make_pkt(last));
            gold_tcm_.push(addr16(r,c));
        }

    wait(500, sc_core::SC_NS);   // allow pipeline flush

    /* --- phase‑2 : set fused bit → MLSU --- */
    o_reg_map.write(std::make_shared<uint32_t>(1u << 20));

    for (uint32_t r=0; r<M; ++r)
        for (uint32_t c=0; c<N; ++c) {
            bool last = (r==M-1) && (c==N-1);
            o_mmu2ru[(r+1)&0xF].write(make_pkt(last));
            ++gold_mlsu_;
        }
}

/* ── monitor RU→TCM ---------------------------------------------------*/
void tb_ru_funccore::resp_tcm()
{
    while (true) {
        for (int l = 0; l < 4; ++l) {
            if (!i_ru2tcm[l].num_available()) continue;
            ru2tcm_PTR p = i_ru2tcm[l].read();
            assert(!gold_tcm_.empty());
            uint16_t exp = gold_tcm_.front(); gold_tcm_.pop();
            assert(p->address.to_uint() == exp && "TCM addr mismatch");
        }
        wait(sc_core::SC_ZERO_TIME);
    }
}

/* ── monitor RU→MLSU --------------------------------------------------*/
void tb_ru_funccore::resp_mlsu()
{
    while (true) {
        for (int l = 0; l < 4; ++l) {
            if (!i_ru2mlsu[l].num_available()) continue;
            i_ru2mlsu[l].read();                 // content not checked
            assert(gold_mlsu_ > 0); --gold_mlsu_;
        }
        wait(sc_core::SC_ZERO_TIME);
    }
}

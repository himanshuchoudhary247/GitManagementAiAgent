/*********************
* Author: ABC at abc
* Project: Pname
* File: tb_top_ru.cpp
* Description: Wiring harness that connects the Design‑Under‑Test
*              (RU + ru_funccore) to the stimulus / scoreboard
*              module tb_ru_funccore.  All FIFOs are one‑deep to
*              keep the test‑bench lightweight.
*********************/
#include "tb_top_ru.hpp"

tb_top_ru::tb_top_ru(sc_core::sc_module_name name, int argc, char* argv[])
    : sc_module(name)
    , clk("clk")
    , reset("reset")
    , dut("dut", 0)          /* RU instance with id 0 */
    , tb("tb")               /* stimulus / checker     */
    , npuc2mmu_fifo("npuc2mmu_fifo", 1)
    , mmu2npuc_fifo("mmu2npuc_fifo", 1)
    , reg_map_fifo("reg_map_fifo", 1)
{
    /* Forward argc/argv to tb if it wants to parse CLI flags */
    tb.set_args(argc, argv);
    init();
}

/*------------------------------------------------------------*/
void tb_top_ru::init()
{
    /* Dynamically allocate lane FIFOs so they can be bound by reference */
    for (auto& f : mmu2ru_fifo)   f = new sc_fifo<mmu2ru_PTR>(1);
    for (auto& f : ru2tcm_fifo)   f = new sc_fifo<ru2tcm_PTR>(1);
    for (auto& f : ru2mlsu_fifo)  f = new sc_fifo<ru2mlsu_PTR>(1);

    /* Bind common clock / reset */
    dut.clk(clk);   tb.clk(clk);
    dut.reset(reset); tb.reset(reset);

    /* ID & SFR map */
    dut.set_id(0);
    dut.i_reg_map(reg_map_fifo);
    tb.o_reg_map(reg_map_fifo);

    /* NPU ⇄ MMU handshake paths (unused in this specific TB but wired) */
    dut.i_npuc2mu(npuc2mmu_fifo);
    tb.o_npuc2mmu(npuc2mmu_fifo);
    dut.i_mmu2npuc(mmu2npuc_fifo);
    tb.o_mmu2npuc(mmu2npuc_fifo);

    /* MMU → RU result lanes */
    for (int i = 0; i < 16; ++i) {
        dut.i_mmu2ru.at(i)(*mmu2ru_fifo.at(i));
        tb.o_mmu2ru.at(i)(*mmu2ru_fifo.at(i));
    }

    /* RU → TCM & RU → MLSU output ports */
    for (int i = 0; i < 4; ++i) {
        dut.o_ru2tcm.at(i)(*ru2tcm_fifo.at(i));
        tb.i_ru2tcm.at(i)(*ru2tcm_fifo.at(i));

        dut.o_ru2mlsu.at(i)(*ru2mlsu_fifo.at(i));
        tb.i_ru2mlsu.at(i)(*ru2mlsu_fifo.at(i));
    }
}

/*------------------------------------------------------------*/
/* Nothing special to do after elaboration for this simple TB. */
void tb_top_ru::end_of_elaboration() {}
/*------------------------------------------------------------*/
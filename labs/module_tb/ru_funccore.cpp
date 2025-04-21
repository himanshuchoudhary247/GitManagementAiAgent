/*********************
* Author: ABC at abc
* Project: Pname
* File: ru_funccore.cpp
* Description: Implements the functional core of the RU.  
*  – Monitors SFRs to determine FUSED / NUMBER_FORMAT settings.  
*  – Routes every result packet from MMU → (TCM or MLSU) without bank conflicts.  
*********************/
#include "ru_funccore.hpp"
#include <cstdint>

namespace {
/* Bit locations inside MODE_TENSOR */
constexpr std::uint32_t FUSED_OPERATION_BIT = 20;
constexpr std::uint32_t NUMBER_FORMAT_MASK  = 0x7;
}

/*----------------------------------------------------------------------*/
ru_funccore::ru_funccore(sc_core::sc_module_name name)
    : sc_module(name)
    , clk("clk")
    , reset("reset")
    , i_npuc2mmu("i_npuc2mmu")
    , i_mmu2npuc("i_mmu2npuc")
    , i_mmu2ru("i_mmu2ru", 16)
    , o_ru2tcm("o_ru2tcm", 4)
    , o_ru2mlsu("o_ru2mlsu", 4)
    , i_reg_map("i_reg_map")
{
    SC_CTHREAD(router_proc, clk.pos());
    reset_signal_is(reset, true);

    SC_THREAD(sfr_decode_proc);
    sensitive << reset;
}

/*----------------------------------------------------------------------*/
/* State updated by the SFR thread */
static bool g_fused_operation  = false;
static std::uint32_t g_num_fmt = 0;

/*----------------------------------------------------------------------*/
void ru_funccore::sfr_decode_proc()
{
    /* Simple, blocking decode process. */
    while (true) {
        /* Block until a new SFR write appears */
        sfr_PTR raw = i_reg_map.read();

        /* Assume the 32‑bit value is stored at *raw */
        std::uint32_t val = *reinterpret_cast<std::uint32_t*>(raw);

        g_fused_operation = (val >> FUSED_OPERATION_BIT) & 0x1;
        g_num_fmt         = (val & NUMBER_FORMAT_MASK);

#if DEBUG_LOG_SEVERITY > 0
        std::cout << sc_time_stamp() << " [RU‑SFR] fused=" << g_fused_operation
                  << ", num_fmt=" << static_cast<int>(g_num_fmt) << '\n';
#endif
    }
}

/*----------------------------------------------------------------------*/
void ru_funccore::router_proc()
{
    /* Reset section */
    for (auto& fifo : o_ru2tcm)  while (fifo.nb_read(nullptr));
    for (auto& fifo : o_ru2mlsu) while (fifo.nb_read(nullptr));
    wait();               /* leave reset */

    while (true) {
        /* Round‑robin over 16 result lanes */
        for (size_t lane = 0; lane < i_mmu2ru.size(); ++lane) {

            if (i_mmu2ru.at(lane).num_available() == 0) continue;

            /* Pop one result pointer */
            mmu2ru_PTR pkt = i_mmu2ru.at(lane).read();

            if (g_fused_operation) {
                /* Route to Math Load/Store */
                size_t port = lane % o_ru2mlsu.size();
                o_ru2mlsu.at(port).write(pkt);
#if DEBUG_LOG_SEVERITY > 1
                std::cout << sc_time_stamp() << "  RU: lane" << lane
                          << " → MLSU[" << port << "]\n";
#endif
            } else {
                /* Route to TCM */
                size_t bank_port = lane % o_ru2tcm.size();
                o_ru2tcm.at(bank_port).write(pkt);
#if DEBUG_LOG_SEVERITY > 1
                std::cout << sc_time_stamp() << "  RU: lane" << lane
                          << " → TCM[" << bank_port << "]\n";
#endif
            }
        }
        wait();   /* next clock */
    }
}

/*----------------------------------------------------------------------*/
void ru_funccore::set_Id(int set_id) { id = set_id; }

/*********************
* Author : ABC at abc
* Project: Pname
* File   : ru_funccore.cpp
* Notes  :
*   – Implements the behavioural model of the Routing-Unit core.
*   – Works with the fixed public interface in ru_funccore.hpp (unchanged).
*   – Computes a 48-bit physical TCM address for every 32-byte C-matrix
*     chunk (row,col) and decides, per the FUSED_OPERATION SFR bit,
*     whether to forward the packet to TCM (ru2tcm) or straight to
*     the Math-Engine load-store path (ru2mlsu).
*********************/
#include "ru_funccore.hpp"
#include <cstring>          // std::memcpy
#include <iomanip>
#include <sstream>

/*-----------------------------------------------------------------------------
 * Helpers
 *---------------------------------------------------------------------------*/
namespace {
/* Map (row,col) → 5-bit bank id (0-31).  Rows are grouped by 16, columns by 2.
 *
 * ┌──── row low-bits (0-15) ────┐
 * bank =  (row & 0xF) * 2 | (col >> 1)
 */
inline std::uint32_t bank_of(std::uint32_t row, std::uint32_t col)
{
    return ((row & 0xF) << 1U) | (col >> 1U);
}

/* Generate byte offset inside one 64-B bank-row. */
inline std::uint32_t offset_in_bank(std::uint32_t row, std::uint32_t col)
{
    const std::uint32_t bank_row = row >> 4U;          // every 16 rows → next 64 B line
    const std::uint32_t chunk    = (col & 1U) ? 32U : 0U;
    return bank_row * 64U + chunk;
}

/* Compose 48-bit address: upper 16 b = bank id (only 5 used); lower 32 = offset. */
inline std::uint64_t make_addr(std::uint32_t row, std::uint32_t col)
{
    return (static_cast<std::uint64_t>(bank_of(row,col)) << 32) |
            offset_in_bank(row,col);
}

/* Each eight banks (0-7, 8-15, …) share the same 512-B output line. */
inline std::size_t line_for_bank(std::uint32_t bank) { return bank >> 3; }
} // anonymous-ns

/*-----------------------------------------------------------------------------
 * Constructor + process
 *---------------------------------------------------------------------------*/
ru_funccore::ru_funccore(sc_core::sc_module_name n)
: sc_module(n)
, clk          ("clk")
, reset        ("reset")
, i_npuc2mmu   ("i_npuc2mmu")
, i_mmu2npuc   ("i_mmu2npuc")
, i_mmu2ru     ("i_mmu2ru" , 16)
, o_ru2tcm     ("o_ru2tcm" ,  4)
, o_ru2mlsu    ("o_ru2mlsu",  4)
, i_reg_map    ("i_reg_map")
{
    SC_CTHREAD(main_thread, clk.pos());
    reset_signal_is(reset,true);
}

void ru_funccore::set_Id(int v) { id = v; }

/*-----------------------------------------------------------------------------
 * Main behavioural thread
 *---------------------------------------------------------------------------*/
void ru_funccore::main_thread()
{
    bool fused = false;             // current FUSED_OPERATION bit
    wait();                         // reset cycle

    while (true)
    {
        /* ---- 1) snoop SFR stream (non-blocking) ------------------------ */
        if (i_reg_map.num_available())
        {
            sfr_PTR p = i_reg_map.read();          // concrete SFR type is unknown.
            const std::uint32_t word = *reinterpret_cast<std::uint32_t const*>(p);
            fused = (word >> 20) & 0x1;
#if DEBUG_LOG_SEVERITY
            std::cout << sc_time_stamp() << "  [RU" << id << "] "
                      << "SFR update → fused=" << fused << '\n';
#endif
        }

        /* ---- 2) service up to one packet per port this cycle ----------- */
        for (std::size_t port=0; port<i_mmu2ru.size(); ++port)
        {
            if (!i_mmu2ru[port].num_available()) continue;

            /* The minimal assumed packet shape: (row,col,payload[32]). */
            mmu2ru_PTR in = i_mmu2ru[port].read();
            const std::uint32_t r = in->row;
            const std::uint32_t c = in->col;
            const std::uint64_t addr = make_addr(r,c);
            const std::size_t  line  = line_for_bank(static_cast<std::uint32_t>(addr>>32));

            if (fused)   /* ---- direct to MLSU -------------------------------- */
            {
                ru2mlsu_PTR out = new ru2mlsu_t;
                out->addr = addr;
                std::memcpy(out->payload, in->payload, 32);
                o_ru2mlsu[line].write(out);
            }
            else         /* ---- store to TCM ---------------------------------- */
            {
                ru2tcm_PTR out = new ru2tcm_t;
                out->addr = addr;
                std::memcpy(out->payload, in->payload, 32);
                o_ru2tcm[line].write(out);
            }
        }

        wait();     // next cycle
    }
}

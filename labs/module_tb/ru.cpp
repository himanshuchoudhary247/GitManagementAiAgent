// /*********************
// * Author: ABC at abc
// * Project: Pname
// * File: ru.cpp
// * Description: Top‑level wrapper for the Routing Unit (RU).  
// *              Instantiates ru_funccore, binds all ports, and forwards the ID.
// *********************/
// #include "ru.hpp"

// ru::ru(sc_core::sc_module_name name, int id_)
//     : sc_module(name)
//     , clk("clk")
//     , reset("reset")
//     , i_npuc2mu("i_npuc2mu")
//     , i_mmu2npuc("i_mmu2npuc")
//     , i_mmu2ru("i_mmu2ru", 16)
//     , o_ru2tcm("o_ru2tcm", 4)
//     , o_ru2mlsu("o_ru2mlsu", 4)
//     , i_reg_map("i_reg_map")
//     , funccore("funccore")
// {
//     /* Port binding */
//     funccore.clk(clk);
//     funccore.reset(reset);

//     funccore.i_npuc2mmu(i_npuc2mu);
//     funccore.i_mmu2npuc(i_mmu2npuc);
//     funccore.i_reg_map(i_reg_map);

//     for (size_t i = 0; i < i_mmu2ru.size(); ++i)
//         funccore.i_mmu2ru.at(i)(i_mmu2ru.at(i));

//     for (size_t i = 0; i < o_ru2tcm.size(); ++i)
//         funccore.o_ru2tcm.at(i)(o_ru2tcm.at(i));

//     for (size_t i = 0; i < o_ru2mlsu.size(); ++i)
//         funccore.o_ru2mlsu.at(i)(o_ru2mlsu.at(i));

//     funccore.set_Id(id_);
//     id = id_;
// }

// void ru::set_id(int set_id)
// {
//     id = set_id;
//     funccore.set_Id(set_id);
// }
/*********************
* Author : ABC at abc
* Project: Pname
* File   : ru.cpp
* Purpose:
*   Thin wrapper that instantiates `ru_funccore` and connects all
*   SystemC FIFO ports declared in ru.hpp.  Nothing fancy – the real
*   routing logic already lives in ru_funccore.cpp.
*********************/
#include "ru.hpp"

ru::ru(sc_core::sc_module_name name, int id_)
: sc_module (name)
, clk       ("clk")
, reset     ("reset")
, i_npuc2mu ("i_npuc2mu")
, i_mmu2npuc("i_mmu2npuc")
, i_mmu2ru  ("i_mmu2ru" , 16)
, o_ru2tcm  ("o_ru2tcm" ,  4)
, o_ru2mlsu ("o_ru2mlsu",  4)
, i_reg_map ("i_reg_map")
, funccore  ("funccore")
, id(id_)
{
    /*──── bind clock / reset ───────────────────────────────────────────*/
    funccore.clk  (clk);
    funccore.reset(reset);

    /*──── upstream control packets (NPuC↔MMU) – not used in demo ───────*/
    funccore.i_npuc2mmu(i_npuc2mu);
    funccore.i_mmu2npuc(i_mmu2npuc);

    /*──── SFR stream ───────────────────────────────────────────────────*/
    funccore.i_reg_map(i_reg_map);

    /*──── 16 × MMU→RU data ports ───────────────────────────────────────*/
    for (int p = 0; p < 16; ++p)
        funccore.i_mmu2ru [p]( i_mmu2ru [p] );

    /*──── 4 × RU→TCM   and 4 × RU→MLSU outputs ─────────────────────────*/
    for (int l = 0; l < 4; ++l) {
        funccore.o_ru2tcm [l]( o_ru2tcm [l] );
        funccore.o_ru2mlsu[l]( o_ru2mlsu[l] );
    }

    /*──── propagate instance-ID to the core ────────────────────────────*/
    funccore.set_Id(id);
}

/* simple setter (also forwards to funccore) */
void ru::set_id(int set_id)
{
    id = set_id;
    funccore.set_Id(set_id);
}

/* Optional helper so a top-level can instantiate by symbol name */
void instantiate_RU()
{
    static ru i_demo_ru("demo_ru", 0);
}

/*********************
* Routing Unit wrapper
*********************/
#include "ru.hpp"

ru::ru(sc_core::sc_module_name name, int id_)
    : sc_module(name)
    , clk       ("clk")
    , reset     ("reset")
    , i_npuc2mu ("i_npuc2mu")
    , i_mmu2npuc("i_mmu2npuc")
    , i_mmu2ru  ("i_mmu2ru" , 16)
    , o_ru2tcm  ("o_ru2tcm" , 4)
    , o_ru2mlsu ("o_ru2mlsu", 4)
    , i_reg_map ("i_reg_map")
    , funccore  ("funccore")
{
    /* port binding identical to MALU style */
    funccore.clk(clk);
    funccore.reset(reset);

    funccore.i_npuc2mmu(i_npuc2mu);
    funccore.i_mmu2npuc(i_mmu2npuc);
    funccore.i_reg_map(i_reg_map);

    for (size_t i = 0; i < i_mmu2ru.size(); ++i)
        funccore.i_mmu2ru[i](i_mmu2ru[i]);

    for (size_t i = 0; i < o_ru2tcm.size(); ++i)
        funccore.o_ru2tcm[i](o_ru2tcm[i]);

    for (size_t i = 0; i < o_ru2mlsu.size(); ++i)
        funccore.o_ru2mlsu[i](o_ru2mlsu[i]);

    funccore.set_Id(id_);
    id = id_;
}

void ru::set_id(int set_id)
{
    id = set_id;
    funccore.set_Id(set_id);
}

void instantiate_RU()
{
    static ru demo_ru("ru_demo", 0);
}

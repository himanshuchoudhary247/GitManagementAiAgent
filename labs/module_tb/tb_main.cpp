/*********************
* Author: ABC at abc
* Project: Pname
* File: tb_main.cpp
* Description: Top‑level SystemC entry‑point that instantiates
*              tb_top_ru, drives the system clock/reset, and
*              runs the simulation until sc_stop().
*********************/
#include "tb_top_ru.hpp"

/* Optional VCD tracing handle (left unused but ready) */
sc_trace_file *tf = nullptr;

int sc_main(int argc, char* argv[])
{
    /* 100 MHz clock → 10 ns period */
    sc_clock clk("clk", 10, SC_NS);
    sc_signal<bool> reset;

    /* Instantiate the composite RU test‑bench */
    tb_top_ru test_top("test_top", argc, argv);
    test_top.clk(clk);
    test_top.reset(reset);

    sc_start();         /* runs until tb_ru_funccore calls sc_stop() */
    return 0;
}

#define DEBUG_LOG_SEVERITY_TB 1
#include "tb_ru_funccore.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

/*──────────────────────── ctor + thread registration ─────────────────────*/
tb_ru_funccore::tb_ru_funccore(sc_core::sc_module_name name,
                               int argc,char* argv[])
: sc_module  (name)
, clk        ("clk")
, reset      ("reset")
, o_npuc2mmu ("o_npuc2mmu")
, o_mmu2npuc ("o_mmu2npuc")
, o_mmu2ru   ("o_mmu2ru" , NUM_PORTS)
, i_ru2tcm   ("i_ru2tcm" , NUM_LINES)
, i_ru2mlsu  ("i_ru2mlsu", NUM_LINES)
, o_reg_map  ("o_reg_map")
{
    tb_config::instance().load(argc,argv);        // reuse LU helper

    SC_THREAD(handlerTestMain);    sensitive << clk.pos();  dont_initialize();
    SC_THREAD(respru2tcm);         for(int i=0;i<NUM_LINES;i++)
                                       sensitive << i_ru2tcm[i].data_written();
                                   dont_initialize();
    SC_THREAD(respru2mlsu);        for(int i=0;i<NUM_LINES;i++)
                                       sensitive << i_ru2mlsu[i].data_written();
                                   dont_initialize();

    SC_METHOD(receive_done_micro); sensitive << o_mmu2npuc.data_written();
                                   dont_initialize();
}

/*──────────────────────── helper: build dummy packets ────────────────────*/
mmu2ru_PTR tb_ru_funccore::make_mmu_pkt(uint32_t r,uint32_t c)
{
    mmu2ru_PTR p=new mmu2ru_t; p->row=r; p->col=c;
    for(int i=0;i<32;i++) p->payload[i]=(r+c+i)&0xFF;
    return p;
}
sfr_PTR tb_ru_funccore::make_sfr(bool fused)
{
    uint32_t v=(fused?1u:0u)<<20; sfr_PTR p=new sfr_t; std::memcpy(p,&v,4); return p;
}

/*──────────────────────── main testcase handler ──────────────────────────*/
void tb_ru_funccore::handlerTestMain()
{
    wait();                         // initial delta
    while(reset.read()) wait();

    run_testcase();                 // single self-contained testcase
    wait(2000, SC_NS);              // safety slack
    sc_stop();
}

/*──────────────────────── run_testcase (config-driven) ───────────────────*/
void tb_ru_funccore::run_testcase()
{
    /* 1) fetch testcase parameters */
    common_register_map cfg{};
    tb_config::instance().get_cfg_registers(cfg);

    uint32_t M = cfg.option_tensor_size_size_m * 8;
    uint32_t N = cfg.option_tensor_size_size_n * 32;

    uint32_t base_c     = cfg.addr_tensor_matrix_c_base_matrix_c_base_addr;
    uint32_t row_stride = cfg.addr_tensor_matrix_c_stride_matrix_c_row_stride;
    uint32_t col_stride = cfg.addr_tensor_matrix_c_stride_matrix_c_col_stride;

    std::cout<<"[TB]  M="<<M<<"  N="<<N
             <<"  baseC=0x"<<std::hex<<base_c<<std::dec
             <<"  rowStr="<<row_stride<<"  colStr="<<col_stride<<"\n";

    /* 2) generate golden reference list (non-fused + fused) */
    golden_addr_gen_matrix_c(M,N,base_c,row_stride,col_stride);

    /* 3) phase-1 (non-fused) → expect RU→TCM */
    o_reg_map.write( make_sfr(false) );

    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
            o_mmu2ru[ r & 0xF ].write( make_mmu_pkt(r,c) );

    /* 4) phase-2 (fused) */
    wait(500,SC_NS);
    o_reg_map.write( make_sfr(true) );

    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
            o_mmu2ru[ (r+1) & 0xF ].write( make_mmu_pkt(r,c) );
}

/*──────────────────────── golden address generator ───────────────────────*/
inline uint32_t bank_of(uint32_t r,uint32_t c){return ((r&0xF)<<1)|(c>>1);}
inline uint32_t off_in_bank(uint32_t r,uint32_t c){return (r>>4)*64+((c&1)?32:0);}

void tb_ru_funccore::golden_addr_gen_matrix_c(uint32_t M,uint32_t N,
                                              uint32_t base,uint32_t rs,uint32_t cs)
{
    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
        {
            uint64_t addr = base + uint64_t(r)*rs + uint64_t(c)*cs;
            addr = (addr & ~0xFFFF'FFFFULL) |
                   (uint64_t(bank_of(r,c))<<32) | off_in_bank(r,c);

            golden_q_.emplace(addr,false);   // non-fused phase
            golden_q_.emplace(addr,true );   // fused phase
        }
}

/*──────────────────────── monitor threads ────────────────────────────────*/
void tb_ru_funccore::respru2tcm()
{
    while(true)
    {
        for(int l=0;l<NUM_LINES;l++)
            if(i_ru2tcm[l].num_available())
                compare_golden_output_tcm(i_ru2tcm[l].read());
        wait(SC_ZERO_TIME);
    }
}
void tb_ru_funccore::respru2mlsu()
{
    while(true)
    {
        for(int l=0;l<NUM_LINES;l++)
            if(i_ru2mlsu[l].num_available())
                compare_golden_output_mlsu(i_ru2mlsu[l].read());
        wait(SC_ZERO_TIME);
    }
}

/*──────────────────────── compare helpers ────────────────────────────────*/
void tb_ru_funccore::compare_golden_output_tcm(ru2tcm_PTR p)
{
    assert(!golden_q_.empty());
    auto [gold,fused] = golden_q_.front(); assert(!fused);
    assert(p->addr==gold);
#if DEBUG_LOG_SEVERITY_TB
    std::cout<<sc_time_stamp()<<" ✓ TCM "<<std::hex<<p->addr<<std::dec<<"\n";
#endif
    golden_q_.pop();
}
void tb_ru_funccore::compare_golden_output_mlsu(ru2mlsu_PTR p)
{
    assert(!golden_q_.empty());
    auto [gold,fused] = golden_q_.front(); assert(fused);
    assert(p->addr==gold);
#if DEBUG_LOG_SEVERITY_TB
    std::cout<<sc_time_stamp()<<" ✓ MLSU "<<std::hex<<p->addr<<std::dec<<"\n";
#endif
    golden_q_.pop();
}

/*──────────────────────── optional micro-done (NOP) ──────────────────────*/
void tb_ru_funccore::receive_done_micro()
{
    /* In this RU TB no explicit micro-done packet is expected.
       Keep function for completeness; left empty on purpose. */
}

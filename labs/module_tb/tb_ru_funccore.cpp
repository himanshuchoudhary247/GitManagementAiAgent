#include "tb_ru_funccore.hpp"
#include <fstream>
#include <sstream>
#include <cstring>
#include <iostream>
#include <cassert>

/*────────────────────────── tb_config ───────────────────────────────*/
tb_config::tb_config(int argc,char* argv[])
{
    /* parse one optional JSON/CFG file (same quick parser as before) */
    if(argc < 2) return;
    std::ifstream in(argv[1]);
    if(!in) { std::cerr<<"[TB] cannot open "<<argv[1]<<'\n'; return; }

    std::string l;
    while(std::getline(in,l))
    {
        auto c=l.find(':'); if(c==std::string::npos) continue;
        auto trim=[](std::string&s){s.erase(0,s.find_first_not_of(" \t\""));
                                    s.erase(s.find_last_not_of(" \t\",")+1);};
        std::string key=l.substr(0,c), val=l.substr(c+1); trim(key); trim(val);
        uint32_t num = (val.rfind("0x",0)==0)? std::stoul(val,nullptr,16)
                                            : std::stoul(val,nullptr,10);
#define SET(k) if(key==#k){reg_.k=num; continue;}
        SET(option_tensor_size_size_m)
        SET(option_tensor_size_size_k)
        SET(option_tensor_size_size_n)
        SET(addr_tensor_matrix_c_base_matrix_c_base_addr)
        SET(addr_tensor_matrix_c_stride_matrix_c_row_stride)
        SET(addr_tensor_matrix_c_stride_matrix_c_col_stride)
#undef  SET
    }
}
bool tb_config::get_next_testcase(common_register_map& out)
{
    if(delivered_) return false;
    out = reg_; delivered_=true; return true;
}

/*────────────────────────── TB ctor ────────────────────────────────*/
tb_ru_funccore::tb_ru_funccore(sc_core::sc_module_name n,
                               int argc,char* argv[])
: sc_module(n)
, clk       ("clk")
, reset     ("reset")
, o_npuc2mmu("o_npuc2mmu")
, o_mmu2npuc("o_mmu2npuc")
, o_mmu2ru  ("o_mmu2ru", NUM_PORTS)
, i_ru2tcm  ("i_ru2tcm", NUM_LINES)
, i_ru2mlsu ("i_ru2mlsu", NUM_LINES)
, o_reg_map ("o_reg_map")
, cfg_loader_(argc,argv)
{
    SC_THREAD(handlerTestMain); sensitive<<clk.pos(); dont_initialize();
    SC_THREAD(respru2tcm);      for(int i=0;i<NUM_LINES;i++)
                                    sensitive<<i_ru2tcm[i].data_written();
                                dont_initialize();
    SC_THREAD(respru2mlsu);     for(int i=0;i<NUM_LINES;i++)
                                    sensitive<<i_ru2mlsu[i].data_written();
                                dont_initialize();
    SC_METHOD(receive_done_micro); sensitive<<o_mmu2npuc.data_written();
                                   dont_initialize();
}

/*──────────────────────── helper builders ──────────────────────────*/
mmu2ru_PTR tb_ru_funccore::make_mmu_pkt(uint32_t r,uint32_t c)
{
    mmu2ru_PTR p = new mmu2ru_t;
    p->row = r; p->col = c;
    for(int i=0;i<32;i++) p->payload[i]=(r+c+i)&0xFF;
    return p;
}
sfr_PTR tb_ru_funccore::make_sfr(bool fused)
{
    uint32_t word = (fused?1u:0u)<<20;
    sfr_PTR p=new sfr_t; std::memcpy(p,&word,4); return p;
}

/*──────────────────────── main handler ─────────────────────────────*/
void tb_ru_funccore::handlerTestMain()
{
    common_register_map sfr;
    while(cfg_loader_.get_next_testcase(sfr))
    {
        /* present SFR block */
        sfr_PTR map(new common_register_map()); *map = sfr; o_reg_map.write(map);
        run_testcase(sfr);
        /* mimic micro‑handshake */
        done_micro=false; npuc2mmu_PTR dummy(new npuc2mmu_t); o_npuc2mmu.write(dummy);
        while(!done_micro) wait();
    }
    sc_stop();
}

/*──────────────────────── run_testcase ─────────────────────────────*/
void tb_ru_funccore::run_testcase(const common_register_map& cfg)
{
    uint32_t M  = cfg.option_tensor_size_size_m*8;
    uint32_t N  = cfg.option_tensor_size_size_n*32;
    uint32_t BA = cfg.addr_tensor_matrix_c_base_matrix_c_base_addr;
    uint32_t RS = cfg.addr_tensor_matrix_c_stride_matrix_c_row_stride;
    uint32_t CS = cfg.addr_tensor_matrix_c_stride_matrix_c_col_stride;

    golden_addr_gen_matrix_c(M,N,BA,RS,CS);

    o_reg_map.write(make_sfr(false));          // phase‑1 (TCM)
    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
            o_mmu2ru[r&0xF].write(make_mmu_pkt(r,c));

    wait(500,SC_NS);
    o_reg_map.write(make_sfr(true));           // phase‑2 (MLSU)
    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
            o_mmu2ru[(r+1)&0xF].write(make_mmu_pkt(r,c));
}

/*──────────────────────── golden addresses ─────────────────────────*/
inline uint32_t bank_of(uint32_t r,uint32_t c){return ((r&0xF)<<1)|(c>>1);}
inline uint32_t off_in_bank(uint32_t r,uint32_t c){return (r>>4)*64+((c&1)?32:0);}
void tb_ru_funccore::golden_addr_gen_matrix_c(uint32_t M,uint32_t N,
                                              uint32_t base,uint32_t rs,uint32_t cs)
{
    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
        {
            uint64_t addr= base + uint64_t(r)*rs + uint64_t(c)*cs;
            addr = (addr & ~0xFFFFFFFFULL)              /* clear low 32 */
                 | (uint64_t(bank_of(r,c))<<32)
                 | off_in_bank(r,c);
            golden_q_.emplace(addr,false);
            golden_q_.emplace(addr,true );
        }
}

/*──────────────────────── response monitors ────────────────────────*/
void tb_ru_funccore::respru2tcm()
{
    while(true)
    {
        for(int i=0;i<NUM_LINES;i++)
            if(i_ru2tcm[i].num_available())
                compare_golden_output_tcm(i_ru2tcm[i].read());
        wait(SC_ZERO_TIME);
    }
}
void tb_ru_funccore::respru2mlsu()
{
    while(true)
    {
        for(int i=0;i<NUM_LINES;i++)
            if(i_ru2mlsu[i].num_available())
                compare_golden_output_mlsu(i_ru2mlsu[i].read());
        wait(SC_ZERO_TIME);
    }
}

/*──────────────────────── compare helpers ─────────────────────────*/
void tb_ru_funccore::compare_golden_output_tcm(ru2tcm_PTR p)
{
    auto [addr,fused] = golden_q_.front(); golden_q_.pop();
    assert(!fused && p->addr==addr);
}
void tb_ru_funccore::compare_golden_output_mlsu(ru2mlsu_PTR p)
{
    auto [addr,fused] = golden_q_.front(); golden_q_.pop();
    assert(fused && p->addr==addr);
}

/*──────────────────────── dummy micro done ───────────────────────*/
void tb_ru_funccore::receive_done_micro(){ done_micro=true; }

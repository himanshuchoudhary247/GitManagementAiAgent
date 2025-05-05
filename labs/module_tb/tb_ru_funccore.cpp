#include "tb_ru_funccore.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>
#include <cstring>
/*──────────────────── tb_config implementation ───────────────────────────*/
tb_config::tb_config(int argc,char* argv[])
{
    /* If a JSON file name is given, read simple key/value pairs.           *
     * To keep dependencies low we use a trivial hand‑rolled parser that    *
     * recognises "key": value  (value in hex or dec).                      */
    if(argc < 2) return;                        // use defaults

    std::ifstream in(argv[1]);
    if(!in) { std::cerr<<"[TB] Warning: cannot open "<<argv[1]<<"\n"; return; }

    std::string line;
    while(std::getline(in,line))
    {
        auto colon=line.find(':');
        if(colon==std::string::npos) continue;
        std::string key = line.substr(0,colon);
        std::string val = line.substr(colon+1);
        auto trim=[](std::string&s){s.erase(0,s.find_first_not_of(" \t\"")); s.erase(s.find_last_not_of(" \t\",")+1);};
        trim(key); trim(val);
        uint32_t num = (val.rfind("0x",0)==0)? std::stoul(val,nullptr,16)
                                            : std::stoul(val,nullptr,10);
#define SET(k) if(key==#k) { reg_.k = num; continue; }
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
    out = reg_; delivered_ = true; return true;
}

/*──────────────────── TB ctor ────────────────────────────────────────────*/
tb_ru_funccore::tb_ru_funccore(sc_core::sc_module_name name,
                               int argc,char* argv[])
: sc_module (name)
, clk       ("clk")
, reset     ("reset")
, o_npuc2mmu("o_npuc2mmu")
, o_mmu2npuc("o_mmu2npuc")
, o_mmu2ru  ("o_mmu2ru" , NUM_PORTS)
, i_ru2tcm  ("i_ru2tcm" , NUM_LINES)
, i_ru2mlsu ("i_ru2mlsu", NUM_LINES)
, o_reg_map ("o_reg_map")
, cfg_loader_(argc,argv)
{
    SC_THREAD(handlerTestMain);  sensitive << clk.pos();  dont_initialize();
    SC_THREAD(respru2tcm);       for(int i=0;i<NUM_LINES;i++)
                                     sensitive << i_ru2tcm[i].data_written();
                                 dont_initialize();
    SC_THREAD(respru2mlsu);      for(int i=0;i<NUM_LINES;i++)
                                     sensitive << i_ru2mlsu[i].data_written();
                                 dont_initialize();

    SC_METHOD(receive_done_micro); sensitive << o_mmu2npuc.data_written();
                                   dont_initialize();
}

/*──────────────────── helper builders ───────────────────────────────────*/
mmu2ru_PTR tb_ru_funccore::make_mmu_pkt(uint32_t r,uint32_t c)
{
    mmu2ru_PTR p=new mmu2ru_t; p->row=r; p->col=c;
    for(int i=0;i<32;i++) p->payload[i]=(r+c+i)&0xFF; return p;
}
sfr_PTR tb_ru_funccore::make_sfr(bool fused)
{
    uint32_t v=(fused?1u:0u)<<20; sfr_PTR p=new sfr_t; std::memcpy(p,&v,4); return p;
}

/*──────────────────── MAIN handler (LOOP over testcases) ────────────────*/
void tb_ru_funccore::handlerTestMain()
{
    std::cout<<"[TB_RU]: running @"<<sc_time_stamp()<<"\n";
    common_register_map sfr;
    while(cfg_loader_.get_next_testcase(sfr))
    {
        /* push SFR block to DUT */
        sfr_PTR reg_map(new common_register_map());
        *reg_map = sfr;
        o_reg_map.write(reg_map);

        run_testcase(sfr);

        send_start_micro();          // dummy handshake
        while(!done_micro) wait();   // wait until receive_done_micro sets flag
        done_micro=false;            // reset flag for next testcase
        wait();                      // one delta
    }
    std::cout<<"[TB_RU]: finished @"<<sc_time_stamp()<<"\n";
    sc_stop();
}

/*──────────────────── run_testcase (single config) ──────────────────────*/
void tb_ru_funccore::run_testcase(const common_register_map& cfg)
{
    uint32_t M = cfg.option_tensor_size_size_m * 8;
    uint32_t N = cfg.option_tensor_size_size_n * 32;
    uint32_t base = cfg.addr_tensor_matrix_c_base_matrix_c_base_addr;
    uint32_t rs   = cfg.addr_tensor_matrix_c_stride_matrix_c_row_stride;
    uint32_t cs   = cfg.addr_tensor_matrix_c_stride_matrix_c_col_stride;

#if DEBUG_LOG_SEVERITY_TB
    std::cout<<"[TB_RU]: M="<<M<<" N="<<N<<" base=0x"<<std::hex<<base
             <<std::dec<<" rs="<<rs<<" cs="<<cs<<"\n";
#endif
    golden_addr_gen_matrix_c(M,N,base,rs,cs);

    /* Phase‑1 → TCM */
    o_reg_map.write(make_sfr(false));
    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
            o_mmu2ru[r&0xF].write(make_mmu_pkt(r,c));

    /* Phase‑2 → MLSU */
    wait(500,SC_NS);
    o_reg_map.write(make_sfr(true));
    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
            o_mmu2ru[(r+1)&0xF].write(make_mmu_pkt(r,c));
}

/*──────────────────── golden addr generator ─────────────────────────────*/
inline uint32_t bank_of(uint32_t r,uint32_t c){return ((r&0xF)<<1)|(c>>1);}
inline uint32_t off_in_bank(uint32_t r,uint32_t c){return (r>>4)*64+((c&1)?32:0);}
void tb_ru_funccore::golden_addr_gen_matrix_c(uint32_t M,uint32_t N,
                                              uint32_t base,uint32_t rs,uint32_t cs)
{
    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
        {
            uint64_t addr= base + uint64_t(r)*rs + uint64_t(c)*cs;
            addr = (addr & ~0xFFFF'FFFFULL)
                 | (uint64_t(bank_of(r,c))<<32) | off_in_bank(r,c);
            golden_q_.emplace(addr,false);
            golden_q_.emplace(addr,true );
        }
}

/*──────────────────── response monitors ────────────────────────────────*/
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

/*──────────────────── compare helpers ──────────────────────────────────*/
void tb_ru_funccore::compare_golden_output_tcm(ru2tcm_PTR p)
{
    assert(!golden_q_.empty());
    auto [gold,fused]=golden_q_.front(); assert(!fused);
    assert(p->addr==gold);
#if DEBUG_LOG_SEVERITY_TB
    std::cout<<sc_time_stamp()<<" ✓ TCM 0x"<<std::hex<<p->addr<<std::dec<<"\n";
#endif
    golden_q_.pop();
}
void tb_ru_funccore::compare_golden_output_mlsu(ru2mlsu_PTR p)
{
    assert(!golden_q_.empty());
    auto [gold,fused]=golden_q_.front(); assert(fused);
    assert(p->addr==gold);
#if DEBUG_LOG_SEVERITY_TB
    std::cout<<sc_time_stamp()<<" ✓ MLSU 0x"<<std::hex<<p->addr<<std::dec<<"\n";
#endif
    golden_q_.pop();
}

/*──────────────────── dummy micro handshake ────────────────────────────*/
void tb_ru_funccore::receive_done_micro()
{
    /* For this demo we treat **any** packet written to o_mmu2npuc
       as “micro‑done”.  In a real design this would decode a header. */
    done_micro=true;
}
void tb_ru_funccore::send_start_micro()
{
    /* Send one dummy word down o_npuc2mmu to mimic micro‑start */
    npuc2mmu_PTR p=new npuc2mmu_t;
    o_npuc2mmu.write(p);
}

#include "tb_ru_funccore.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

tb_ru_funccore::tb_ru_funccore(sc_core::sc_module_name n)
: sc_module(n)
, clk("clk"), reset("reset")
, o_npuc2ru("o_npuc2ru")
, i_ru2npuc("i_ru2npuc")
, o_mmu2ru("o_mmu2ru",NUM_PORTS)
, i_ru2tcm("i_ru2tcm",NUM_LINES)
, i_ru2mlsu("i_ru2mlsu",NUM_LINES)
, o_reg_map("o_reg_map")
{
    SC_THREAD(handlerTestMain); sensitive<<clk.pos(); dont_initialize();
    SC_THREAD(resp_tcm);        for(int i=0;i<NUM_LINES;i++)
                                    sensitive<<i_ru2tcm[i].data_written();
                                dont_initialize();
    SC_THREAD(resp_mlsu);       for(int i=0;i<NUM_LINES;i++)
                                    sensitive<<i_ru2mlsu[i].data_written();
                                dont_initialize();
    SC_METHOD(receive_done_micro); sensitive<<i_ru2npuc.data_written();
                                   dont_initialize();
}

/* helpers -----------------------------------------------------------*/
inline uint32_t bank_of(uint32_t r,uint32_t c){return ((r&0xF)<<1)|(c>>1);}
inline uint32_t off_in_bank(uint32_t r,uint32_t c){return (r>>4)*64+((c&1)?32:0);}
static uint64_t mk_addr(uint32_t r,uint32_t c,uint64_t base)
{
    return (base & ~0xFFFFFFFFULL) | (uint64_t(bank_of(r,c))<<32) | off_in_bank(r,c);
}

/* main thread -------------------------------------------------------*/
void tb_ru_funccore::handlerTestMain()
{
    common_register_map cfg;
    tb_config::instance().get_cfg_registers(cfg);

    /* push full SFR to DUT */
    sfr_PTR s(new common_register_map(cfg));
    o_reg_map.write(s);

    golden_gen(cfg);
    send_phase(false,cfg);   // to TCM
    wait(500,SC_NS);
    send_phase(true ,cfg);   // to MLSU
    while(!done_micro) wait();
    std::cout<<"[TB_RU] ✓ finished "<<sc_time_stamp()<<std::endl;
    sc_stop();
}

/* golden address list ----------------------------------------------*/
void tb_ru_funccore::golden_gen(const common_register_map& cfg)
{
    uint32_t M = cfg.option_tensor_size_size_m*8;
    uint32_t N = cfg.option_tensor_size_size_n*32;
    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
        {
            gold_.push({mk_addr(r,c,cfg.addr_tensor_matrix_c_base_matrix_c_base_addr),false});
            gold_.push({mk_addr(r,c,cfg.addr_tensor_matrix_c_base_matrix_c_base_addr),true });
        }
}

/* drive one phase ---------------------------------------------------*/
void tb_ru_funccore::send_phase(bool fused,const common_register_map& cfg)
{
    /* write FUSED_OPERATION bit */
    sfr_PTR sfr(new common_register_map(cfg));
    sfr->mode_tensor.FUSED_OPERATION = fused;
    o_reg_map.write(sfr);

    uint32_t M = cfg.option_tensor_size_size_m*8;
    uint32_t N = cfg.option_tensor_size_size_n*32;

    for(uint32_t r=0;r<M;++r)
        for(uint32_t c=0;c<N;++c)
        {
            mmu2ru_PTR p(new mmu2ru_t);
            p->row=r; p->col=c;
            std::memset(p->C_data,0,sizeof(p->C_data));
            p->done = (r==M-1 && c==N-1);
            o_mmu2ru[r&0xF].write(p);
        }

    /* micro start */
    npuc2ru_PTR start(new npuc2ru_t); start->start=true; o_npuc2ru.write(start);
}

/* monitors ----------------------------------------------------------*/
void tb_ru_funccore::resp_tcm()
{
    while(true){
        for(int i=0;i<NUM_LINES;i++)
            if(i_ru2tcm[i].num_available())
            {
                auto pkt=i_ru2tcm[i].read();
                auto [addr,fused]=gold_.front(); gold_.pop();
                assert(!fused && pkt->data[0].length()==512);
                assert(pkt->address[i]==uint16_t(addr>>32));
            }
        wait(SC_ZERO_TIME);
    }
}
void tb_ru_funccore::resp_mlsu()
{
    while(true){
        for(int i=0;i<NUM_LINES;i++)
            if(i_ru2mlsu[i].num_available())
            {
                auto pkt=i_ru2mlsu[i].read();
                auto [addr,fused]=gold_.front(); gold_.pop();
                assert(fused && pkt->data[0].length()==512);
            }
        wait(SC_ZERO_TIME);
    }
}
void tb_ru_funccore::receive_done_micro()
{
    if(i_ru2npuc.read()->done) done_micro=true;
}

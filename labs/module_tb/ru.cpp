#include "ru.hpp"

ru::ru(sc_core::sc_module_name name, int id_)
: sc_core::sc_module(name)
, clk      ("clk")
, reset    ("reset")
, i_npuc2mu("i_npuc2mu")
, i_mmu2npuc("i_mmu2npuc")
, i_mmu2ru ("i_mmu2ru",16)
, o_ru2tcm ("o_ru2tcm",4)
, o_ru2mlsu("o_ru2mlsu",4)
, i_reg_map("i_reg_map")
, id(id_)
{
    SC_CTHREAD(main_thread, clk.pos());
    reset_signal_is(reset, true);

    // bind vectors
    for(int i=0; i<16; ++i) funccore.i_mmu2ru[i](i_mmu2ru[i]);
    for(int i=0; i< 4; ++i) funcore.o_ru2tcm[i](o_ru2tcm[i]),
                          funccore.o_ru2mlsu[i](o_ru2mlsu[i]);
}

void ru::set_id(int v){ id=v; }

void ru::main_thread()
{
    bool fused = false;
    sc_dt::sc_bv<512> aggregator[4];
    uint8_t fill_count[4] = {0};
    uint32_t row_cnt[16]={0}, col_cnt[16]={0};

    wait(); // reset

    while(true){
        // check SFR
        if(i_reg_map.num_available()){
            uint32_t v = *i_reg_map.read();
            fused = ((v>>20)&1);
        }

        // service all ports
        for(int p=0;p<16;++p){
            if(!i_mmu2ru[p].num_available()) continue;
            auto pkt = i_mmu2ru[p].read();

            uint32_t r = row_cnt[p], c = col_cnt[p];
            uint16_t bank   = ((r&0xF)<<1)|(c>>1);
            uint16_t off    = (r>>4)*64 + ((c&1)?32:0);
            uint16_t addr16 = (bank<<11)|off;
            int line = bank>>3;

            // pack 16×8⟶128 bits
            for(int b=0;b<16;++b){
                aggregator[line].range(fill_count[line]*128+8*b+7,
                                       fill_count[line]*128+8*b)
                    = pkt->C_data[b];
            }
            if(++fill_count[line]==4){
                if(fused){
                    auto o = std::make_shared<ru2mlsu>();
                    o->data = aggregator[line];
                    o->done = pkt->done;
                    o_ru2mlsu[line].write(o);
                } else {
                    auto o = std::make_shared<ru2tcm>();
                    o->data    = aggregator[line];
                    o->address = addr16;
                    o->done    = pkt->done;
                    o_ru2tcm[line].write(o);
                }
                aggregator[line]=0; fill_count[line]=0;
            }

            if(pkt->done==1){
                if(++col_cnt[p]==64){col_cnt[p]=0; ++row_cnt[p];}
            }
        }

        wait();
    }
}

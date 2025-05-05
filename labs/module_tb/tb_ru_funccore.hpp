#pragma once
#include "npucommon.hpp"
#include "npudefine.hpp"
#include <queue>
#include <tuple>
#include <string>

/*─────────────────────────────────────────────────────────────────────────────
 *  On‑the‑fly configuration (no external header required)
 *───────────────────────────────────────────────────────────────────────────*/
struct common_register_map {
    /* tensor size registers (same semantics as LU TB) */
    uint32_t option_tensor_size_size_m = 4;  // default 4×8 = 32 rows
    uint32_t option_tensor_size_size_k = 2;  // not used by RU
    uint32_t option_tensor_size_size_n = 2;  // default 2×32 = 64 cols

    /* matrix‑C base & strides */
    uint32_t addr_tensor_matrix_c_base_matrix_c_base_addr   = 0x00004000;
    uint32_t addr_tensor_matrix_c_stride_matrix_c_row_stride= 0x00000800;
    uint32_t addr_tensor_matrix_c_stride_matrix_c_col_stride= 0x00000020;
};

/* Very small loader that returns exactly **one** testcase. */
class tb_config {
public:
    tb_config(int argc,char* argv[]);
    bool get_next_testcase(common_register_map& out);   // true once, then false
private:
    bool delivered_{false};
    common_register_map reg_;
};

/*──────────────────────────────────────────────────────────────────────────*/
#define NUM_PORTS 16
#define NUM_LINES 4
#define DEBUG_LOG_SEVERITY_TB 1

class tb_ru_funccore : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(tb_ru_funccore);
    tb_ru_funccore(sc_core::sc_module_name name,
                   int argc,
                   char* argv[]);

    /* DUT‑side ports (wired in tb_top_ru) */
    sc_in <bool>              clk;
    sc_in <bool>              reset;
    sc_fifo_out<npuc2mmu_PTR> o_npuc2mmu;
    sc_fifo_out<mmu2npuc_PTR> o_mmu2npuc;
    sc_vector< sc_fifo_out<mmu2ru_PTR> >  o_mmu2ru;
    sc_vector< sc_fifo_in <ru2tcm_PTR> >  i_ru2tcm;
    sc_vector< sc_fifo_in <ru2mlsu_PTR> > i_ru2mlsu;
    sc_fifo_out<sfr_PTR>      o_reg_map;

    void set_id(int v){id=v;}

private:
    /* TB threads / methods */
    void handlerTestMain();
    void respru2tcm();
    void respru2mlsu();
    void receive_done_micro();     // dummy for completeness

    void run_testcase(const common_register_map& cfg);
    void golden_addr_gen_matrix_c(uint32_t M,uint32_t N,
                                  uint32_t base,uint32_t rs,uint32_t cs);
    void compare_golden_output_tcm (ru2tcm_PTR  pkt);
    void compare_golden_output_mlsu(ru2mlsu_PTR pkt);

    mmu2ru_PTR make_mmu_pkt(uint32_t r,uint32_t c);
    sfr_PTR    make_sfr(bool fused);

private:
    tb_config cfg_loader_;
    std::queue<std::tuple<std::uint64_t,bool>> golden_q_;
    bool done_micro{false};
    int  id{0};
};

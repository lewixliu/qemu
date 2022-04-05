/*
 * FR emulation for qemu: main translation routines.
 *
 * Copyright (c) 2022 Lewis Liu <lewix@ustc.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/lgpl-2.1.html>
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "tcg/tcg-op.h"
#include "exec/exec-all.h"
#include "disas/disas.h"
#include "exec/helper-proto.h"
#include "exec/helper-gen.h"
#include "exec/log.h"
#include "exec/cpu_ldst.h"
#include "exec/translator.h"
#include "qemu/qemu-print.h"

static const char * const regnames[] = {
    "r0",         "r1",         "r2",         "r3",
    "r4",         "r5",         "r6",         "r7",
    "r8",         "r9",         "r10",        "r11",
    "r12",        "ac",         "fp",         "sp",
    "tbr",        "rp",         "ssp",        "usp",
    "mdl",        "mdh",        "rsrv",       "rsrv",
    "rsrv",       "rsrv",       "rsrv",       "rsrv",
    "rsrv",       "rsrv",       "rsrv",       "rsrv",
    "pc",         "ps"
};

static TCGv cpu_R[FR_NUM_CORE_REGS];
static TCGv dpc;
static TCGv cpu_PS_N; // should alway be 1 or 0;
static TCGv cpu_PS_Z; // should alway be 1 or 0;
static TCGv cpu_PS_V; // should alway be 1 or 0;
static TCGv cpu_PS_C; // should alway be 1 or 0;

#define tcg_Ri      cpu_R[(a->ri)]
#define tcg_R(n)    cpu_R[n]
#define tcg_Rj      cpu_R[(a->rj)]
#define tcg_Rs      cpu_R[(a->rs)+DR_BASE]
#define tcg_PC      cpu_R[R_PC]
#define tcg_PS      cpu_R[R_PS]

#define tcg_tmp0    cpu_R[31]
#define tcg_tmp1    cpu_R[30]
#define tcg_tmp2    cpu_R[29]
#define tcg_tmp3    cpu_R[28]

#define DISAS_TB_JUMP   DISAS_TARGET_0
#define DISAS_DELAY_JUMP DISAS_TARGET_1

#include "exec/gen-icount.h"


typedef struct DisasContext {
    DisasContextBase base;
    CPUFRState *env;

    target_ulong pc;    /* current Program Counter: integer */
    uint32_t ccr_cached;
    bool dpc;
} DisasContext;

static inline bool use_goto_tb(DisasContext *dc, target_ulong dest)
{
    if (unlikely(dc->base.singlestep_enabled)) {
        return false;
    }

#ifndef CONFIG_USER_ONLY
    return (dc->base.tb->pc & TARGET_PAGE_MASK) == (dest & TARGET_PAGE_MASK);
#else
    return true;
#endif
}

static inline void gen_goto_tb(DisasContext *dc, int n, target_ulong dest)
{
    if (use_goto_tb(dc, dest)) {
        tcg_gen_goto_tb(n);
        tcg_gen_movi_tl(tcg_PC, dest);;
        tcg_gen_exit_tb(dc->base.tb, n);
    } else {
        tcg_gen_movi_tl(tcg_PC, dest);;
        tcg_gen_exit_tb(NULL, 0);
    }
}

typedef struct DisasCompare {
    TCGv value;
    TCGCond cond;
    bool value_global;
} DisasCompare;

static inline void fr_test_cc(DisasCompare *cmp, int cc)
{
    TCGv value;
    TCGCond cond;
    bool global = true;

    switch (cc) {
    case 0: /* BRA:always */
    case 1: /* BNO:always no */
        /* Use the ALWAYS condition, which will fold early.
         * It doesn't matter what we use for the value.  */
        cond = TCG_COND_ALWAYS;
        value = cpu_PS_Z;
        break;

    case 2: /* BEQ: Z = 1 */
    case 3: /* BNE: Z = 0 */
        cond = TCG_COND_NE;
        value = cpu_PS_Z;
        break;

    case 4: /* BC: C = 1 */
    case 5: /* BNC: C = 0 */
        cond = TCG_COND_NE;
        value = cpu_PS_C;
        break;

    case 6: /* BN: N = 1*/
    case 7: /* BP: N = 0 */
        cond = TCG_COND_NE;
        value = cpu_PS_N;
        break;

    case 8: /* BV: V = 1 */
    case 9: /* BNV: V = 0 */
        cond = TCG_COND_NE;
        value = cpu_PS_V;
        break;

    case 10: /* BLT: (N xor V) = 1 */
    case 11: /* BGE: (N xor V) = 0 */
        cond = TCG_COND_NE;
        value = tcg_temp_new();
        global = false;
        tcg_gen_xor_tl(value, cpu_PS_V, cpu_PS_N);
        break;

    case 12: /* BLE: ((V xor N) or Z) = 1 */
    case 13: /* BGT: ((V xor N) or Z) = 0 */
        cond = TCG_COND_NE;
        value = tcg_temp_new();
        global = false;
        tcg_gen_xor_tl(value, cpu_PS_V, cpu_PS_N);
        tcg_gen_or_tl(value, cpu_PS_Z, value);
        break;

    case 14: /* BLS: (C or Z) = 1 */
    case 15: /* BHI: (C or Z) = 0 */
        cond = TCG_COND_NE;
        value = tcg_temp_new();
        global = false;
        tcg_gen_or_tl(value, cpu_PS_C, cpu_PS_Z);
        break;
    default:
        qemu_log_mask(LOG_UNIMP, "Bad condition code 0x%x\n", cc);
        abort();
    }

    if (cc & 1) {
        cond = tcg_invert_cond(cond);
    }

    cmp->cond = cond;
    cmp->value = value;
    cmp->value_global = global;
}

static inline void fr_free_cc(DisasCompare *cmp)
{
    if (!cmp->value_global) {
        tcg_temp_free(cmp->value);
    }
}

static inline void fr_jump_cc(DisasCompare *cmp, TCGLabel *label)
{
    tcg_gen_brcondi_tl(cmp->cond, cmp->value, 0, label);
}

static inline uint32_t ex_load_32(DisasContext *dc)
{
    uint32_t code = cpu_ldl_code(dc->env, dc->base.pc_next);
    dc->base.pc_next += 4;
    return code;
}

static inline uint32_t ex_load_20(DisasContext *dc, int n)
{
    uint32_t code = cpu_lduw_code(dc->env, dc->base.pc_next);
    dc->base.pc_next += 2;
    return (n<<16) | code;
}

static inline uint32_t ex_load_u4c(DisasContext *dc)
{
    uint32_t code = cpu_lduw_code(dc->env, dc->base.pc_next);
    dc->base.pc_next += 2;
    return code;
}

#define ex_lshift_2(dc, n)  ((n)<<2)
#define ex_lshift_1(dc, n)  ((n)<<1)

#include "decode.inc.c"

#define FLAG_NONE               0
#define FLAG_UPDATE_N           (R_PS_CCR_N)
#define FLAG_UPDATE_Z           (R_PS_CCR_Z)
#define FLAG_UPDATE_V           (R_PS_CCR_V)
#define FLAG_UPDATE_C           (R_PS_CCR_C)
#define FLAG_UPDATE_NZ          (FLAG_UPDATE_N|FLAG_UPDATE_Z)
#define FLAG_UPDATE_ZC          (FLAG_UPDATE_Z|FLAG_UPDATE_C)
#define FLAG_UPDATE_NZV         (FLAG_UPDATE_N|FLAG_UPDATE_Z|FLAG_UPDATE_V)
#define FLAG_UPDATE_NZC         (FLAG_UPDATE_N|FLAG_UPDATE_Z|FLAG_UPDATE_C)
#define FLAG_UPDATE_NZVC        (FLAG_UPDATE_N|FLAG_UPDATE_Z|FLAG_UPDATE_V|FLAG_UPDATE_C)
#define FLAG_I                  (R_PS_CCR_I)
#define FLAG_S                  (R_PS_CCR_S)
#define FLAG_SI                 (FLAG_I|FLAG_S)
#define FLAG_T                  (R_PS_SCR_T)
#define FLAG_D0                 (R_PS_SCR_D0)
#define FLAG_D1                 (R_PS_SCR_D1)
#define FLAG_D                  (FLAG_D0|FLAG_D1)
#define FLAG_WRITE_SP           (1u<<27)    // USP/SSP has changed, sync to R15
#define FLAG_WRITE_R15          (1u<<28)    // R15 has changed, sync to USP/SSP
#define FLAG_CCR_SYNC           (1u<<29)    // cpu_PS_x ==> cpu_R[R_PS]
#define FLAG_CCR_INVALID        (1u<<30)    // cpu_R[R_PS] ==> cpu_PS_x
#define FLAG_NOT_IN_DELAY_SLOT  (1u<<31)

static inline void ccr_sync(DisasContext *dc)
{
    TCGv tmp = tcg_temp_new();
    tcg_gen_andi_tl(tcg_PS, tcg_PS, ~(R_PS_CCR_N|R_PS_CCR_Z|R_PS_CCR_V|R_PS_CCR_C));
    tcg_gen_shli_tl(tmp, cpu_PS_N, R_PS_CCR_N_BIT);
    tcg_gen_or_tl(tcg_PS, tcg_PS, tmp);
    tcg_gen_shli_tl(tmp, cpu_PS_Z, R_PS_CCR_Z_BIT);
    tcg_gen_or_tl(tcg_PS, tcg_PS, tmp);
    tcg_gen_shli_tl(tmp, cpu_PS_V, R_PS_CCR_V_BIT);
    tcg_gen_or_tl(tcg_PS, tcg_PS, tmp);
    tcg_gen_shli_tl(tmp, cpu_PS_C, R_PS_CCR_C_BIT);
    tcg_gen_or_tl(tcg_PS, tcg_PS, tmp);
    tcg_temp_free(tmp);
    dc->ccr_cached = 0;
}

static inline void ccr_invalid(DisasContext *dc)
{
    tcg_gen_extract_tl(cpu_PS_N, tcg_PS, R_PS_CCR_N_BIT, 1);
    tcg_gen_extract_tl(cpu_PS_Z, tcg_PS, R_PS_CCR_Z_BIT, 1);
    tcg_gen_extract_tl(cpu_PS_V, tcg_PS, R_PS_CCR_V_BIT, 1);
    tcg_gen_extract_tl(cpu_PS_C, tcg_PS, R_PS_CCR_C_BIT, 1);
    dc->ccr_cached = 0;
}

static inline void sp_has_change(DisasContext *dc)
{
    TCGv tmp = tcg_temp_new();
    TCGLabel *l1 = gen_new_label();
    TCGLabel *l2 = gen_new_label();
    tcg_gen_andi_tl(tmp, tcg_PS, R_PS_CCR_S); // S: 0 is SSP, 1 is USP.
    tcg_gen_brcondi_tl(TCG_COND_NE, tmp, 0, l1); // if flag_s != 0 (is USP): goto l1
    tcg_gen_mov_tl(tcg_R(R_15), tcg_R(R_SSP));
    tcg_gen_brcondi_tl(TCG_COND_ALWAYS, tmp, 0, l2);

    gen_set_label(l1);
    tcg_gen_mov_tl(tcg_R(R_15), tcg_R(R_USP));

    gen_set_label(l2);
    tcg_temp_free(tmp);
}

static inline void r15_has_change(DisasContext *dc)
{
    TCGv tmp = tcg_temp_new();
    TCGLabel *l1 = gen_new_label();
    TCGLabel *l2 = gen_new_label();
    tcg_gen_andi_tl(tmp, tcg_PS, R_PS_CCR_S); // S: 0 is SSP, 1 is USP.
    tcg_gen_brcondi_tl(TCG_COND_NE, tmp, 0, l1); // if flag_s != 0 (is USP): goto l1
    tcg_gen_mov_tl(tcg_R(R_SSP), tcg_R(R_15));
    tcg_gen_brcondi_tl(TCG_COND_ALWAYS, tmp, 0, l2);

    gen_set_label(l1);
    tcg_gen_mov_tl(tcg_R(R_USP), tcg_R(R_15));

    gen_set_label(l2);
    tcg_temp_free(tmp);
}

#define FR_GEN_CODE(insn, format, cycle, _flag, sequence)               \
static bool glue(trans_, insn)(DisasContext *dc, glue(arg_, insn) *a)   \
{                                                                       \
    uint32_t flag = _flag;                                              \
    CPUFRState *env = dc->env;                                          \
    dc->ccr_cached |= (_flag & FLAG_UPDATE_NZVC);                       \
    if (dc->dpc == true) {                                              \
        if (_flag&FLAG_NOT_IN_DELAY_SLOT)                               \
            cpu_abort(env_cpu(env), "Forbidden insn in delay slot!");   \
        dc->base.is_jmp = DISAS_DELAY_JUMP;                             \
        tcg_gen_mov_tl(tcg_PC, dpc);                                    \
        dc->dpc = false;                                                \
    }                                                                   \
    if (_flag&FLAG_CCR_SYNC)                                            \
        ccr_sync(dc);                                                   \
    sequence;                                                           \
    if (flag&FLAG_CCR_INVALID)                                          \
        ccr_invalid(dc);                                                \
    if ((flag&(FLAG_WRITE_SP|FLAG_WRITE_R15))                           \
             == (FLAG_WRITE_SP|FLAG_WRITE_R15))                         \
        cpu_abort(env_cpu(env), "Internal error!");                     \
    if (flag&FLAG_WRITE_SP)                                             \
        sp_has_change(dc);                                              \
    if (flag&FLAG_WRITE_R15)                                            \
        r15_has_change(dc);                                             \
    return true;                                                        \
}

#include "insns.helper.c"
#undef FR_GEN_CODE

static inline void save_state(DisasContext *dc)
{
    qemu_log_mask(LOG_UNIMP, "Unimplemented function %s\n", __func__);
}

static void fr_tr_init_disas_context(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    CPUFRState *env = cs->env_ptr;
    int bound;

    dc->pc = dc->base.pc_first;
    dc->env = env;

    /*
     * if we reach a page boundary, we stop generation so that the
     * PC of a TT_TFAULT exception is always in the right page
     */
    bound = -(dc->base.pc_first | TARGET_PAGE_MASK) / INSN_SIZE;
    dc->base.max_insns = MIN(dc->base.max_insns, bound);
}

static void fr_tr_tb_start(DisasContextBase *db, CPUState *cs)
{
    CPUFRState *env = cs->env_ptr;
    DisasContext *dc = container_of(db, DisasContext, base);

    dc->dpc = !!env->dpc;

    if (qemu_loglevel_mask(CPU_LOG_TB_OP)) {
        cpu_dump_state(env_cpu(env), stderr, 0);
    }
}

static void fr_tr_insn_start(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    tcg_gen_insn_start(dc->pc);
}

static bool fr_tr_breakpoint_check(DisasContextBase *dcbase, CPUState *cs,
                                      const CPUBreakpoint *bp)
{
    // TODO: check
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    if (dc->pc != dc->base.pc_first) {
        save_state(dc);
    }
    //gen_helper_debug(cpu_env);
    tcg_gen_exit_tb(NULL, 0);
    dc->base.is_jmp = DISAS_NORETURN;
    /* update pc_next so that the current instruction is included in tb->size */
    dc->base.pc_next += INSN_SIZE;
    return true;
}

static void fr_tr_translate_insn(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    uint32_t insn;

    insn = cpu_lduw_code(dc->env, dc->base.pc_next);
    dc->base.pc_next += INSN_SIZE;

    if (!fr_decode(dc, insn)) {
        if (!dc->dpc) // if illegal instruction in delayslot, just ignore
            gen_helper_raise_illegal_instruction(cpu_env);
    }

    dc->pc = dc->base.pc_next;

    if (dc->base.is_jmp == DISAS_NORETURN) {
        return;
    }

}

static void fr_tr_tb_stop(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    CPUFRState *env = cs->env_ptr; (void)env;

    if (!dc->dpc) {
        tcg_gen_movi_tl(dpc, 0); // clear it
    }

    if (dc->ccr_cached) {
        ccr_sync(dc);
    }

    switch (dc->base.is_jmp) {
    case DISAS_NEXT:
    case DISAS_TOO_MANY:
        gen_goto_tb(dc, 0, dc->pc);
        break;
    case DISAS_TB_JUMP:
    case DISAS_DELAY_JUMP:
        tcg_gen_exit_tb(NULL, 0);
        break;
    case DISAS_NORETURN:
        break;
    default:
        g_assert_not_reached();
    }
}

static void fr_tr_disas_log(const DisasContextBase *dcbase, CPUState *cpu)
{
    qemu_log("IN: %s\n", lookup_symbol(dcbase->pc_first));
    log_target_disas(cpu, dcbase->pc_first, dcbase->tb->size);
}

static const TranslatorOps fr_tr_ops = {
    .init_disas_context = fr_tr_init_disas_context,
    .tb_start           = fr_tr_tb_start,
    .insn_start         = fr_tr_insn_start,
    .breakpoint_check   = fr_tr_breakpoint_check,
    .translate_insn     = fr_tr_translate_insn,
    .tb_stop            = fr_tr_tb_stop,
    .disas_log          = fr_tr_disas_log,
};

/* generate intermediate code for basic block 'tb'.  */
void gen_intermediate_code(CPUState *cs, TranslationBlock *tb, int max_insns)
{
    DisasContext dc = {};

    translator_loop(&fr_tr_ops, &dc.base, cs, tb, max_insns);
}

void fr_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    FRCPU *cpu = FR_CPU(cs);
    CPUFRState *env = &cpu->env;

    int i;

    if (!env) {
        return;
    }

    qemu_fprintf(f, "IN:    PC=%08x %s\n",
                 env->regs[R_PC], lookup_symbol(env->regs[R_PC]));

    for (i = 0; i < FR_NUM_CORE_REGS; i++) {
        if (i > R_MDH && i <= R_PC) continue;

        qemu_fprintf(f, "%9s=%8.8x ", regnames[i], env->regs[i]);
        if ((i + 1) % 4 == 0) {
            qemu_fprintf(f, "\n");
        }
    }


#ifdef _FR_DEBUG_
    qemu_fprintf(f, "\nFor debug: R_PS: %8.8x\n"
                    "CCR flag: \tS: %x, I:%x\n"
                    "\t\t\tN:%8x Z:%8x V:%8x C:%8x\n"
                    "dpc: %8.8x",
        env->regs[R_PS],
        !!(env->regs[R_PS]&R_PS_CCR_S),
        !!(env->regs[R_PS]&R_PS_CCR_I),
        env->flag_n, env->flag_z, env->flag_v, env->flag_c,
        env->dpc);
#endif
    qemu_fprintf(f, "\n\n");
}

void fr_tcg_init(void)
{
    int i;

    for (i = 0; i < FR_NUM_CORE_REGS; i++) {
        cpu_R[i] = tcg_global_mem_new(cpu_env,
                                      offsetof(CPUFRState, regs[i]),
                                      regnames[i]);
    }
    dpc = tcg_global_mem_new(cpu_env,
                                offsetof(CPUFRState, dpc),
                                "dpc");
    cpu_PS_N = tcg_global_mem_new(cpu_env,
                                offsetof(CPUFRState, flag_n),
                                "ccr_N");
    cpu_PS_Z = tcg_global_mem_new(cpu_env,
                                offsetof(CPUFRState, flag_z),
                                "ccr_Z");
    cpu_PS_V = tcg_global_mem_new(cpu_env,
                                offsetof(CPUFRState, flag_v),
                                "ccr_V");
    cpu_PS_C = tcg_global_mem_new(cpu_env,
                                offsetof(CPUFRState, flag_c),
                                "ccr_C");
}

void restore_state_to_opc(CPUFRState *env, TranslationBlock *tb,
                          target_ulong *data)
{
    env->regs[R_PC] = data[0];
}

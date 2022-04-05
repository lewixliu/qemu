/*
 * FR CPU header
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

#ifndef FR_CPU_H
#define FR_CPU_H

#include "exec/cpu-defs.h"
#include "hw/core/cpu.h"

#ifdef _FR_DEBUG_
#define _D(x)    x
#else
#define _D(x)
#endif

typedef struct CPUFRState CPUFRState;
#if !defined(CONFIG_USER_ONLY)
#include "mmu.h"
#endif

#define TYPE_FR_CPU "fr-cpu"

#define FR_CPU_CLASS(klass) \
    OBJECT_CLASS_CHECK(FRCPUClass, (klass), TYPE_FR_CPU)
#define FR_CPU(obj) \
    OBJECT_CHECK(FRCPU, (obj), TYPE_FR_CPU)
#define FR_CPU_GET_CLASS(obj) \
    OBJECT_GET_CLASS(FRCPUClass, (obj), TYPE_FR_CPU)

/**
 * FRCPUClass:
 * @parent_reset: The parent class' reset handler.
 *
 * A FR CPU model.
 */
typedef struct FRCPUClass {
    /*< private >*/
    CPUClass parent_class;
    /*< public >*/

    DeviceRealize parent_realize;
    DeviceReset parent_reset;
} FRCPUClass;

#define INSN_SIZE 2

/* GP regs + DR regs + PC + PS */
#define FR_NUM_CORE_REGS (16 + 16 + 2)

/* General-purpose Register aliases */
#define R_0     0
#define R_1     1
#define R_2     2
#define R_3     3
#define R_4     4
#define R_5     5
#define R_6     6
#define R_7     7
#define R_8     8
#define R_9     9
#define R_10    10
#define R_11    11
#define R_12    12
#define R_13    13
#define R_14    14
#define R_15    15
#define R_AC    13
#define R_FP    14
#define R_SP    15

/* Dedicated register aliases */
#define DR_BASE 16
#define R_TBR   16
#define R_RP    17
#define R_SSP   18
#define R_USP   19
#define R_MDL   20
#define R_MDH   21
#define DR_END  21
#define SR_BASE 32
#define R_PC    32
#define R_PS    33
#define   R_PS_CCR_C_BIT    0
#define   R_PS_CCR_C        (1 << 0)
#define   R_PS_CCR_V_BIT    1
#define   R_PS_CCR_V        (1 << 1)
#define   R_PS_CCR_Z_BIT    2
#define   R_PS_CCR_Z        (1 << 2)
#define   R_PS_CCR_N_BIT    3
#define   R_PS_CCR_N        (1 << 3)
#define   R_PS_CCR_I_BIT    4
#define   R_PS_CCR_I        (1 << 4)
#define   R_PS_CCR_S_BIT    5
#define   R_PS_CCR_S        (1 << 5)
#define   R_PS_CCR_T_BIT    8
#define   R_PS_SCR_T        (1 << 8)
#define   R_PS_SCR_D0_BIT   9
#define   R_PS_SCR_D0       (1 << 9)
#define   R_PS_SCR_D1_BIT   10
#define   R_PS_SCR_D1       (1 << 10)
#define   R_PS_ILM0_BIT     16
#define   R_PS_ILM0         (1 << 16)
#define   R_PS_ILM1_BIT     17
#define   R_PS_ILM1         (1 << 17)
#define   R_PS_ILM2_BIT     18
#define   R_PS_ILM2         (1 << 18)
#define   R_PS_ILM3_BIT     19
#define   R_PS_ILM3         (1 << 19)
#define   R_PS_ILM4_BIT     20
#define   R_PS_ILM4         (1 << 20)
#define   R_PS_ILM          (R_PS_ILM0|R_PS_ILM1|R_PS_ILM2|R_PS_ILM3|R_PS_ILM4)

typedef void * FRMMU;  // TODO: no mmu?

#define INT_RESET   0
#define INT_CET     7   // Coprocessor Error Trap
#define INT_CNFT    8   // Coprocessor Not Found Trap
#define INT_INTE    9
#define INT_STT     12   // Step Trace Traps
#define INT_UIE     14
#define INT_NMI     15
#define INT_N(n)    (0x3FC-4*(n))

#define CPU_INTERRUPT_NMI       CPU_INTERRUPT_TGT_EXT_3
#define FR_USER_INTERRUPTS      48
#define FR_USER_INT_VECTOR_BASE 16

struct CPUFRState {
    uint32_t regs[FR_NUM_CORE_REGS];
    uint32_t dpc;
    uint32_t flag_n;
    uint32_t flag_z;
    uint32_t flag_v;
    uint32_t flag_c;

#if !defined(CONFIG_USER_ONLY)
    FRMMU mmu;
    DECLARE_BITMAP(irq_pending, FR_USER_INTERRUPTS);
#endif
};

/**
 * FRCPU:
 * @env: #CPUFRState
 *
 * A FR CPU.
 */
typedef struct FRCPU {
    /*< private >*/
    CPUState parent_obj;
    /*< public >*/

    CPUNegativeOffsetState neg;
    CPUFRState env;

    hwaddr reset_addr;
    bool mmu_present;
} FRCPU;


void fr_tcg_init(void);
void fr_cpu_do_interrupt(CPUState *cs);
void fr_cpu_dump_state(CPUState *cpu, FILE *f, int flags);
hwaddr fr_cpu_get_phys_page_debug(CPUState *cpu, vaddr addr);
void fr_cpu_do_unaligned_access(CPUState *cpu, vaddr addr,
                                   MMUAccessType access_type,
                                   int mmu_idx, uintptr_t retaddr);

void fr_check_interrupts(CPUFRState *env);

#define CPU_RESOLVING_TYPE TYPE_FR_CPU

//#define cpu_gen_code cpu_fr_gen_code
#define cpu_signal_handler cpu_fr_signal_handler

/* MMU modes definitions */
#define MMU_IDX  0 // TODO: no mmu?

static inline int cpu_mmu_index(CPUFRState *env, bool ifetch)
{
    return MMU_IDX;
}

bool fr_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                        MMUAccessType access_type, int mmu_idx,
                        bool probe, uintptr_t retaddr);

static inline int cpu_interrupts_enabled(CPUFRState *env)
{
    return env->regs[R_PS] & R_PS_CCR_I;
}

typedef CPUFRState CPUArchState;
typedef FRCPU ArchCPU;

#include "exec/cpu-all.h"

static inline void cpu_get_tb_cpu_state(CPUFRState *env, target_ulong *pc,
                                        target_ulong *cs_base, uint32_t *flags)
{
    *pc = env->regs[R_PC];
    *cs_base = 0;
    *flags = 0;
}

#endif /* FR_CPU_H */

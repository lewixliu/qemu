/*
 * FR helper routines header.
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
#include "qemu/host-utils.h"
#include "exec/exec-all.h"
#include "exec/cpu_ldst.h"
#include "exec/log.h"
#include "exec/helper-proto.h"

#if !defined(CONFIG_USER_ONLY)

void fr_cpu_do_interrupt(CPUState *cs)
{
    FRCPU *cpu = FR_CPU(cs);
    CPUFRState *env = &cpu->env;
    uint32_t ilm;

    if (cs->exception_index < 0 || cs->exception_index > 255 ) {
        return;
    }

    ilm = extract32(env->regs[R_PS], R_PS_ILM0_BIT, 5);

    // TODO: Multi-EIT Processing
    switch (cs->exception_index) {
        case 0:
            ilm = 16;
            break;
        case 9: // INTE
        case 12: // Step trace trap
            ilm = 4;
            break;
        case 15: // NMI
            ilm = 15;
            break;
        default:
            if (cs->exception_index >= 16 && cs->exception_index <= 63) {
                uint32_t icr = cpu_ldub_data(env, 0x440+cs->exception_index-16);
                ilm = icr|0x10;
            }
    }

    qemu_log_mask(CPU_LOG_INT, "interrupt at pc=%x, index: %d\n", env->regs[R_PC], cs->exception_index);

    env->regs[R_SSP] -=4;
    cpu_stl_data(env, env->regs[R_SSP], env->regs[R_PS]);
    env->regs[R_SSP] -=4;
    cpu_stl_data(env, env->regs[R_SSP], env->regs[R_PC]);
    env->regs[R_PS] = deposit32(env->regs[R_PS], R_PS_ILM0_BIT, 5, ilm);
    env->regs[R_PC] = cpu_ldl_data(env, env->regs[R_TBR] + INT_N(cs->exception_index));
    env->regs[R_PS] &= ~R_PS_CCR_S;
    env->regs[R_15] = env->regs[R_SSP];
}

bool fr_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                        MMUAccessType access_type, int mmu_idx,
                        bool probe, uintptr_t retaddr)
{
    FRCPU *cpu = FR_CPU(cs);

    assert(!cpu->mmu_present);

    if (!cpu->mmu_present) {
        /* No MMU */
        address &= TARGET_PAGE_MASK;
        tlb_set_page(cs, address, address, PAGE_BITS,
                     mmu_idx, TARGET_PAGE_SIZE);
        return true;
    }

    return false;
}

hwaddr fr_cpu_get_phys_page_debug(CPUState *cs, vaddr addr)
{
    //FRCPU *cpu = FR_CPU(cs);
    target_ulong paddr = 0;

    paddr = addr & TARGET_PAGE_MASK;

    return paddr;
}

void fr_cpu_do_unaligned_access(CPUState *cs, vaddr addr,
                                   MMUAccessType access_type,
                                   int mmu_idx, uintptr_t retaddr)
{
    FRCPU *cpu = FR_CPU(cs);
    CPUFRState *env = &cpu->env;

    helper_raise_exception(env, INT_UIE); // TODO: not UIE
}

#endif /* !CONFIG_USER_ONLY */

/*
 * FR helper routines.
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
#include "exec/helper-proto.h"
#include "exec/cpu_ldst.h"
#include "exec/exec-all.h"
#include "qemu/main-loop.h"

#if !defined(CONFIG_USER_ONLY)
void helper_mmu_read_debug(CPUFRState *env, uint32_t rn)
{
    mmu_read_debug(env, rn);
}

void helper_mmu_write(CPUFRState *env, uint32_t rn, uint32_t v)
{
    mmu_write(env, rn, v);
}

void helper_check_interrupts(CPUFRState *env)
{
    qemu_mutex_lock_iothread();
    fr_check_interrupts(env);
    qemu_mutex_unlock_iothread();
}
#endif /* !CONFIG_USER_ONLY */

void helper_raise_exception(CPUFRState *env, uint32_t index)
{
    CPUState *cs = env_cpu(env);
    cs->exception_index = index;
    cpu_loop_exit(cs);
}

void helper_raise_illegal_instruction(CPUFRState *env)
{
    // TODO:
    //helper_raise_exception(env, INT_UIE);
    qemu_log_mask(LOG_UNIMP, "Unimplemented function %s\n", __func__);
    cpu_abort(env_cpu(env), "Illegal instruction!");
}

void helper_debug_dump(CPUFRState *env)
{
    // TODO: add more debugs
    cpu_dump_state(env_cpu(env), stderr, 0);
}

/*
 * FR MMU emulation stub for qemu.
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
#include "qemu/qemu-print.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "mmu.h"

#if !defined(CONFIG_USER_ONLY)

void mmu_init(CPUFRState *env)
{
    qemu_log_mask(LOG_UNIMP, "Unimplemented function %s\n", __func__);
}

void mmu_read_debug(CPUFRState *env, uint32_t rn)
{
    qemu_log_mask(LOG_UNIMP, "Unimplemented function %s\n", __func__);
}

void mmu_write(CPUFRState *env, uint32_t rn, uint32_t v)
{
    qemu_log_mask(LOG_UNIMP, "Unimplemented function %s\n", __func__);
}

#endif

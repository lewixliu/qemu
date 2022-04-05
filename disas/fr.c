/* FR opcode library for QEMU.
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
#include "disas/dis-asm.h"
#include "exec/log.h"

int print_insn_fr(bfd_vma memaddr, struct disassemble_info *info)
{
    int status;
    bfd_byte buffer[6];

    if (!(status = info->read_memory_func(memaddr, buffer, 6, info))) {
        return do_print_insn_fr(memaddr, buffer);
    }

    info->memory_error_func(status, memaddr, info);
    return -1;
}


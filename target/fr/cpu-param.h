/*
 * FR cpu parameters for qemu.
 *
 * Copyright (c) 2022 Lewis Liu <lewix@ustc.edu>
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef FR_CPU_PARAM_H
#define FR_CPU_PARAM_H 1

#define TARGET_WORDS_BIGENDIAN
#define TARGET_LONG_BITS 32
#define TARGET_PAGE_BITS 12 // TODO: ???
#define TARGET_PHYS_ADDR_SPACE_BITS 32
#ifdef CONFIG_USER_ONLY
# define TARGET_VIRT_ADDR_SPACE_BITS 31
#else
# define TARGET_VIRT_ADDR_SPACE_BITS 32
#endif
#define NB_MMU_MODES 1 // TODO: ???

#endif

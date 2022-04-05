/*
 * Generic simulator target with no MMU or devices.
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
#include "qapi/error.h"
#include "qemu-common.h"
#include "cpu.h"

#include "hw/sysbus.h"
#include "hw/char/serial.h"
#include "hw/boards.h"
#include "exec/memory.h"
#include "exec/address-spaces.h"
#include "qemu/config-file.h"

#include "boot.h"

#define BINARY_DEVICE_TREE_FILE    "generic-nommu.dtb"

static void fr_generic_nommu_init(MachineState *machine)
{
    FRCPU *cpu;
    MemoryRegion *address_space_mem = get_system_memory();
    MemoryRegion *phys_ram = g_new(MemoryRegion, 1);
    MemoryRegion *phys_rom = g_new(MemoryRegion, 1);
    ram_addr_t ram_base = 0x34000;
    ram_addr_t ram_size = 0xc000;
    ram_addr_t rom_base = 0x80000;
    ram_addr_t rom_size = 0x80000;

    memory_region_init_ram(phys_ram, NULL, "fr.ram", ram_size,
                           &error_abort);
    memory_region_add_subregion(address_space_mem, ram_base, phys_ram);

    memory_region_init_rom(phys_rom, NULL, "fr.rom", rom_size,
                           &error_abort);
    memory_region_add_subregion(address_space_mem, rom_base, phys_rom);

    cpu = FR_CPU(cpu_create(TYPE_FR_CPU));

    /* Remove MMU */
    cpu->mmu_present = false;

    fr_load_rom(cpu, rom_base, rom_size);
}

static void fr_generic_nommu_machine_init(struct MachineClass *mc)
{
    mc->desc = "Generic FR CPU";
    mc->init = fr_generic_nommu_init;
}

DEFINE_MACHINE("fr-generic-nommu", fr_generic_nommu_machine_init);

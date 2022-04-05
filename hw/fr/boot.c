/*
 * FR kernel loader
 *
 * Copyright (c) 2022 Lewis Liu <lewix@ustc.edu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qemu-common.h"
#include "cpu.h"
#include "qemu/option.h"
#include "qemu/config-file.h"
#include "qemu/error-report.h"
#include "sysemu/device_tree.h"
#include "sysemu/reset.h"
#include "sysemu/sysemu.h"
#include "hw/loader.h"
#include "elf.h"

#include "boot.h"

#define EM_FR30 84

static struct fr_boot_info {
    uint32_t bootstrap_pc;
} boot_info;

static void main_cpu_reset(void *opaque)
{
    FRCPU *cpu = opaque;
    CPUState *cs = CPU(cpu);
    //CPUFRState *env = &cpu->env;

    cpu_reset(CPU(cpu));
    cpu_set_pc(cs, boot_info.bootstrap_pc);

}

void fr_load_rom(FRCPU *cpu, hwaddr rom_base, uint32_t romsize)
{
    QemuOpts *machine_opts;
    const char *kernel_filename;

    machine_opts = qemu_get_machine_opts();
    kernel_filename = qemu_opt_get(machine_opts, "kernel");

    qemu_register_reset(main_cpu_reset, cpu);

    if (kernel_filename) {
        int kernel_size;
        uint64_t entry, low, high;
        int big_endian = 0;

#ifdef TARGET_WORDS_BIGENDIAN
        big_endian = 1;
#endif

        /* Boots a kernel elf binary. */
        kernel_size = load_elf(kernel_filename, NULL, NULL, NULL,
                               &entry, &low, &high, NULL,
                               big_endian, EM_FR30, 0, 0);
        /* Use the entry point in the ELF image.  */
        boot_info.bootstrap_pc = (uint32_t)entry;

        /* If it wasn't an ELF image, try an u-boot image. */
        if (kernel_size < 0) {
            hwaddr uentry, loadaddr = LOAD_UIMAGE_LOADADDR_INVALID;

            kernel_size = load_uimage(kernel_filename, &uentry, &loadaddr, 0,
                                      NULL, NULL);
            boot_info.bootstrap_pc = uentry;
            high = loadaddr + kernel_size;
        }

        /* Not an ELF image nor an u-boot image, try a RAW image. */
        if (kernel_size < 0) {
            kernel_size = load_image_targphys(kernel_filename, rom_base,
                                              romsize);

            if (kernel_size < romsize) {
                boot_info.bootstrap_pc = rom_base; // no reset address.
            }
            else {
                uint32_t *reset = rom_ptr(cpu->reset_addr, 4);
                uint32_t reset_0 = reset[0];
                if (big_endian) {
                    reset_0 = bswap32(reset_0);
                }
                boot_info.bootstrap_pc = reset_0;
            }
            high = rom_base + kernel_size;
        }

        high = ROUND_UP(high, 512 * KiB);

    }
}

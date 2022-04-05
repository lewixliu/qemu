/*
 * MB91635 Simulator.
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
//#define _FR_DEBUG_
#include "cpu.h"

#include "hw/sysbus.h"
#include "hw/char/serial.h"
#include "hw/boards.h"
#include "hw/ptimer.h"
#include "exec/memory.h"
#include "exec/address-spaces.h"
#include "qemu/config-file.h"
#include "exec/log.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"

#include "mb91635.h"
#include "boot.h"

uint8_t iomem[IOSIZE];

struct mb91637_registers {
    FRCPU *cpu;
    ptimer_state *TMRL[3];
    qemu_irq irq;
    qemu_irq dicr; // TODO:
} all_regs;

static void reload_timer_hit(void *opaque, int timer)
{
    struct mb91637_registers *regs = (struct mb91637_registers *)opaque;
    FRCPU *cpu = regs->cpu;

    CPUState *cs = CPU(cpu);
    cs->exception_index = 20;
    cpu_interrupt(cs, CPU_INTERRUPT_HARD);
    iomem[REGISTER_TMCSR0+timer*8+1] |= 1<<2; // [bit2]: UF (Underflow interrupt request flag bit)
}

static void reload_timer0_hit(void *opaque)
{
    reload_timer_hit(opaque, 0);
}

static void reload_timer1_hit(void *opaque)
{
    reload_timer_hit(opaque, 1);
}

static void reload_timer2_hit(void *opaque)
{
    reload_timer_hit(opaque, 2);
}

static void reload_timer_write_TMCSR(void *opaque, uint16_t value, int timer)
{
    struct mb91637_registers *regs = (struct mb91637_registers *)opaque;
    uint8_t reld = extract16(value, 4, 1);
    //uint8_t inte = extract16(value, 3, 1);
    //uint8_t cnte = extract16(value, 1, 1);
    uint8_t trg = extract16(value, 0, 1);

    ptimer_state *ptimer = regs->TMRL[timer];

    iomem[REGISTER_TMR0+timer*8] = iomem[REGISTER_TMRLRA0+timer*8];
    iomem[REGISTER_TMR0+timer*8 + 1] = iomem[REGISTER_TMRLRA0+timer*8 + 1];

    if (trg) {
        uint16_t limit = *(uint16_t *)(iomem+REGISTER_TMR0+timer*8);
        limit = be16_to_cpu(limit);
        ptimer_transaction_begin(ptimer);
        ptimer_set_freq(ptimer, 40*1000*1000/64/100); // TODO:
        ptimer_set_limit(ptimer, limit, reld);
        ptimer_run(ptimer, 0);
        ptimer_transaction_commit(ptimer);
    }
}

static void reload_timer_write(void *opaque, hwaddr addr,
                           uint64_t value, unsigned size)
{
    assert(size == 2);
    switch (addr) {
        case REGISTER_TMCSR0:
        case REGISTER_TMCSR1:
        case REGISTER_TMCSR2:
            reload_timer_write_TMCSR(opaque, value, (addr-REGISTER_TMCSR0)/8);
            break;
    }
}

static void dicr_write(void *opaque, uint64_t value)
{
    struct mb91637_registers *regs = (struct mb91637_registers *)opaque;
    //FRCPU *cpu = regs->cpu;

    //CPUState *cs = CPU(cpu);
    _D(qemu_log("%s: value: %lx\n", __func__, value));
    qemu_set_irq(regs->dicr, value&1);
}

static uint64_t io_read(void *opaque, hwaddr addr,
                              unsigned size)
{
    int i;
    uint64_t value = 0;
    struct mb91637_registers *regs = (struct mb91637_registers *)opaque;
    FRCPU *cpu = regs->cpu;
    CPUFRState *env = &cpu->env; (void)env;
    for (i=0;i<size;i++) {
        value = deposit64(value, 8*(size-1-i), 8, iomem[addr+i]); // big endian
    }
    if (addr == REGISTER_CMONR && size == 1) {
        value = iomem[REGISTER_CSELR];
    }

    _D(qemu_log("%s: addr: %04lx size: %d value: %lx at pc: %x\n", __func__, addr, size, value, env->regs[R_PC]));
    return value;
}

static void io_write(void *opaque, hwaddr addr,
                           uint64_t value, unsigned size)
{
    int i;
    struct mb91637_registers *regs = (struct mb91637_registers *)opaque;
    FRCPU *cpu = regs->cpu;
    CPUFRState *env = &cpu->env; (void)env;

    _D(qemu_log("%s: addr: %04lx size: %d value: %lx at pc: %x\n", __func__, addr, size, value, env->regs[R_PC]));
    for (i=0;i<size;i++) {
        iomem[addr+i] = extract64(value, 8*(size-1-i), 8); // big endian
    }

    if (addr >= REGISTER_TMRLRA0 && addr <= REGISTER_TMCSR2) {
        reload_timer_write(opaque, addr, value, size);
    }
    else if (addr == REGISTER_DICR && size == 2) {
        dicr_write(opaque, value);
    }
    else if (addr >= REGISTER_ICR00 && addr <= REGISTER_ICR47) {
        //qemu_irq_raise(regs->irq);
    }

}

const MemoryRegionOps iomem_ops = {
    .read = io_read,
    .write = io_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4,
    },
};

#include "exec/cpu_ldst.h"
static inline void fr_check_pic(CPUFRState *env, int interrupt)
{
    CPUState *cs = env_cpu(env);

#if 0 // TODO: enable it.
    uint32_t icr = cpu_ldub_data(env, 0x440+interrupt);
    uint32_t ilm = extract32(env->regs[R_PS], R_PS_ILM0_BIT, 5);
    icr |= 0x10;
    if (icr >= ilm) {
        qemu_log_mask(CPU_LOG_INT, "Interrupt masked at pc=%x, index: %d, icr: %x, ilm: %x\n",
            env->regs[R_PC], interrupt, icr, ilm);
        return;
    }

    if ((env->regs[R_PS]&R_PS_CCR_I) == 0) {
        qemu_log_mask(CPU_LOG_INT, "R_PS_CCR_I is 0. Interrupt disabled at pc=%x, index: %d\n", env->regs[R_PC], interrupt);
        return;
    }
#endif

    cs->exception_index = interrupt+FR_USER_INT_VECTOR_BASE;
    cpu_interrupt(cs, CPU_INTERRUPT_HARD);
    clear_bit(interrupt, env->irq_pending);
}

static void fr_pic_cpu_handler(void *opaque, int irq, int level)
{
    FRCPU *cpu = (FRCPU*)opaque;
    CPUFRState *env = &cpu->env;
    CPUState *cs = env_cpu(env);
    int interrupt;
    _D(qemu_log("%s: irq: %d, level: %d\n", __func__, irq, level));

    if (level == 0) {
        cpu_reset_interrupt(cs, CPU_INTERRUPT_HARD);
    }
    else {
        // TODO: get next exception by priority
        interrupt = 0;
        while ((interrupt = find_next_bit(env->irq_pending, FR_USER_INTERRUPTS, interrupt)) != FR_USER_INTERRUPTS) {
            fr_check_pic(env, interrupt);

            interrupt++;
        }
    }
}

static void fr_mb91637_init(MachineState *machine)
{
    FRCPU *cpu;
    DeviceState *dev;
    MemoryRegion *address_space_mem = get_system_memory();
    MemoryRegion *phys_ram = g_new(MemoryRegion, 1);
    MemoryRegion *phys_rom = g_new(MemoryRegion, 1);
    MemoryRegion *io = g_new(MemoryRegion, 1);
    ram_addr_t ram_base = 0x34000;
    ram_addr_t ram_size = 0xc000;
    ram_addr_t rom_base = 0x80000;
    ram_addr_t rom_size = 0x80000;
    qemu_irq cpu_irq;

    memory_region_init_ram(phys_ram, NULL, "mb91637.ram", ram_size,
                           &error_abort);
    memory_region_add_subregion(address_space_mem, ram_base, phys_ram);

    memory_region_init_rom(phys_rom, NULL, "mb91637.rom", rom_size,
                           &error_abort);
    memory_region_add_subregion(address_space_mem, rom_base, phys_rom);

    all_regs.TMRL[0] = ptimer_init(reload_timer0_hit, &all_regs, PTIMER_POLICY_DEFAULT);
    all_regs.TMRL[1] = ptimer_init(reload_timer1_hit, &all_regs, PTIMER_POLICY_DEFAULT);
    all_regs.TMRL[2] = ptimer_init(reload_timer2_hit, &all_regs, PTIMER_POLICY_DEFAULT);
    // TODO: all_regs.irq_TMRL = qemu_allocate_irqs(reload_timer_handler, cpu, 20);

    memory_region_init_io(io, NULL, &iomem_ops, &all_regs,
                          "mb91637.io", 0x1000);
    memory_region_add_subregion(address_space_mem, 0x0, io);

    cpu = FR_CPU(cpu_create(TYPE_FR_CPU));
    all_regs.cpu = cpu;

    cpu_irq = qemu_allocate_irq(fr_pic_cpu_handler, cpu, 0);

    dev = qdev_create(NULL, "fr80,eit");
    qdev_prop_set_uint32(dev, "len-batch-interrupt", 8);
    qdev_prop_set_uint32(dev, "batch-interrupt[0]", (20<<16)|0xE000);
    qdev_prop_set_uint32(dev, "batch-interrupt[1]", (39<<16)|0xEEEE);
    qdev_prop_set_uint32(dev, "batch-interrupt[2]", (40<<16)|0xF000);
    qdev_prop_set_uint32(dev, "batch-interrupt[3]", (41<<16)|0xE000);
    qdev_prop_set_uint32(dev, "batch-interrupt[4]", (44<<16)|0xF000);
    qdev_prop_set_uint32(dev, "batch-interrupt[5]", (37<<16)|0xF800);
    qdev_prop_set_uint32(dev, "batch-interrupt[6]", (45<<16)|0xF000);
    qdev_prop_set_uint32(dev, "batch-interrupt[7]", (38<<16)|0xFC00);
    object_property_add_const_link(OBJECT(dev), "cpu", OBJECT(cpu),
                                   &error_abort);
    qdev_init_nofail(dev);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, cpu_irq);

    all_regs.dicr = qdev_get_gpio_in(dev, 47);
    all_regs.irq = cpu_irq;

    cpu->reset_addr = 0xffffc;
    /* Remove MMU */
    cpu->mmu_present = false;

    fr_load_rom(cpu, rom_base, rom_size);
}

static void fr_mb91637_machine_init(struct MachineClass *mc)
{
    mc->desc = "MB91637 CPU"; // CPU is MB91F637
    mc->init = fr_mb91637_init;
    mc->is_default = true;
}

DEFINE_MACHINE("mb91637", fr_mb91637_machine_init);

/*
 * QEMU FR CPU
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
#include "qemu/module.h"
#include "qapi/error.h"
#include "cpu.h"
#include "exec/log.h"
#include "exec/cpu_ldst.h"
#include "exec/gdbstub.h"
#include "hw/qdev-properties.h"

static void fr_cpu_set_pc(CPUState *cs, vaddr value)
{
    FRCPU *cpu = FR_CPU(cs);
    CPUFRState *env = &cpu->env;

    env->regs[R_PC] = value;
}

static bool fr_cpu_has_work(CPUState *cs)
{
    return cs->interrupt_request & (CPU_INTERRUPT_HARD | CPU_INTERRUPT_NMI);
}

static void fr_cpu_reset(DeviceState *dev)
{
    CPUState *cs = CPU(dev);
    FRCPU *cpu = FR_CPU(cs);
    FRCPUClass *ncc = FR_CPU_GET_CLASS(cpu);
    CPUFRState *env = &cpu->env;

    if (qemu_loglevel_mask(CPU_LOG_RESET)) {
        qemu_log("CPU Reset (CPU %d)\n", cs->cpu_index);
        log_cpu_state(cs, 0);
    }

    ncc->parent_reset(dev);

    memset(env->regs, 0, sizeof(uint32_t) * FR_NUM_CORE_REGS);
    env->regs[R_PS] = 0x000F0000;
    env->regs[R_TBR] = 0xFFC00;
}

static void fr_cpu_initfn(Object *obj)
{
    FRCPU *cpu = FR_CPU(obj);

    cpu_set_cpustate_pointers(cpu);

#if !defined(CONFIG_USER_ONLY)
    mmu_init(&cpu->env);
#endif
}

void fr_check_interrupts(CPUFRState *env)
{
#if 0 // TODO:
    if (env->irq_pending) {
        env->irq_pending = 0;
        cpu_interrupt(env_cpu(env), CPU_INTERRUPT_HARD);
    }
#endif
}

static ObjectClass *fr_cpu_class_by_name(const char *cpu_model)
{
    return object_class_by_name(TYPE_FR_CPU);
}

static void fr_cpu_realizefn(DeviceState *dev, Error **errp)
{
    CPUState *cs = CPU(dev);
    FRCPUClass *ncc = FR_CPU_GET_CLASS(dev);
    Error *local_err = NULL;

    cpu_exec_realizefn(cs, &local_err);
    if (local_err != NULL) {
        error_propagate(errp, local_err);
        return;
    }

    qemu_init_vcpu(cs);
    cpu_reset(cs);

    ncc->parent_realize(dev, errp);
}

static bool fr_cpu_exec_interrupt(CPUState *cs, int interrupt_request)
{
    FRCPU *cpu = FR_CPU(cs);
    CPUFRState *env = &cpu->env;

    if (env->dpc) {
        qemu_log_mask(CPU_LOG_INT, "Interrupt in delayslot at pc=%x, index: %d\n", env->regs[R_PC], cs->exception_index);
        return false;
    }

    // TODO: move to fr_pic_cpu_handler()
    if (cs->exception_index >= 16 && cs->exception_index <= 63) {
        uint32_t icr = cpu_ldub_data(env, 0x440+cs->exception_index-16);
        uint32_t ilm = extract32(env->regs[R_PS], R_PS_ILM0_BIT, 5);
        icr |= 0x10;
        if (icr >= ilm) {
            qemu_log_mask(CPU_LOG_INT, "Interrupt masked at pc=%x, index: %d, icr: %x, ilm: %x\n",
                env->regs[R_PC], cs->exception_index, icr, ilm);
            return false;
        }
    }

    if ((env->regs[R_PS]&R_PS_CCR_I) == 0) {
        qemu_log_mask(CPU_LOG_INT, "R_PS_CCR_I is 0. Interrupt disabled at pc=%x, index: %d\n", env->regs[R_PC], cs->exception_index);
        return false;
    }

    if (interrupt_request & CPU_INTERRUPT_HARD) {
        fr_cpu_do_interrupt(cs);
        cs->interrupt_request &= ~CPU_INTERRUPT_HARD;
        return true;
    }
    return false;
}

static void fr_cpu_disas_set_info(CPUState *cpu, disassemble_info *info)
{
    info->print_insn = print_insn_fr;
}

static int fr_cpu_gdb_read_register(CPUState *cs, GByteArray *mem_buf, int n)
{
    FRCPU *cpu = FR_CPU(cs);
    CPUClass *cc = CPU_GET_CLASS(cs);
    CPUFRState *env = &cpu->env;

    if (n > cc->gdb_num_core_regs) {
        return 0;
    }

    if (n < (cc->gdb_num_core_regs - 2)) { /* GP regs and Dedicated regs*/
        return gdb_get_reg32(mem_buf, env->regs[n]);
    }
    else { /* PC and PS */
        return gdb_get_reg32(mem_buf, env->regs[n-(cc->gdb_num_core_regs - 2)+32]);
    }

    /* Invalid regs */
    return 0;
}

static int fr_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    FRCPU *cpu = FR_CPU(cs);
    CPUClass *cc = CPU_GET_CLASS(cs);
    CPUFRState *env = &cpu->env;

    if (n > cc->gdb_num_core_regs) {
        return 0;
    }

    if (n < (cc->gdb_num_core_regs-2)) { /* GP regs and Dedicated regs*/
        env->regs[n] = ldl_p(mem_buf);
    }
    else { /* PC and PS */
        env->regs[n - (cc->gdb_num_core_regs-2) + 32] = ldl_p(mem_buf);
    }
    return 4;
}

static Property fr_properties[] = {
    DEFINE_PROP_BOOL("mmu_present", FRCPU, mmu_present, false),
    DEFINE_PROP_END_OF_LIST(),
};


static void fr_cpu_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    CPUClass *cc = CPU_CLASS(oc);
    FRCPUClass *ncc = FR_CPU_CLASS(oc);

    device_class_set_parent_realize(dc, fr_cpu_realizefn,
                                    &ncc->parent_realize);
    device_class_set_props(dc, fr_properties);
    device_class_set_parent_reset(dc, fr_cpu_reset, &ncc->parent_reset);

    cc->class_by_name = fr_cpu_class_by_name;
    cc->has_work = fr_cpu_has_work;
    cc->do_interrupt = fr_cpu_do_interrupt;
    cc->cpu_exec_interrupt = fr_cpu_exec_interrupt;
    cc->dump_state = fr_cpu_dump_state;
    cc->set_pc = fr_cpu_set_pc;
    cc->disas_set_info = fr_cpu_disas_set_info;
    cc->tlb_fill = fr_cpu_tlb_fill;
#ifndef CONFIG_USER_ONLY
    cc->do_unaligned_access = fr_cpu_do_unaligned_access;
    cc->get_phys_page_debug = fr_cpu_get_phys_page_debug;
#endif
    cc->gdb_read_register = fr_cpu_gdb_read_register;
    cc->gdb_write_register = fr_cpu_gdb_write_register;
    cc->gdb_num_core_regs = (16+6+2);
    cc->tcg_initialize = fr_tcg_init;
}

static const TypeInfo fr_cpu_type_info = {
    .name = TYPE_FR_CPU,
    .parent = TYPE_CPU,
    .instance_size = sizeof(FRCPU),
    .instance_init = fr_cpu_initfn,
    .class_size = sizeof(FRCPUClass),
    .class_init = fr_cpu_class_init,
};

static void fr_cpu_register_types(void)
{
    type_register_static(&fr_cpu_type_info);
}

type_init(fr_cpu_register_types)

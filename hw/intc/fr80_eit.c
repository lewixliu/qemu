/*
 * QEMU FR80 EIT (Exception, Interrupt, Trap) hanlder.
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

#include "hw/irq.h"
#include "hw/sysbus.h"
#include "hw/qdev-properties.h"
#include "cpu.h"
#include "exec/log.h"

#define TYPE_FR80_EIT "fr80,eit"
#define FR80_EIT(obj) \
    OBJECT_CHECK(FR80EITState, (obj), TYPE_FR80_EIT)

typedef struct FR80EITState {
    SysBusDevice  parent_obj;
    void         *cpu;
    qemu_irq      parent_irq;

    uint32_t     *interrupt_batch_register;
    uint32_t      interrupt_batch_count;

} FR80EITState;

static void irq_handler(void *opaque, int irq, int level)
{
    FR80EITState *pv = opaque;
    CPUFRState *env = &((FRCPU *)(pv->cpu))->env;
    if (level) {
        set_bit(irq, env->irq_pending);
    }
    else {
        clear_bit(irq, env->irq_pending);
    }

    if (bitmap_empty(env->irq_pending, FR_USER_INTERRUPTS)) {
        qemu_irq_lower(pv->parent_irq);
    }
    else {
        qemu_irq_raise(pv->parent_irq);
    }
}

static void fr80_eit_init(Object *obj)
{
    FR80EITState *pv = FR80_EIT(obj);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &pv->parent_irq);
    qdev_init_gpio_in(DEVICE(pv), irq_handler, FR_USER_INTERRUPTS);
}

static void fr80_eit_realize(DeviceState *dev, Error **errp)
{
    struct FR80EITState *pv = FR80_EIT(dev);
    Error *err = NULL;
    int i;

    pv->cpu = object_property_get_link(OBJECT(dev), "cpu", &err);
    if (!pv->cpu) {
        error_setg(errp, "fr80,eit: CPU link not found: %s",
                   error_get_pretty(err));
        return;
    }

    qemu_log("interrupt_batch_count: %d\n", pv->interrupt_batch_count); // TODO: Interrupt Request Batch-Read Function
    for (i=0;i<pv->interrupt_batch_count;i++) {
        qemu_log("interrupt_batch_register[%d]=%x\n", i, pv->interrupt_batch_register[i]);
    }
}

static Property fr80_eit_properties[] = {
    DEFINE_PROP_ARRAY("batch-interrupt", FR80EITState, interrupt_batch_count,
                      interrupt_batch_register, qdev_prop_uint32, uint32_t),
    DEFINE_PROP_END_OF_LIST(),
};

static void fr80_eit_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    /* Reason: needs to be wired up, e.g. by fr_mb91637_init() */
    dc->user_creatable = false;
    dc->realize = fr80_eit_realize;
    device_class_set_props(dc, fr80_eit_properties);
}

static TypeInfo fr80_eit_info = {
    .name          = "fr80,eit",
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(FR80EITState),
    .instance_init = fr80_eit_init,
    .class_init    = fr80_eit_class_init,
};

static void fr80_eit_register(void)
{
    type_register_static(&fr80_eit_info);
}

type_init(fr80_eit_register)

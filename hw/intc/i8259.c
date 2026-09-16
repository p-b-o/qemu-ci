/*
 * QEMU 8259 interrupt controller emulation
 *
 * Copyright (c) 2003-2004 Fabrice Bellard
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
#include "hw/intc/i8259.h"
#include "hw/core/irq.h"
#include "hw/core/qdev-properties.h"
#include "hw/isa/isa.h"
#include "qapi/error.h"
#include "qemu/timer.h"
#include "qemu/log.h"
#include "hw/isa/i8259_internal.h"
#include "trace.h"
#include "qom/object.h"

/*#define DEBUG_IRQ_LATENCY*/

#define TYPE_I8259 "isa-i8259"

#define TYPE_I8259_PIC "isa-i8259-pic"

struct I8259PICState {
    DeviceState parent_obj;

    qemu_irq pic_out_irq;
    qemu_irq pass_irqs[ISA_NUM_IRQS];

    ISABus *isabus;
    IRQState i8259_primary_out_irq;
    I8259CommonState i8259[2];
};

OBJECT_DECLARE_SIMPLE_TYPE(I8259PICState, I8259_PIC)


#ifdef DEBUG_IRQ_LATENCY
static int64_t irq_time[16];
#endif
I8259CommonState *isa_pic;
static I8259CommonState *slave_pic;

/* return the highest priority found in mask (highest = smallest
   number). Return 8 if no irq */
static int get_priority(I8259CommonState *s, int mask)
{
    int priority;

    if (mask == 0) {
        return 8;
    }
    priority = 0;
    while ((mask & (1 << ((priority + s->priority_add) & 7))) == 0) {
        priority++;
    }
    return priority;
}

/* return the i8259 wanted interrupt. return -1 if none */
static int i8259_get_irq(I8259CommonState *s)
{
    int mask, cur_priority, priority;

    mask = s->irr & ~s->imr;
    priority = get_priority(s, mask);
    if (priority == 8) {
        return -1;
    }
    /* compute current priority. If special fully nested mode on the
       master, the IRQ coming from the slave is not taken into account
       for the priority computation. */
    mask = s->isr;
    if (s->special_mask) {
        mask &= ~s->imr;
    }
    if (s->special_fully_nested_mode && s->master) {
        mask &= ~(1 << 2);
    }
    cur_priority = get_priority(s, mask);
    if (priority < cur_priority) {
        /* higher priority found: an irq should be generated */
        return (priority + s->priority_add) & 7;
    } else {
        return -1;
    }
}

/* Update INT output. Must be called every time the output may have changed. */
static void i8259_update_irq(I8259CommonState *s)
{
    int irq;

    irq = i8259_get_irq(s);
    if (irq >= 0) {
        trace_pic_update_irq(s->master, s->imr, s->irr, s->priority_add);
        qemu_irq_raise(s->int_out[0]);
    } else {
        qemu_irq_lower(s->int_out[0]);
    }
}

/* set irq level. If an edge is detected, then the IRR is set to 1 */
static void i8259_set_irq(void *opaque, int irq, int level)
{
    I8259CommonState *s = opaque;
    int mask = 1 << irq;
    int irq_index = s->master ? irq : irq + 8;

    trace_pic_set_irq(s->master, irq, level);
    i8259_stat_update_irq(irq_index, level);

#ifdef DEBUG_IRQ_LATENCY
    if (level) {
        irq_time[irq_index] = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    }
#endif

    if (s->ltim || (s->elcr & mask)) {
        /* level triggered */
        if (level) {
            s->irr |= mask;
            s->last_irr |= mask;
        } else {
            s->irr &= ~mask;
            s->last_irr &= ~mask;
        }
    } else {
        /* edge triggered */
        if (level) {
            if ((s->last_irr & mask) == 0) {
                s->irr |= mask;
            }
            s->last_irr |= mask;
        } else {
            s->last_irr &= ~mask;
        }
    }
    i8259_update_irq(s);
}

/* acknowledge interrupt 'irq' */
static void i8259_intack(I8259CommonState *s, int irq)
{
    if (s->auto_eoi) {
        if (s->rotate_on_auto_eoi) {
            s->priority_add = (irq + 1) & 7;
        }
    } else {
        s->isr |= (1 << irq);
    }
    /* We don't clear a level sensitive interrupt here */
    if (!s->ltim && !(s->elcr & (1 << irq))) {
        s->irr &= ~(1 << irq);
    }
    i8259_update_irq(s);
}

int pic_read_irq(I8259CommonState *s)
{
    int irq, intno;

    irq = i8259_get_irq(s);
    if (irq >= 0) {
        int irq2;

        if (irq == 2) {
            irq2 = i8259_get_irq(slave_pic);
            if (irq2 >= 0) {
                i8259_intack(slave_pic, irq2);
            } else {
                /* spurious IRQ on slave controller */
                irq2 = 7;
            }
            intno = slave_pic->irq_base + irq2;
            i8259_intack(s, irq);
            irq = irq2 + 8;
        } else {
            intno = s->irq_base + irq;
            i8259_intack(s, irq);
        }
    } else {
        /* spurious IRQ on host controller */
        irq = 7;
        intno = s->irq_base + irq;
    }

#ifdef DEBUG_IRQ_LATENCY
    printf("IRQ%d latency=%0.3fus\n",
           irq,
           (double)(qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) -
                    irq_time[irq]) * 1000000.0 / NANOSECONDS_PER_SECOND);
#endif

    trace_pic_interrupt(irq, intno);
    return intno;
}

static void i8259_init_reset(I8259CommonState *s)
{
    i8259_common_reset(s);
    i8259_update_irq(s);
}

static void i8259_reset(DeviceState *dev)
{
    I8259CommonState *s = I8259_COMMON(dev);

    s->elcr = 0;
    s->ltim = 0;
    i8259_init_reset(s);
}

static void i8259_base_ioport_write(void *opaque, hwaddr addr64,
                                    uint64_t val64, unsigned size)
{
    I8259CommonState *s = opaque;
    uint32_t addr = addr64;
    uint32_t val = val64;
    int priority, cmd, irq;

    trace_pic_ioport_write(s->master, addr, val);

    if (addr == 0) {
        if (val & 0x10) {
            i8259_init_reset(s);
            s->init_state = 1;
            s->init4 = val & 1;
            s->single_mode = val & 2;
            s->ltim = val & 8;
        } else if (val & 0x08) {
            if (val & 0x04) {
                s->poll = 1;
            }
            if (val & 0x02) {
                s->read_reg_select = val & 1;
            }
            if (val & 0x40) {
                s->special_mask = (val >> 5) & 1;
            }
        } else {
            cmd = val >> 5;
            switch (cmd) {
            case 0:
            case 4:
                s->rotate_on_auto_eoi = cmd >> 2;
                break;
            case 1: /* end of interrupt */
            case 5:
                priority = get_priority(s, s->isr);
                if (priority != 8) {
                    irq = (priority + s->priority_add) & 7;
                    s->isr &= ~(1 << irq);
                    if (cmd == 5) {
                        s->priority_add = (irq + 1) & 7;
                    }
                    i8259_update_irq(s);
                }
                break;
            case 3:
                irq = val & 7;
                s->isr &= ~(1 << irq);
                i8259_update_irq(s);
                break;
            case 6:
                s->priority_add = (val + 1) & 7;
                i8259_update_irq(s);
                break;
            case 7:
                irq = val & 7;
                s->isr &= ~(1 << irq);
                s->priority_add = (irq + 1) & 7;
                i8259_update_irq(s);
                break;
            default:
                /* no operation */
                break;
            }
        }
    } else {
        switch (s->init_state) {
        case 0:
            /* normal mode */
            s->imr = val;
            i8259_update_irq(s);
            break;
        case 1:
            s->irq_base = val & 0xf8;
            s->init_state = s->single_mode ? (s->init4 ? 3 : 0) : 2;
            break;
        case 2:
            if (s->init4) {
                s->init_state = 3;
            } else {
                s->init_state = 0;
            }
            break;
        case 3:
            s->special_fully_nested_mode = (val >> 4) & 1;
            s->auto_eoi = (val >> 1) & 1;
            s->init_state = 0;
            break;
        }
    }
}

static uint64_t i8259_base_ioport_read(void *opaque, hwaddr addr,
                                       unsigned size)
{
    I8259CommonState *s = opaque;
    int ret;

    if (s->poll) {
        ret = i8259_get_irq(s);
        if (ret >= 0) {
            i8259_intack(s, ret);
            ret |= 0x80;
        } else {
            ret = 0;
        }
        s->poll = 0;
    } else {
        if (addr == 0) {
            if (s->read_reg_select) {
                ret = s->isr;
            } else {
                ret = s->irr;
            }
        } else {
            ret = s->imr;
        }
    }
    trace_pic_ioport_read(s->master, addr, ret);
    return ret;
}

int pic_get_output(I8259CommonState *s)
{
    return (i8259_get_irq(s) >= 0);
}

static void i8259_elcr_ioport_write(void *opaque, hwaddr addr,
                                    uint64_t val, unsigned size)
{
    I8259CommonState *s = opaque;
    s->elcr = val & s->elcr_mask;
}

static uint64_t i8259_elcr_ioport_read(void *opaque, hwaddr addr,
                                       unsigned size)
{
    I8259CommonState *s = opaque;
    return s->elcr;
}

static const MemoryRegionOps i8259_base_ioport_ops = {
    .read = i8259_base_ioport_read,
    .write = i8259_base_ioport_write,
    .impl = {
        .min_access_size = 1,
        .max_access_size = 1,
    },
};

static const MemoryRegionOps i8259_elcr_ioport_ops = {
    .read = i8259_elcr_ioport_read,
    .write = i8259_elcr_ioport_write,
    .impl = {
        .min_access_size = 1,
        .max_access_size = 1,
    },
};

static void i8259_realize(DeviceState *dev, Error **errp)
{
    I8259CommonState *s = I8259_COMMON(dev);
    I8259CommonClass *k = I8259_COMMON_GET_CLASS(dev);

    memory_region_init_io(&s->base_io, OBJECT(s), &i8259_base_ioport_ops, s,
                          "pic", 2);
    memory_region_init_io(&s->elcr_io, OBJECT(s), &i8259_elcr_ioport_ops, s,
                          "elcr", 1);

    qdev_init_gpio_out(dev, s->int_out, ARRAY_SIZE(s->int_out));
    qdev_init_gpio_in(dev, i8259_set_irq, 8);

    k->parent_realize(dev, errp);
}

qemu_irq *i8259_init(ISABus *bus, qemu_irq parent_irq_in)
{
    qemu_irq *irq_set;
    DeviceState *dev;
    Object *pic_obj;
    int i;

    irq_set = g_new0(qemu_irq, ISA_NUM_IRQS);

    dev = qdev_new(TYPE_I8259_PIC);
    object_property_set_link(OBJECT(dev), "bus", OBJECT(bus), &error_fatal);
    qdev_realize_and_unref(dev, NULL, &error_fatal);

    qdev_connect_gpio_out(dev, 0, parent_irq_in);
    for (i = 0 ; i < ISA_NUM_IRQS; i++) {
        irq_set[i] = qdev_get_gpio_in(dev, i);
    }

    pic_obj = object_resolve_path_component(OBJECT(dev), "primary");
    isa_pic = I8259_COMMON(pic_obj);
    pic_obj = object_resolve_path_component(OBJECT(dev), "secondary");
    slave_pic = I8259_COMMON(pic_obj);

    return irq_set;
}

static void i8259_class_init(ObjectClass *klass, const void *data)
{
    I8259CommonClass *k = I8259_COMMON_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_parent_realize(dc, i8259_realize, &k->parent_realize);
    device_class_set_legacy_reset(dc, i8259_reset);
}


static void i8259_pic_set_irq(void *opaque, int n, int level)
{
    I8259PICState *s = opaque;

    qemu_set_irq(s->pass_irqs[n], level);
}

static void i8259_primary_out_irq(void *opaque, int n, int level)
{
    I8259PICState *s = opaque;

    qemu_set_irq(s->pic_out_irq, level);
}

static void i8259_pic_init(Object *obj)
{
    I8259PICState *s = I8259_PIC(obj);

    object_initialize_child(obj, "primary", &s->i8259[0], TYPE_I8259);
    object_initialize_child(obj, "secondary", &s->i8259[1], TYPE_I8259);

    qemu_init_irq(&s->i8259_primary_out_irq, i8259_primary_out_irq, s, 1);

    qdev_init_gpio_in(DEVICE(obj), i8259_pic_set_irq, ISA_NUM_IRQS);
    qdev_init_gpio_out(DEVICE(obj), &s->pic_out_irq, 1);
}

static void i8259_pic_realize(DeviceState *dev, Error **errp)
{
    I8259PICState *s = I8259_PIC(dev);
    DeviceState *pri_dev, *sec_dev;
    int i;

    /* Primary */
    pri_dev = DEVICE(&s->i8259[0]);
    qdev_prop_set_uint32(pri_dev, "iobase", 0x20);
    qdev_prop_set_uint32(pri_dev, "elcr_addr", 0x4d0);
    qdev_prop_set_uint8(pri_dev, "elcr_mask", 0xf8);
    qdev_prop_set_bit(pri_dev, "master", 1);
    if (!isa_realize_and_unref(ISA_DEVICE(pri_dev), s->isabus, errp)) {
        return;
    }

    for (i = 0; i < 8; i++) {
        s->pass_irqs[i] = qdev_get_gpio_in(pri_dev, i);
    }

    qdev_connect_gpio_out(pri_dev, 0, &s->i8259_primary_out_irq);

    /* Secondary */
    sec_dev = DEVICE(&s->i8259[1]);
    qdev_prop_set_uint32(sec_dev, "iobase", 0xa0);
    qdev_prop_set_uint32(sec_dev, "elcr_addr", 0x4d1);
    qdev_prop_set_uint8(sec_dev, "elcr_mask", 0xde);
    if (!isa_realize_and_unref(ISA_DEVICE(sec_dev), s->isabus, errp)) {
        return;
    }

    /* Wire up secondary cascade */
    qdev_connect_gpio_out(sec_dev, 0,
                          qdev_get_gpio_in(pri_dev, 2));

    for (i = 8; i < ISA_NUM_IRQS; i++) {
        s->pass_irqs[i] = qdev_get_gpio_in(sec_dev, i - 8);
    }
}

static const Property i8259_pic_properties[] = {
    DEFINE_PROP_LINK("bus", I8259PICState, isabus, TYPE_ISA_BUS,
                     ISABus *),
};

static void i8259_pic_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = i8259_pic_realize;
    device_class_set_props(dc, i8259_pic_properties);
    /*
     * Reason: must be wired to the ISA bus via the "bus" property
     */
    dc->user_creatable = false;
}

static const TypeInfo i8259_type_infos[] = {
    {
        .name       = TYPE_I8259,
        .parent     = TYPE_I8259_COMMON,
        .class_init = i8259_class_init,
    },
    {
        .name          = TYPE_I8259_PIC,
        .parent        = TYPE_DEVICE,
        .class_init    = i8259_pic_class_init,
        .instance_init = i8259_pic_init,
        .instance_size = sizeof(I8259PICState),
    },
};

DEFINE_TYPES(i8259_type_infos)

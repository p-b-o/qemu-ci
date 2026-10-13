/*
 * I2C multiplexer for PCA954x series of I2C multiplexer/switch chips.
 *
 * Copyright 2021 Google LLC
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/i2c/i2c.h"
#include "hw/i2c/i2c_mux_pca954x.h"
#include "hw/i2c/smbus_slave.h"
#include "hw/core/qdev.h"
#include "hw/core/qdev-properties.h"
#include "hw/core/sysbus.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/queue.h"
#include "qom/object.h"
#include "trace.h"

#define PCA9548_CHANNEL_COUNT 8
#define PCA9546_CHANNEL_COUNT 4
#define PCA9544_CHANNEL_COUNT 4

/* enable bit */
#define PCA9544_ENABLE 0x4

/*
 * struct Pca954xState - The pca954x state object.
 * @control: The value written to the mux control.
 * @channel: The set of i2c channel buses that act as channels which own the
 * i2c children.
 */
typedef struct Pca954xState {
    SMBusDevice parent;

    uint8_t control;

    bool enabled[PCA9548_CHANNEL_COUNT];
    I2CBus *bus[PCA9548_CHANNEL_COUNT];

    char *name;
} Pca954xState;

/*
 * struct Pca954xClass - The pca954x class object.
 * @nchans: The number of i2c channels this device has.
 */
typedef struct Pca954xClass {
    SMBusDeviceClass parent;

    uint8_t nchans;

    /* channel enable bit for mux devices, zero for switches */
    uint8_t mux_enable;

    int (*parent_event)(I2CSlave *s, enum i2c_event event);
} Pca954xClass;

#define TYPE_PCA954X "pca954x"
OBJECT_DECLARE_TYPE(Pca954xState, Pca954xClass, PCA954X)

/*
 * For each channel, if it's enabled, recursively call match on those children.
 */
static bool pca954x_match(I2CSlave *candidate, uint8_t address,
                          bool broadcast,
                          I2CNodeList *current_devs)
{
    Pca954xState *mux = PCA954X(candidate);
    Pca954xClass *mc = PCA954X_GET_CLASS(mux);
    int i;

    /* They are talking to the mux itself (or all devices enabled). */
    if ((candidate->address == address) || broadcast) {
        I2CNode *node = g_new(struct I2CNode, 1);
        node->elt = candidate;
        QLIST_INSERT_HEAD(current_devs, node, next);
        if (!broadcast) {
            return true;
        }
    }

    for (i = 0; i < mc->nchans; i++) {
        if (!mux->enabled[i]) {
            continue;
        }

        if (i2c_scan_bus(mux->bus[i], address, broadcast,
                         current_devs)) {
            if (!broadcast) {
                return true;
            }
        }
    }

    /* If we arrived here we didn't find a match, return broadcast. */
    return broadcast;
}

static void pca954x_enable_channel(Pca954xState *s, uint8_t control)
{
    Pca954xClass *mc = PCA954X_GET_CLASS(s);
    int i;

    if (mc->mux_enable) {
        uint8_t selected = control & (mc->mux_enable - 1);
        bool enabled = control & mc->mux_enable;

        for (i = 0; i < mc->nchans; i++) {
            s->enabled[i] = enabled && i == selected;
        }
        return;
    }

    /*
     * For each channel, check if their bit is set in control and if yes,
     * enable it, otherwise disable, hide it.
     */
    for (i = 0; i < mc->nchans; i++) {
        s->enabled[i] = control & (1 << i);
    }
}

static void pca954x_write(Pca954xState *s, uint8_t data)
{
    Pca954xClass *mc = PCA954X_GET_CLASS(s);
    uint8_t data_writable = data;

    if (mc->mux_enable) {
        data_writable = data & 0x0f;
    }

    s->control = data_writable;
    if (!mc->mux_enable) {
        pca954x_enable_channel(s, data_writable);
    }

    trace_pca954x_write_bytes(data);
}

static int pca954x_write_data(SMBusDevice *d, uint8_t *buf, uint8_t len)
{
    Pca954xState *s = PCA954X(d);
    Pca954xClass *mc = PCA954X_GET_CLASS(s);

    if (len == 0) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: writing empty data\n", __func__);
        return -1;
    }

    /*
     * len should be 1, because they write one byte to enable/disable channels.
     * In case of PCA9544A, multiple bytes can be written but only the last one
     * will be used.
     */
    if (len > 1 && !mc->mux_enable) {
        qemu_log_mask(LOG_GUEST_ERROR,
            "%s: extra data after channel selection mask\n",
            __func__);
        return -1;
    }

    pca954x_write(s, buf[len - 1]);
    return 0;
}

static uint8_t pca954x_read_byte(SMBusDevice *d)
{
    Pca954xState *s = PCA954X(d);
    uint8_t data = s->control;
    trace_pca954x_read_data(data);
    return data;
}

static int pca954x_event(I2CSlave *s, enum i2c_event event)
{
    Pca954xState *state = PCA954X(s);
    Pca954xClass *mc = PCA954X_GET_CLASS(state);

    int res = mc->parent_event(s, event);

    /*
     * When a channel is selected, it will become active after
     * a STOP condition has been placed on the I2C-bus.
     */
    if (mc->mux_enable && event == I2C_FINISH) {
        pca954x_enable_channel(state, state->control);
    }
    return res;
}

static void pca954x_enter_reset(Object *obj, ResetType type)
{
    Pca954xState *s = PCA954X(obj);
    Pca954xClass *mc = PCA954X_GET_CLASS(s);
    /* Reset will disable all channels. */
    pca954x_write(s, 0);

    if (mc->mux_enable) {
        pca954x_enable_channel(s, s->control);
    }
}

I2CBus *pca954x_i2c_get_bus(I2CSlave *mux, uint8_t channel)
{
    Pca954xClass *pc = PCA954X_GET_CLASS(mux);
    Pca954xState *pca954x = PCA954X(mux);

    g_assert(channel < pc->nchans);
    return pca954x->bus[channel];
}

static void pca9544_class_init(ObjectClass *klass, const void *data)
{
    Pca954xClass *s = PCA954X_CLASS(klass);
    s->nchans = PCA9544_CHANNEL_COUNT;
    s->mux_enable = PCA9544_ENABLE;
}

static void pca9546_class_init(ObjectClass *klass, const void *data)
{
    Pca954xClass *s = PCA954X_CLASS(klass);
    s->nchans = PCA9546_CHANNEL_COUNT;
}

static void pca9548_class_init(ObjectClass *klass, const void *data)
{
    Pca954xClass *s = PCA954X_CLASS(klass);
    s->nchans = PCA9548_CHANNEL_COUNT;
}

static void pca954x_realize(DeviceState *dev, Error **errp)
{
    Pca954xState *s = PCA954X(dev);
    DeviceState *d = DEVICE(s);
    if (s->name) {
        d->id = g_strdup(s->name);
    } else {
        d->id = g_strdup_printf("pca954x[%x]", s->parent.i2c.address);
    }
}

static void pca954x_init(Object *obj)
{
    Pca954xState *s = PCA954X(obj);
    Pca954xClass *c = PCA954X_GET_CLASS(obj);
    int i;

    /* SMBus modules. Cannot fail. */
    for (i = 0; i < c->nchans; i++) {
        g_autofree gchar *bus_name = g_strdup_printf("i2c.%d", i);

        /* start all channels as disabled. */
        s->enabled[i] = false;
        s->bus[i] = i2c_init_bus(DEVICE(s), bus_name);
    }
}

static const Property pca954x_props[] = {
    DEFINE_PROP_STRING("name", Pca954xState, name),
};

static void pca954x_class_init(ObjectClass *klass, const void *data)
{
    I2CSlaveClass *sc = I2C_SLAVE_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);
    SMBusDeviceClass *k = SMBUS_DEVICE_CLASS(klass);
    Pca954xClass *s = PCA954X_CLASS(klass);

    /* hook our parent event handler */
    s->parent_event = sc->event;

    sc->match_and_add = pca954x_match;
    sc->event = pca954x_event;

    rc->phases.enter = pca954x_enter_reset;

    dc->desc = "Pca954x i2c-mux";
    dc->realize = pca954x_realize;

    k->write_data = pca954x_write_data;
    k->receive_byte = pca954x_read_byte;

    device_class_set_props(dc, pca954x_props);
}

static const TypeInfo pca954x_info[] = {
    {
        .name          = TYPE_PCA954X,
        .parent        = TYPE_SMBUS_DEVICE,
        .instance_size = sizeof(Pca954xState),
        .instance_init = pca954x_init,
        .class_size    = sizeof(Pca954xClass),
        .class_init    = pca954x_class_init,
        .abstract      = true,
    },
    {
        .name          = TYPE_PCA9544,
        .parent        = TYPE_PCA954X,
        .class_init    = pca9544_class_init,
    },
    {
        .name          = TYPE_PCA9546,
        .parent        = TYPE_PCA954X,
        .class_init    = pca9546_class_init,
    },
    {
        .name          = TYPE_PCA9548,
        .parent        = TYPE_PCA954X,
        .class_init    = pca9548_class_init,
    },
};

DEFINE_TYPES(pca954x_info)

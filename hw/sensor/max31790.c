/*
 * Maxim MAX31790 PMBus 6-Channel Fan Controller
 *
 * Datasheet:
 * https://www.analog.com/media/en/technical-documentation/data-sheets/MAX31790.pdf
 *
 * Copyright 2026 9elements
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/i2c/i2c.h"
#include "hw/sensor/max31790.h"
#include "migration/vmstate.h"
#include "qemu/module.h"
#include "qom/object.h"
#include "trace.h"

#define MAX31790_NUM_FANS 6
#define MAX31790_NUM_TACHS 12

#define MAX31790_REG_GLOBAL_CONFIG 0x00
#define MAX31790_REG_PWM_FREQ 0x01

/* 0x02 to 0x07: N = 0 .. 5 */
#define MAX31790_REG_FAN_CONFIG(N) (0x02 + (N))

/* 0x08 to 0x0d: N = 0 .. 5 */
#define MAX31790_REG_FAN_DYNAMICS(N) (0x08 + (N))

#define MAX31790_REG_FAN_FAULT_STATUS_2 0x10
#define MAX31790_REG_FAN_FAULT_STATUS_1 0x11
#define MAX31790_REG_FAN_FAULT_MASK_2 0x12
#define MAX31790_REG_FAN_FAULT_MASK_1 0x13
#define MAX31790_REG_FAILED_FAN_OPT 0x14

/* 0x18 to 0x2f: N = 0 .. 11 */
#define MAX31790_REG_TACH_COUNT_MSB(N) (0x18 + 2 * (N))
#define MAX31790_REG_TACH_COUNT_LSB(N) (0x19 + 2 * (N))

/* 0x30 to 0x3b: N = 0 .. 5 */
#define MAX31790_REG_PWM_DUTY_CYCLE_MSB(N) (0x30 + 2 * (N))
#define MAX31790_REG_PWM_DUTY_CYCLE_LSB(N) (0x31 + 2 * (N))

/* .. reserved registers ... */

/* 0x40 to 0x4b: N = 0 .. 5 */
#define MAX31790_REG_PWM_TARGET_DUTY_CYCLE_MSB(N) (0x40 + 2 * (N))
#define MAX31790_REG_PWM_TARGET_DUTY_CYCLE_LSB(N) (0x41 + 2 * (N))

/* ... 'User Byte' registers ... */

/* 0x50 to 0x5b: N = 0 .. 5 */
#define MAX31790_REG_TACH_TARGET_COUNT_MSB(N) (0x50 + 2 * (N))
#define MAX31790_REG_TACH_TARGET_COUNT_LSB(N) (0x51 + 2 * (N))

static inline uint8_t lsb16(uint16_t v) { return extract16(v, 0, 8); }

static inline uint8_t msb16(uint16_t v) { return extract16(v, 8, 8); }

struct MAX31790State {
    I2CSlave i2c;

    uint8_t fan_config[MAX31790_NUM_FANS];
    uint8_t fan_dynamics[MAX31790_NUM_FANS];

    uint16_t pwm[MAX31790_NUM_FANS];
    uint16_t tach_target[MAX31790_NUM_FANS];

    /*
     * Fan speed is measured by counting the number of internal 8192Hz
     * (fTOSC/4) clock cycles that take place during a select-
     * able number of tachometer periods. The number of clock
     * cycles counted (11-bit value) is stored in the associated
     * TACH Count registers and the desired number of cycles
     * is stored in the TACH Target Count registers.
     */
    uint16_t tach_count[MAX31790_NUM_TACHS];

    /* command buffer */
    uint8_t len;
    uint8_t buf[2];

    /* output buffer */
    uint8_t outlen;
    uint8_t outbuf[2];

    /* selected register for read/write operation */
    uint8_t pointer;
};

OBJECT_DECLARE_SIMPLE_TYPE(MAX31790State, MAX31790)

/* helper function for max31790 register layout */
static size_t reg16_index(uint8_t pointer, uint8_t base)
{
    /* even registers have even base address */
    /* MSB is always stored at +1 byte offset*/
    g_assert(pointer >= base);
    g_assert(base % 2 == 0);

    if (pointer % 2 == 0) {
        return (pointer - base) / 2;
    } else {
        return (pointer - (base + 1)) / 2;
    }
}

static void set_outbuf_16(MAX31790State *s, bool even, uint16_t value)
{

    if (even) {
        s->outbuf[0] = msb16(value);
        s->outbuf[1] = lsb16(value);
    } else {
        s->outbuf[0] = lsb16(value);
    }
}

static void max31790_read(MAX31790State *s)
{
    size_t index = 0;
    const bool even_reg = s->pointer % 2 == 0;

    s->outbuf[0] = 0;
    s->outbuf[1] = 0;

    switch (s->pointer) {
    case MAX31790_REG_FAN_CONFIG(0)... MAX31790_REG_FAN_CONFIG(5):
        index = s->pointer - MAX31790_REG_FAN_CONFIG(0);

        g_assert(index < MAX31790_NUM_FANS);

        s->outbuf[0] = s->fan_config[index];
        trace_max31790_fan_config_read(s->i2c.address, index, s->outbuf[0]);
        break;
    case MAX31790_REG_FAN_DYNAMICS(0)... MAX31790_REG_FAN_DYNAMICS(5):
        index = s->pointer - MAX31790_REG_FAN_DYNAMICS(0);

        g_assert(index < MAX31790_NUM_FANS);

        s->outbuf[0] = s->fan_dynamics[index];
        trace_max31790_fan_dynamics_read(s->i2c.address, index, s->outbuf[0]);
        break;
    case MAX31790_REG_FAN_FAULT_STATUS_1:
    case MAX31790_REG_FAN_FAULT_STATUS_2:
        /* we do not have any fan fault */
        s->outbuf[0] = 0x00;
        s->outbuf[1] = 0x00;
        break;
    case MAX31790_REG_TACH_COUNT_MSB(0)... MAX31790_REG_TACH_COUNT_LSB(11):
        index = reg16_index(s->pointer, MAX31790_REG_TACH_COUNT_MSB(0));

        g_assert(index < MAX31790_NUM_TACHS);

        set_outbuf_16(s, even_reg, s->tach_count[index % MAX31790_NUM_TACHS]);
        break;

    case MAX31790_REG_PWM_DUTY_CYCLE_MSB(0)... MAX31790_REG_PWM_DUTY_CYCLE_LSB(
        5):
        index = reg16_index(s->pointer, MAX31790_REG_PWM_DUTY_CYCLE_MSB(0));

        g_assert(index < MAX31790_NUM_FANS);

        set_outbuf_16(s, even_reg, s->pwm[index & MAX31790_NUM_FANS]);
        break;

    case MAX31790_REG_PWM_TARGET_DUTY_CYCLE_MSB(
        0)... MAX31790_REG_PWM_TARGET_DUTY_CYCLE_LSB(5):
        index =
            reg16_index(s->pointer, MAX31790_REG_PWM_TARGET_DUTY_CYCLE_MSB(0));

        g_assert(index < MAX31790_NUM_FANS);

        set_outbuf_16(s, even_reg, s->pwm[index & MAX31790_NUM_FANS]);
        break;

    case MAX31790_REG_TACH_TARGET_COUNT_MSB(
        0)... MAX31790_REG_TACH_TARGET_COUNT_LSB(5):
        index = reg16_index(s->pointer, MAX31790_REG_TACH_TARGET_COUNT_MSB(0));

        g_assert(index < MAX31790_NUM_FANS);

        set_outbuf_16(s, even_reg, s->tach_target[index]);
        break;
    default:
        qemu_log_mask(LOG_UNIMP, "%s: read of register %d\n", __func__,
                      s->pointer);
        break;
    }
}

static uint8_t max31790_get_speed_range(MAX31790State *s, size_t index)
{
    g_assert(index < MAX31790_NUM_FANS);
    const uint16_t value = (s->fan_dynamics[index] >> 5);
    return (value > 5) ? 32 : 1 << value;
}

static void max31790_set_rpm(MAX31790State *s, size_t index, uint16_t rpm)
{
    trace_max31790_rpm_write(s->i2c.address, index, rpm);

    /*
     * (see page 11) NP = number of tachometer pulses per revolution.
     * Most general-purpose brushless DC fans produce two tachometer
     * pulses per revolution.
     */
    const uint64_t sr = max31790_get_speed_range(s, index);

    /* page 11: tach_count = 60 * SR * 8192 / (NP * RPM) */

    /* prevent division by 0 */
    rpm = (rpm == 0) ? 1 : rpm;

    /* (60 * 8192) / 2 */
    const uint64_t tach_count_constant = 245760ULL;

    /* simplified, assuming NP = 2 */
    uint64_t tach_count = (tach_count_constant * sr) / (uint64_t)rpm;

    /* in case of overflow, clamp to the maximum value */
    if (tach_count > 0x7ff) {
        tach_count = 0x7ff;
    }

    g_assert(index < MAX31790_NUM_TACHS);

    /* datasheet: lowest 5 bits are 0 */
    s->tach_count[index] = (uint16_t)(tach_count << 5);

    trace_max31790_tach_count_reg(s->i2c.address, index, s->tach_count[index]);
}

static void max31790_pwm_write(MAX31790State *s, size_t index, uint16_t value)
{
    trace_max31790_pwm_write(s->i2c.address, index, value);

    g_assert(index < MAX31790_NUM_FANS);

    s->pwm[index % MAX31790_NUM_FANS] = value;

    /* last 7 bits in register are 0 , 9 bits remaining */
    const uint16_t pwm_real = s->pwm[index % MAX31790_NUM_FANS] >> 7;

    /* maximum value will be 511 */

    /*
     * This formula has magic values which model the relationship
     * of PWM input to a fan. Not derived from datasheet.
     */
    max31790_set_rpm(s, index, 100 + pwm_real * 20);
}

static void max31790_write_2_byte(MAX31790State *s)
{
    size_t index = 0;
    const uint8_t value0 = s->buf[0];
    const uint8_t value1 = s->buf[1];

    switch (s->pointer) {
    case MAX31790_REG_FAN_CONFIG(0)... MAX31790_REG_FAN_CONFIG(5):
        break; /* handled by one byte write */
    case MAX31790_REG_FAN_DYNAMICS(0)... MAX31790_REG_FAN_DYNAMICS(5):
        break; /* handled by one byte write */
    case MAX31790_REG_PWM_TARGET_DUTY_CYCLE_MSB(0):
    case MAX31790_REG_PWM_TARGET_DUTY_CYCLE_MSB(1):
    case MAX31790_REG_PWM_TARGET_DUTY_CYCLE_MSB(2):
    case MAX31790_REG_PWM_TARGET_DUTY_CYCLE_MSB(3):
    case MAX31790_REG_PWM_TARGET_DUTY_CYCLE_MSB(4):
    case MAX31790_REG_PWM_TARGET_DUTY_CYCLE_MSB(5):
        index =
            reg16_index(s->pointer, MAX31790_REG_PWM_TARGET_DUTY_CYCLE_MSB(0));
        max31790_pwm_write(s, index, value0 << 8 | value1);
        break;
    case MAX31790_REG_TACH_TARGET_COUNT_MSB(0):
    case MAX31790_REG_TACH_TARGET_COUNT_MSB(1):
    case MAX31790_REG_TACH_TARGET_COUNT_MSB(2):
    case MAX31790_REG_TACH_TARGET_COUNT_MSB(3):
    case MAX31790_REG_TACH_TARGET_COUNT_MSB(4):
    case MAX31790_REG_TACH_TARGET_COUNT_MSB(5):
        index = reg16_index(s->pointer, MAX31790_REG_TACH_TARGET_COUNT_MSB(0));

        g_assert(index < MAX31790_NUM_FANS);

        s->tach_target[index] = (value0 << 8) | value1;
        break;
    default:
        qemu_log_mask(LOG_UNIMP, "%s: write to register %d\n", __func__,
                      s->pointer);
        break;
    }
}

static uint16_t reg16_write_byte(uint16_t value, bool is_msb, uint8_t b)
{

    if (is_msb) {
        return (b << 8) | (value & 0x00ff);
    } else {
        return (value & 0xff00) | (b & 0x00ff);
    }
}

static void max31790_write_1_byte(MAX31790State *s)
{
    size_t index = 0;
    uint16_t pwm = 0;
    const uint8_t value = s->buf[0];
    const bool is_msb = s->pointer % 2 == 0;

    switch (s->pointer) {
    case MAX31790_REG_FAN_CONFIG(0)... MAX31790_REG_FAN_CONFIG(5):
        index = s->pointer - MAX31790_REG_FAN_CONFIG(0);

        g_assert(index < MAX31790_NUM_FANS);

        s->fan_config[index] = value;
        trace_max31790_fan_config_write(s->i2c.address, index, value);
        break;
    case MAX31790_REG_FAN_DYNAMICS(0)... MAX31790_REG_FAN_DYNAMICS(5):
        index = s->pointer - MAX31790_REG_FAN_DYNAMICS(0);

        g_assert(index < MAX31790_NUM_FANS);

        s->fan_dynamics[index] = value;
        trace_max31790_fan_dynamics_write(s->i2c.address, index, value);
        break;
    case MAX31790_REG_PWM_TARGET_DUTY_CYCLE_MSB(
        0)... MAX31790_REG_PWM_TARGET_DUTY_CYCLE_LSB(5):
        index =
            reg16_index(s->pointer, MAX31790_REG_PWM_TARGET_DUTY_CYCLE_MSB(0));

        g_assert(index < MAX31790_NUM_FANS);

        pwm =
            reg16_write_byte(s->pwm[index & MAX31790_NUM_FANS], is_msb, value);

        max31790_pwm_write(s, index, pwm);
        break;
    case MAX31790_REG_TACH_TARGET_COUNT_MSB(
        0)... MAX31790_REG_TACH_TARGET_COUNT_LSB(5):
        index = reg16_index(s->pointer, MAX31790_REG_TACH_TARGET_COUNT_MSB(0));

        g_assert(index < MAX31790_NUM_FANS);

        s->tach_target[index] =
            reg16_write_byte(s->tach_target[index], is_msb, value);
        break;
    default:
        qemu_log_mask(LOG_UNIMP, "%s: write to register %d\n", __func__,
                      s->pointer);
        break;
    }
}

static int max31790_send(I2CSlave *i2c, uint8_t data)
{
    MAX31790State *s = MAX31790(i2c);

    trace_max31790_send(s->i2c.address, data);

    if (s->len == 0) {
        /* first byte is the register pointer for a read / write operation */
        s->pointer = data;
        s->len++;
        return 0;
    }

    if (s->len > 2) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: write too many bytes\n", __func__);
        return 1; /* NAK */
    }

    /* second / third byte is the data to write */
    s->buf[s->len - 1] = data;
    s->len++;

    if (s->len == 2) {
        max31790_write_1_byte(s);
    } else if (s->len == 3) {
        max31790_write_2_byte(s);
    }

    return 0;
}

static uint8_t max31790_recv(I2CSlave *i2c)
{
    MAX31790State *s = MAX31790(i2c);

    max31790_read(s);
    s->len = 0;

    if (s->outlen >= 2) {
        /* error */
        qemu_log_mask(LOG_GUEST_ERROR, "%s: guest over read\n", __func__);
        s->outlen = 0;
    }

    const uint8_t data = s->outbuf[s->outlen++];

    trace_max31790_recv_return(s->i2c.address, s->pointer, data);
    return data;
}

static int max31790_event(I2CSlave *i2c, enum i2c_event event)
{
    MAX31790State *s = MAX31790(i2c);

    switch (event) {
    case I2C_START_RECV:
        s->outlen = 0;
        break;
    case I2C_START_SEND:
        s->len = 0;
        break;
    default:
        break;
    }

    return 0;
}

static const VMStateDescription vmstate_max31790 = {
    .name = TYPE_MAX31790,
    .version_id = 0,
    .minimum_version_id = 0,
    .fields =
        (const VMStateField[]){
            VMSTATE_UINT8(len, MAX31790State),
            VMSTATE_UINT8_ARRAY(fan_config, MAX31790State, MAX31790_NUM_FANS),
            VMSTATE_UINT8_ARRAY(fan_dynamics, MAX31790State, MAX31790_NUM_FANS),
            VMSTATE_UINT16_ARRAY(pwm, MAX31790State, MAX31790_NUM_FANS),
            VMSTATE_UINT16_ARRAY(tach_target, MAX31790State, MAX31790_NUM_FANS),
            VMSTATE_UINT16_ARRAY(tach_count, MAX31790State, MAX31790_NUM_TACHS),
            VMSTATE_UINT8_ARRAY(buf, MAX31790State, 2),
            VMSTATE_UINT8(outlen, MAX31790State),
            VMSTATE_UINT8_ARRAY(outbuf, MAX31790State, 2),
            VMSTATE_UINT8(pointer, MAX31790State),
            VMSTATE_I2C_SLAVE(i2c, MAX31790State), VMSTATE_END_OF_LIST() }
};

static void max31790_reset_enter(MAX31790State *s)
{
    trace_max31790_reset(s->i2c.address);

    for (int i = 0; i < MAX31790_NUM_FANS; i++) {
        /* POR-State 0b 0XX0 0000 */
        s->fan_config[i] = 0x0;

        /* same as POR-State */
        s->tach_target[i] = 0x3c00;

        /* same as POR-State */
        s->fan_dynamics[i] = 0x4c;
    }

    /* this will set both pwm and tach count */
    for (int i = 0; i < MAX31790_NUM_FANS; i++) {
        max31790_pwm_write(s, i, 0x4444);
    }
}

static void max31790_reset_hold(Object *obj, ResetType type)
{
    MAX31790State *s = MAX31790(obj);

    max31790_reset_enter(s);
}

static void max31790_realize(DeviceState *dev, Error **errp)
{
    MAX31790State *s = MAX31790(dev);

    trace_max31790_realize(s->i2c.address);
}

static void max31790_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    I2CSlaveClass *k = I2C_SLAVE_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);

    dc->realize = max31790_realize;
    dc->desc = "Maxim MAX31790 6-Channel Fan Controller";
    dc->vmsd = &vmstate_max31790;
    rc->phases.hold = max31790_reset_hold;
    k->event = max31790_event;
    k->recv = max31790_recv;
    k->send = max31790_send;
}

static const TypeInfo max31790_info = {
    .name = TYPE_MAX31790,
    .parent = TYPE_I2C_SLAVE,
    .instance_size = sizeof(MAX31790State),
    .class_init = max31790_class_init,
};

static void max31790_register_types(void)
{
    type_register_static(&max31790_info);
}

type_init(max31790_register_types)

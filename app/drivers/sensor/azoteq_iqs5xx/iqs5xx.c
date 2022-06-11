
/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT azoteq_iqs5xx

#include <drivers/gpio.h>
#include <init.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <drivers/i2c.h>
#include <init.h>
#include <device.h>
#include "iqs5xx.h"
#include <drivers/sensor.h>
#include <devicetree.h>

// DT_DRV_INST(0);
LOG_MODULE_REGISTER(azoteq_iqs5xx, CONFIG_ZMK_LOG_LEVEL);

// Default config
struct iqs5xx_reg_config iqs5xx_reg_config_default () {
    struct iqs5xx_reg_config regconf;
    regconf.activeRefreshRate =         8;
    regconf.idleRefreshRate =           32;
    regconf.singleFingerGestureMask =   GESTURE_SINGLE_TAP | GESTURE_TAP_AND_HOLD;
    regconf.multiFingerGestureMask =    0;

    return regconf;
}

/**
 * @brief Read from the iqs550 chip via i2c
 * example: iqs5xx_seq_read(dev, GestureEvents0_adr, buffer, 44)
 *
 * @param dev Pointer to device driver MASTER
 * @param start start address for reading
 * @param  pointer to buffer to be read into
 * @param len number of bytes to read
 * @return int
 */
static int iqs5xx_seq_read(const struct device *dev, const uint16_t start, uint8_t *read_buf,
                           const uint8_t len) {
    const struct iqs5xx_data *data = dev->data;
    const struct iqs5xx_config *config = dev->config;
    uint16_t nstart = (start << 8 ) | (start >> 8);
    return i2c_write_read(data->i2c, AZOTEQ_IQS5XX_ADDR, &nstart, sizeof(nstart), read_buf, len);
}

/**
 * @brief Write to the iqs550 chip via i2c
 * example: iqs5xx_write(dev, GestureEvents0_adr, buffer, 44)
 * @param dev Pointer to device driver MASTER
 * @param start address of the i2c slave
 * @param buf Buffer to be written
 * @param len number of bytes to write
 * @return int
 */
static int iqs5xx_write(const struct device *dev, const uint16_t start_addr, uint8_t *buf,
                        uint32_t num_bytes) {

    const struct iqs5xx_data *data = dev->data;

    uint8_t addr_buffer[2];
    struct i2c_msg msg[2];

    addr_buffer[1] = start_addr & 0xFF;
    addr_buffer[0] = start_addr >> 8;
    msg[0].buf = addr_buffer;
    msg[0].len = 2U;
    msg[0].flags = I2C_MSG_WRITE;

    msg[1].buf = (uint8_t *)buf;
    msg[1].len = num_bytes;
    msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

    int err = i2c_transfer(data->i2c, msg, 2, AZOTEQ_IQS5XX_ADDR);
    return err;
}

static int iqs5xx_attr_set(const struct device *dev, enum sensor_channel chan,
                           enum sensor_attribute attr, const struct sensor_value *val) {
	LOG_ERR("\nSetting attributes\n");
    const struct iqs5xx_config *config = dev->config;
    return 0;
}

/**
 * @brief Get sensor channel. Not to be confused with trackpad internal channel.
 *
 * @param dev
 * @param chan
 * @param val
 * @return int
 */
static int iqs5xx_channel_get(const struct device *dev, enum sensor_channel chan,
                              struct sensor_value *val) {
    LOG_ERR("\nCHANNEL GET");

    const struct iqs5xx_data *data = dev->data;
    switch (chan) {
    case SENSOR_CHAN_POS_DX:
        val->val1 = data->rx;
        break;
    case SENSOR_CHAN_POS_DY:
        val->val1 = data->ry;
        break;
    case SENSOR_CHAN_POS_DZ:
        val->val1 = data->gesture;
        break;
    default:
        return -ENOTSUP;
    }
    return 0;
}

struct k_timer tap_release_timer;
void tap_release_handler(struct k_timer *timer) {
    zmk_hid_mouse_button_release(0);
    zmk_endpoints_send_mouse_report();
}

K_TIMER_DEFINE(tap_release_timer, tap_release_handler, NULL);


static int iqs5xx_sample_fetch(const struct device *dev) {
    //LOG_ERR("\nSAMPLE FETCH");

    uint8_t buffer[44];
    int res = iqs5xx_seq_read(dev, GestureEvents0_adr, buffer, 44);
	iqs5xx_write(dev, END_WINDOW, 0, 1);
    if (res < 0) {
        LOG_ERR("\ntrackpad res: %d", res);
        return res;
    }

    struct iqs5xx_data *data = dev->data;
    // define struct to make data harvest easier
    struct iqs5xx_sample {
        int16_t i16RelX[6];
        int16_t i16RelY[6];
        uint16_t ui16AbsX[6];
        uint16_t ui16AbsY[6];
        uint16_t ui16TouchStrength[6];
        uint8_t ui8NoOfFingers;
        uint8_t gesture;
    } d;

    d.ui8NoOfFingers = buffer[4];
    uint8_t i;
    static uint8_t ui8FirstTouch = 0;
    uint8_t ui8NoOfFingers;
    if (d.ui8NoOfFingers != 0) {
        if (!(ui8FirstTouch)) {
            ui8FirstTouch = 1;
        }
        
        // calculate relative data
        d.i16RelX[1] = ((buffer[5] << 8) | (buffer[6]));
        d.i16RelY[1] = ((buffer[7] << 8) | (buffer[8]));

        // calculate absolute position of max 5 fingers
        for (i = 0; i < 5; i++) {
            d.ui16AbsX[i + 1] = ((buffer[(7 * i) + 9] << 8) |
                                 (buffer[(7 * i) + 10])); // 9-16-23-30-37//10-17-24-31-38
            d.ui16AbsY[i + 1] = ((buffer[(7 * i) + 11] << 8) |
                                 (buffer[(7 * i) + 12])); // 11-18-25-32-39//12-19-26-33-40
            d.ui16TouchStrength[i + 1] = ((buffer[(7 * i) + 13] << 8) |
                                          (buffer[(7 * i) + 14])); // 13-20-27-34-11/14-21-28-35-42
            // d.1ui8Area[i+1] = (buffer[7*i+15]); //15-22-29-36-43
        }
    } else {
        ui8FirstTouch = 0;
    }

    // gesture bank 1
    d.gesture = buffer[0];
    // gesture bank 2
    if(buffer[1] > 0)
        d.gesture = buffer[1];


    // set data to device data portion
    data->rx = (int16_t)(int8_t)d.i16RelX[1];
    data->ry = (int16_t)(int8_t)d.i16RelY[1];
    data->ax = d.ui16AbsX[1];
    data->ay = d.ui16AbsY[1];
    data->gesture = d.gesture;

    bool inputChanged = false;

    if(d.gesture & GESTURE_SINGLE_TAP) {
        zmk_hid_mouse_button_press(0);
        inputChanged = true;
        k_timer_start(&tap_release_timer, K_MSEC(50), K_NO_WAIT);
    }
    else if(d.ui8NoOfFingers > 0) {
        zmk_hid_mouse_movement_set(-data->ry, data->rx);
        inputChanged = true;
    }

    if(inputChanged) {
        zmk_endpoints_send_mouse_report();
    }

    return 0;
}

/**
 * @brief Set the interrupt
 *
 * @param dev
 * @param en
 */
static void set_int(const struct device *dev, const bool en) {
	LOG_ERR("\nSetting interrupt\n");
    const struct iqs5xx_config *config = dev->config;
    int ret = gpio_pin_interrupt_configure(config->dr_port, config->dr_pin,
                                           en ? GPIO_INT_LEVEL_ACTIVE : GPIO_INT_DISABLE);
    if (ret < 0) {
        LOG_ERR("\ncan't set interrupt\n");
    }
}

/**
 * @brief set trigger for interrupt
 *
 * @param dev
 * @param trig
 * @param handler
 * @return int
 */
static int iqs5xx_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
                              sensor_trigger_handler_t handler) {
    struct iqs5xx_data *data = dev->data;
    LOG_ERR("\nTRIGGER_SET");

    set_int(dev, false);
    if (trig->type != SENSOR_TRIG_DATA_READY) {
        LOG_ERR("\nENOTSUP");

        return -ENOTSUP;
    }
    LOG_ERR("\nNOT ENOTSUP");

    data->data_ready_trigger = trig;
    data->data_ready_handler = handler;
    set_int(dev, true);
    return 0;
}

static void iqs5xx_thread(void *arg, void *unused2, void *unused3) {
    const struct device *dev = arg;
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);
    struct iqs5xx_data *data = dev->data;
    struct iqs5xx_config *conf = dev->config;
    int nstate = 0;

    while (1) {
        k_sleep(K_MSEC(8));

        // Poll data ready pin
        nstate = gpio_pin_get(conf->dr_port, conf->dr_pin);

        if(nstate) {
            // Fetch the sample
            iqs5xx_sample_fetch(dev);
        }
    }
}

/**
 * @brief Sets registers to initial values
 * 
 * @param dev 
 * @return >0 if error
 */
static int iqs5xx_registers_init (const struct device *dev, const struct iqs5xx_reg_config *config) {
    // TODO: Retry if error on write
    
    int err = 0;

    // 16 or 32 bit values must be swapped to big endian
    uint8_t wbuff[16];

    // Set active refresh rate
    *((uint16_t*)wbuff) = SWPEND16(config->activeRefreshRate);
    err |= iqs5xx_write(dev, ActiveRR_adr, wbuff, 2);
    k_usleep(100);
    // Set idle refresh rate
    *((uint16_t*)wbuff) = SWPEND16(config->idleRefreshRate);
    err |= iqs5xx_write(dev, IdleRR_adr, wbuff, 2);
    k_usleep(100);

    // Set single finger gestures
    err |= iqs5xx_write(dev, SFGestureEnable_adr, &config->singleFingerGestureMask, 2);
    k_usleep(100);
    // Set multi finger gestures
    err |= iqs5xx_write(dev, MFGestureEnable_adr, &config->multiFingerGestureMask, 2);
    k_usleep(100);

    // Terminate transaction
    iqs5xx_write(dev, END_WINDOW, 0, 1);

    return err;
}

K_THREAD_STACK_DEFINE(thread_stack, 2000);
static int iqs5xx_init(const struct device *dev) {
    LOG_DBG("IQS5xx INIT\r\n");
    struct iqs5xx_data *data = dev->data;
    const struct iqs5xx_config *config = dev->config;

    data->dev = dev;
    
    // Configure data ready pin
	gpio_pin_configure(config->dr_port, config->dr_pin, GPIO_INPUT | config->dr_flags);
    
    // Initialize device registers
    struct iqs5xx_reg_config regconf = iqs5xx_reg_config_default();

    int err = iqs5xx_registers_init(dev, &regconf);
    if(err) {
        LOG_ERR("Failed to initialize IQS5xx registers!\r\n");
    }

    return 0;
}

static const struct sensor_driver_api iqs5xx_driver_api = {
    .trigger_set = iqs5xx_trigger_set,
    .sample_fetch = iqs5xx_sample_fetch,
    .channel_get = iqs5xx_channel_get,
    .attr_set = iqs5xx_attr_set,
};

static const struct iqs5xx_data iqs5xx_data = {
    .i2c = DEVICE_DT_GET(DT_BUS(DT_DRV_INST(0))),
};

static const struct iqs5xx_config iqs5xx_config = {
    .dr_port = DEVICE_DT_GET(DT_GPIO_CTLR(DT_DRV_INST(0), dr_gpios)),
    .dr_pin = DT_INST_GPIO_PIN(0, dr_gpios),
    .dr_flags = DT_INST_GPIO_FLAGS(0, dr_gpios),
};

DEVICE_DT_INST_DEFINE(0, iqs5xx_init, NULL, &iqs5xx_data, &iqs5xx_config,
                      POST_KERNEL, 90, &iqs5xx_driver_api);
					  
K_THREAD_DEFINE(thread, 1024, iqs5xx_thread, DEVICE_DT_GET(DT_DRV_INST(0)), NULL, NULL, K_PRIO_COOP(10), 0, 0);
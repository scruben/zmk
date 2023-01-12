#include <zmk/indicator.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

static const struct gpio_dt_spec indicator_led = GPIO_DT_SPEC_GET(DT_ALIAS(indicatorled), gpios);

static enum zmk_indicator_mode indicator_mode = ZMK_INDICATOR_MODE_BLINK;

static uint8_t indicator_led_state = 1;

void zmk_indicator_set (uint8_t on) {

    gpio_pin_set_dt(&indicator_led, on);
    indicator_led_state = on;
    
}

void zmk_indicator_mode (enum zmk_indicator_mode mode) {
    indicator_mode = mode;

    if(mode == ZMK_INDICATOR_MODE_NORMAL) {
        zmk_indicator_set(1);
    }
}

static void _indicator_thread (void *arg, void *unused2, void *unused3) {
    while(1) {
        if(indicator_mode == ZMK_INDICATOR_MODE_BLINK) {
            k_msleep(100);
            gpio_pin_set_dt(&indicator_led, 0);
            k_msleep(2000);
            gpio_pin_set_dt(&indicator_led, indicator_led_state);
        }
        else {
            k_msleep(1000);
        }
    }
}

K_THREAD_DEFINE(indicator_thread, 256, _indicator_thread, NULL, NULL, NULL, K_PRIO_COOP(10), 0, 0);
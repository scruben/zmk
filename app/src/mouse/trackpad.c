/**
 * @file trackpad.c
 * @author Oskari Seppä
 * @brief File to handle iqs5xx data
 * @version 0.1
 * @date 2022-06-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifdef CONFIG_IQS5XX
#include <device.h>
#include <init.h>
#include <drivers/sensor.h>
#include <iqs5xx.h>
#include <logging/log.h>
#include <devicetree.h>

LOG_MODULE_DECLARE(azoteq_iqs5xx, CONFIG_ZMK_LOG_LEVEL);

// Time in ms to release left click after the gesture
#define TRACKPAD_LEFTCLICK_RELEASE_TIME     50
// Time in ms to release right click after the gesture
#define TRACKPAD_RIGHTCLICK_RELEASE_TIME    50
// Scroll speed divider
#define SCROLL_SPEED_DIVIDER                35

static bool isHolding = false;

//LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static const struct device *trackpad;
// Sensor trigger
static struct sensor_trigger trig;

struct k_timer leftclick_release_timer;
static void trackpad_leftclick_release () {
    zmk_hid_mouse_button_release(0);
    zmk_endpoints_send_mouse_report();
}
K_TIMER_DEFINE(leftclick_release_timer, trackpad_leftclick_release, NULL);

struct k_timer rightclick_release_timer;
static void trackpad_rightclick_release () {
    zmk_hid_mouse_button_release(1);
    zmk_endpoints_send_mouse_report();
}
K_TIMER_DEFINE(rightclick_release_timer, trackpad_rightclick_release, NULL);


static inline void trackpad_leftclick () {
    if(isHolding)  {
        zmk_hid_mouse_button_release(0);
        isHolding = false;
    } else {
        zmk_hid_mouse_button_press(0);
        k_timer_start(&leftclick_release_timer, K_MSEC(TRACKPAD_LEFTCLICK_RELEASE_TIME), K_NO_WAIT);
    }
}

static inline void trackpad_rightclick () {
    zmk_hid_mouse_button_press(1);
    k_timer_start(&rightclick_release_timer, K_MSEC(TRACKPAD_RIGHTCLICK_RELEASE_TIME), K_NO_WAIT);
}


static inline void trackpad_tap_and_hold(bool g) {
    if (!isHolding && g) {
        LOG_ERR("!isHolding and G = true");
        zmk_hid_mouse_button_press(0);
        isHolding = true;
    }
}



// Sensor trigger handler
static void trackpad_trigger_handler(const struct device *dev, const struct sensor_trigger *trig) {

    // Fetched sensor value
    struct sensor_value value;

    // X, Y
    int16_t pos_x = 0xFFFF, pos_y = 0xFFFF;
    // Gesture/Z channel
    uint8_t gesture = 0xFF;
    // Fingers
    uint8_t fingers = 0;

    // Fetch X channel
    int err = sensor_channel_get(dev, SENSOR_CHAN_POS_DX, &value);
    if(err) {
        LOG_ERR("Failed to fetch X channel\r\n");
        return;
    }
    // Set pos_x 
    pos_x = value.val1;

    // Fetch Y channel
    err = sensor_channel_get(dev, SENSOR_CHAN_POS_DY, &value);
    if(err) {
        LOG_ERR("Failed to fetch Y channel\r\n");
        return;
    }
    // Set pos_y 
    pos_y = value.val1;

    // Fetch Z/Gesture channel + fingers
    err = sensor_channel_get(dev, SENSOR_CHAN_POS_DZ, &value);
    if(err) {
        LOG_ERR("Failed to fetch gesture channel\r\n");
        return;
    }
    // Set pos_y 
    gesture = value.val1;
    // Set fingers
    fingers = value.val2;

    bool multiTouch = false;
    // Check if msb is high, meaning multi touch
    if(gesture & 0x80) {
        multiTouch = true;
    }

    bool hasGesture = false;
    // Check if any gesture exists
    if(gesture & 0x7F) {

        // Multi touch gestures
        if(multiTouch) {
            switch(gesture & 0x7F) {
                case GESTURE_TWO_FINGER_TAP:
                    // Right click
                    hasGesture = true;
                    trackpad_rightclick();
                    zmk_hid_mouse_movement_set(0,0);
                    break;
                case GESTURE_SCROLLG:
                    hasGesture = true;
                    zmk_hid_mouse_scroll_set(-pos_y/SCROLL_SPEED_DIVIDER, pos_x/SCROLL_SPEED_DIVIDER);
                    zmk_hid_mouse_movement_set(0,0);
                    k_msleep(10);
                    break;
            }
        }
        // Single finger gestures
        else {
            switch(gesture & 0x7F) {
                case GESTURE_SINGLE_TAP:
                    // Left click
                    hasGesture = true;
                    trackpad_leftclick();
                    break;
                case GESTURE_TAP_AND_HOLD:
                    //drag n drop
                    trackpad_tap_and_hold(true);
                    isHolding = true;
            }
        }
    }

    //check for tap and hold release

    bool inputMoved = false;

    if(!hasGesture) {
        // No gesture, can send mouse delta position
        if(fingers == 1) {
            zmk_hid_mouse_movement_set(-pos_y, pos_x);
            inputMoved = true;
        }
    }

    if(inputMoved || hasGesture) {
        // Send mouse report
        zmk_endpoints_send_mouse_report();
    }
}

static int trackpad_init(const struct device *_arg) {
    trackpad = DEVICE_DT_GET_ANY(azoteq_iqs5xx);
    if (trackpad == NULL) {
        LOG_ERR("Failed to get IQS5XX device");
        return -EINVAL;
    }
    int err = 0;
    trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ALL;
	err = sensor_trigger_set(trackpad, &trig, trackpad_trigger_handler);
    if(err) {
        
        return -EINVAL;
    }

    return 0;
}

SYS_INIT(trackpad_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif
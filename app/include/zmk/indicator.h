#pragma once

#include <stdint.h>

enum zmk_indicator_mode {
    ZMK_INDICATOR_MODE_NORMAL,
    ZMK_INDICATOR_MODE_BLINK
};

/**
 * @brief Set indicator state
 * @param on 1 for on, 0 for off
*/
void zmk_indicator_set (uint8_t on);

/**
 * @brief Set indicator mode
 * @param mode See enum zmk_indicator_mode
*/
void zmk_indicator_mode (enum zmk_indicator_mode mode);
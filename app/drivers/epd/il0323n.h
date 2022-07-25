#ifndef _IL0323N_H
#define _IL0323N_H

#include <device.h>


/**
 * @brief Clears an area in the buffer
 * 
 * @param dev 
 * @param x 
 * @param y 
 * @param w 
 * @param h 
 */
static void il0323_clear_area (const struct device *dev, uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief Sets a pixel in the buffer
 * 
 * @param dev 
 * @param x 
 * @param y 
 */
static void il0323_set_pixel (const struct device *dev, uint8_t x, uint8_t y);

/**
 * @brief Clears a pixel in the buffer
 *
 * @param dev 
 * @param x 
 * @param y 
 */
static void il0323_clear_pixel (const struct device *dev, uint8_t x, uint8_t y);

/**
 * @brief Refreshes an area of the screen
 * 
 * @param dev 
 * @param x 
 * @param y 
 * @param w 
 * @param h 
 * @return int 
 */
static int il0323_refresh (struct device *dev, int16_t x, int16_t y, int16_t w, int16_t h);

#endif
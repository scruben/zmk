/*
    Manages device configurations in NVS
*/
#pragma once

#include <kernel.h>
#include <stdint.h>

#define ZMK_CONFIG_MAX_FIELD_SIZE       CONFIG_ZMK_CONFIG_MAX_FIELD_SIZE
#define ZMK_CONFIG_MAX_FIELDS           CONFIG_ZMK_CONFIG_MAX_FIELDS

/**
 * @brief Configuration keys, casted as uint16_t
 * 
 * More fields to be added
 */
enum zmk_config_key {
    // Invalid key
    ZMK_CONFIG_KEY_INVALID =                0x0000,

    // 0x0001 - 0x001F: Misc device config fields
    // Device name for BLE/USB
    ZMK_CONFIG_KEY_DEVICE_NAME =            0x0001,


    // 0x0020 - 0x003F: Keyboard configurations 
    // Keymap
    ZMK_CONFIG_KEY_KEYMAP =                 0x0020,

    // 0x0040 - 0x005F: Mouse/trackpad configurations
    // Mouse sensitivity
    ZMK_CONFIG_KEY_MOUSE_SENSITIVITY =      0x0040,
    // Mouse Y scroll sensitivity
    ZMK_CONFIG_KEY_SCROLL_SENSITIVITY =     0x0041,
    // Mouse X pan sensitivity
    ZMK_CONFIG_KEY_PAN_SENSITIVITY =        0x0042,

};

// Flag if this field has been read from NVS
#define ZMK_CONFIG_FIELD_FLAG_READ        BIT(0)
// Flag if this field has been written to NVS
#define ZMK_CONFIG_FIELD_FLAG_WRITTEN     BIT(1)


/**
 * @brief Configuration field
 * 
 */
struct zmk_config_field {
    // Key identifier (enum zmk_config_key)
    uint16_t key;
    // Bit mask of field flags, see ZMK_CONFIG_FIELD_FLAG_* defs
    uint8_t flags;
    // Mutex lock, if needed for thread safety
    struct k_mutex mutex;
    // Allocated size of the field in bytes
    uint16_t size;
    // Local data, should be initialized to NULL
    void *data;
};


/**
 * @brief Initializes configs and NVS
 * 
 * @return int 
 */
static int zmk_config_init ();

/**
 * @brief Bind a value to config. Automatically reads the value to `data` from NVS when called.
 * 
 * @param key 
 * @param data 
 * @param size 
 * @return struct zmk_config_field* 
 */
static struct zmk_config_field *zmk_config_bind (enum zmk_config_key key, void *data, uint16_t size);

/**
 * @brief Get config field NOTE: does not read from NVS! Use zmk_config_read to update the field
 * 
 * @param key 
 * @return struct zmk_config_field* 
 */
static struct zmk_config_field *zmk_config_get (enum zmk_config_key key);

/**
 * @brief Read field from NVS. If the NVS data length doesn't match with local field's size, NVS data will be rewritten from local memory.
 * @param key
 * @return 0 on success, <0 on fail
 */
static int zmk_config_read (enum zmk_config_key key);

/**
 * @brief Writes a config field to NVS
 * 
 * @param key 
 * @return int 
 */
static int zmk_config_write (enum zmk_config_key key);

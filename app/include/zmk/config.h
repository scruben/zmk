/*
    Manages external configurations, sent via HID USB/BLE and
    interfaces with NVS
*/
#pragma once

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
    ZMK_CONFIG_KEY_INVALID =            0x0000,

    // 0x0001 - 0x001F: Misc device config fields
    // Device name for BLE/USB
    ZMK_CONFIG_KEY_DEVICE_NAME =        0x0001,


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
    // Allocated size of the field in bytes
    uint16_t size;
    // Local data, should be initialized to NULL
    // If data is NULL after loading config, a default value should be returned
    void *data;
};


/**
 * @brief Initializes configs and NVS
 * 
 * @return int 
 */
int zmk_config_init ();

/**
 * @brief Read field from NVS
 * @param key
 * @return Config field or NULL if not found
 */
struct zmk_config_field *zmk_config_read (enum zmk_config_key key);

/**
 * @brief Writes a config field
 * 
 * @param key 
 * @param size 
 * @param data 
 * @param write_nvs Commit to writing to NVS also
 * @return int 
 */
int zmk_config_write (enum zmk_config_key key, uint16_t size, void *data, uint8_t write_nvs);

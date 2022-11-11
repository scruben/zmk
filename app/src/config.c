/*
    Manages external configurations, sent via HID USB/BLE and
    interfaces with NVS
*/
#ifdef CONFIG_ZMK_CONFIG

#include <zmk/config.h>
#include <kernel.h>
#include <device.h>
#include <devicetree.h>
#include <logging/log.h>
#include <drivers/flash.h>
#include <storage/flash_map.h>
#include <fs/nvs.h>
#include <string.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// FS handle
static struct nvs_fs fs;
// Flash info
static struct flash_pages_info flash_info;

#define FIXED_PARTITION_DEVICE(label) \
        DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(DT_NODELABEL(label)))

#define FIXED_PARTITION_OFFSET(label) DT_REG_ADDR(DT_NODELABEL(label))

#define NVS_PARTITION		    storage_partition
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)

// Config initialized flag
static uint8_t _config_initialized = 0;

// Config field buffer
static uint8_t _tmp_buffer[ZMK_CONFIG_MAX_FIELD_SIZE];

// List of read configs
static struct zmk_config_field fields[ZMK_CONFIG_MAX_FIELDS];

int _zmk_config_push_local (enum zmk_config_key key, uint16_t size, void *data);
int _zmk_config_get_local (enum zmk_config_key key);

int zmk_config_init () {
    // No need to initialize multiple times
    if(_config_initialized)
        return 0;

    memset(fields, 0, sizeof(struct zmk_config_field) * ZMK_CONFIG_MAX_FIELDS);

    int err = 0;

    fs.flash_device = NVS_PARTITION_DEVICE;

    if (!device_is_ready(fs.flash_device)) {
		LOG_ERR("Flash device %s is not ready\n", fs.flash_device->name);
		return -1;
	}

    fs.offset = NVS_PARTITION_OFFSET;
	err = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &flash_info);
	if (err) {
		LOG_ERR("Unable to get page info\n");
		return - 1;
	}
	fs.sector_size = flash_info.size;
	fs.sector_count = 3U;

    err = nvs_init(&fs, fs.flash_device->name);
	if (err) {
		LOG_ERR("Flash Init failed\n");
		return - 1;
	}
    // Set initialized flag
    _config_initialized = 1;

    return 0;
}

struct zmk_config_field *zmk_config_read (enum zmk_config_key key) {
    if(!_config_initialized)
        return NULL;

    int index = 0, len = 0;
    // Search from memory before NVS
    if((index = _zmk_config_get_local(key)) >= 0) {
        
        return &fields[index];
    }

    len = nvs_read(&fs, (uint16_t)key, _tmp_buffer, ZMK_CONFIG_MAX_FIELD_SIZE);

    if(len > 0) {
        // Read OK
        // Push it to memory
        index = _zmk_config_push_local(key, len, _tmp_buffer);
        if(index < 0) {
            return NULL;
        }
        return &fields[index];
    }
    else {
        return NULL;
    }


    return NULL;
}

int zmk_config_write (enum zmk_config_key key, uint16_t size, void *data, uint8_t write_nvs) {
    if(!_config_initialized)    
        return -1;

    int index = 0, len = 0;
    struct zmk_config_field *field = NULL;

    // Search from memory before NVS
    if((index = _zmk_config_get_local(key)) >= 0) {
        field = &fields[index];

        // Update field locally
        field->flags &= ~ZMK_CONFIG_FIELD_FLAG_WRITTEN;
        if(field->size != size) {
            // Reallocate
            k_free(field->data);
            field->data = k_malloc(size);
            if(field->data == NULL) {
                LOG_ERR("Failed to reallocate field");
                return -1;
            }
        }
        memcpy(field->data, data, size);
    }
    else {
        // Push new field
        index = _zmk_config_push_local(key, size, data);
        if(index < 0) {
            LOG_ERR("Failed to push local field");
            return -1;
        }
        field = &fields[index];
        field->flags = ZMK_CONFIG_FIELD_FLAG_READ;
    }

    // field should never be NULL here!
    if(write_nvs) {
        // Write NVS
        len = nvs_write(&fs, (uint16_t)key, data, size);
        if(len < 0) {
            LOG_ERR("Failed to write NVS");
            return -1;
        }
    }
    

    return 0;
}

/**
 * @brief Pushes a field to "fields"
 * @private
 * @param field 
 * @return index of the pushed item, <0 on error
 */
int _zmk_config_push_local (enum zmk_config_key key, uint16_t size, void *data) {

    for(int i = 0; i < ZMK_CONFIG_MAX_FIELDS; i++) {
        if(fields[i].key == ZMK_CONFIG_KEY_INVALID) {
            // Free slot
            fields[i].key = (uint16_t)key;
            fields[i].flags = 0;
            fields[i].size = size;
            fields[i].data = k_malloc(size);
            if(fields[i].data == NULL) {
                // Allocation failed
                memset(&fields[i], 0, sizeof(struct zmk_config_field));
                return -1;
            }
            memcpy(fields[i].data, data, size);
            return i;
        }
    }

    return -1;
}

/**
 * @brief Get field index from local array
 * @private
 * @param key 
 * @return int 
 */
int _zmk_config_get_local (enum zmk_config_key key) {
    for(int i = 0; i < ZMK_CONFIG_MAX_FIELDS; i++) {
        if(fields[i].key == key) {
            return i;
        }
    }
    return -1;
}

#endif
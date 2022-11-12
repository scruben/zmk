/*
    Manages device configurations in NVS
*/
#ifdef CONFIG_ZMK_CONFIG

#include <zmk/config.h>
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

static int zmk_config_init () {
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

static struct zmk_config_field *zmk_config_bind (enum zmk_config_key key, void *data, uint16_t size) {
    if(!_config_initialized)
        return NULL;

    int err = 0, len = 0;
    
    // Check if binding exists, can't bind multiple times
    if(zmk_config_get(key) != NULL) {
        LOG_ERR("Can't bind config value twice!");
        return NULL;
    }

    // Push the field
    for(int i = 0; i < ZMK_CONFIG_MAX_FIELDS; i++) {
        if(fields[i].key == ZMK_CONFIG_KEY_INVALID) {
            fields[i].key = key;
            fields[i].data = data;
            fields[i].size = size;
            fields[i].flags = 0;
            // Init mutex lock
            k_mutex_init(&fields[i].mutex);

            err = zmk_config_read(key);
            if(err < 0) {
                // Returns error if field does not exist in NVS so this can be ignored for now
            }

            return &fields[i];
        }
    }

    LOG_ERR("Config field array full, increase CONFIG_ZMK_CONFIG_MAX_FIELDS");
    return NULL;
}

static struct zmk_config_field *zmk_config_get (enum zmk_config_key key) {
    if(!_config_initialized)
        return NULL;

    for(int i = 0; i < ZMK_CONFIG_MAX_FIELDS; i++) {
        if(fields[i].key == key) {
            return &fields[i];
        }
    }
    return NULL;
}

static int zmk_config_read (enum zmk_config_key key) {
    if(!_config_initialized)
        return -1;

    int len = 0;
    struct zmk_config_field *field = zmk_config_get(key);

    // Field not found
    if(field == NULL)
        return -1;
    
    // Update field from NVS
    len = nvs_read(&fs, (uint16_t)key, _tmp_buffer, ZMK_CONFIG_MAX_FIELD_SIZE);

    if(len > 0) {
        // Read OK

        // Lock

        k_mutex_lock(&field->mutex, K_FOREVER);
        // Check field size
        if(field->size == len) {
            // Field size ok, copy new value
            memcpy(field->data, _tmp_buffer, len);
            field->flags |= ZMK_CONFIG_FIELD_FLAG_READ | ZMK_CONFIG_FIELD_FLAG_WRITTEN;
            k_mutex_unlock(&field->mutex);
        }
        else {
            // Unlock mutex, because write locks it again
            k_mutex_unlock(&field->mutex);
            // TODO: replace old value with new one
            if(zmk_config_write(field->key) < 0) {
                field->flags = 0;
                return -1;
            }
        }
        return 0;
    }
    else {
        field->flags &= ~(ZMK_CONFIG_FIELD_FLAG_READ);
        return -1;
    }


    return -1;
}

static int zmk_config_write (enum zmk_config_key key) {
    if(!_config_initialized)    
        return -1;

    int len = 0;
    struct zmk_config_field *field = NULL;
    field = zmk_config_get(key);

    if(field == NULL)
        return -1;

    k_mutex_lock(&field->mutex, K_FOREVER);
    len = nvs_write(&fs, field->key, field->data, field->size);
    k_mutex_unlock(&field->mutex);
    if(len < 0) {
        LOG_ERR("Config failed to write NVS");
        // Clear written flag since it's not written
        field->flags &= ~(ZMK_CONFIG_FIELD_FLAG_WRITTEN);
        return -1;
    }
    field->flags |= ZMK_CONFIG_FIELD_FLAG_READ | ZMK_CONFIG_FIELD_FLAG_WRITTEN;

    return 0;
}

#endif
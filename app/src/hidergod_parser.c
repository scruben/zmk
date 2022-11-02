#include "hidergod_parser.h"
#include <display.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(hidergodparse, CONFIG_DISPLAY_LOG_LEVEL);

int hidergod_set_value (uint8_t *data, size_t len) {

    if(len < sizeof(struct hidergod_msg_set_value))
        return -3;

    const struct hidergod_msg_set_value *msg = (const struct hidergod_msg_set_value *)data;

    switch(msg->key) {
        case HIDERGOD_VALUE_KEY_TIME:
            if(msg->length != sizeof(int32_t) * 2) {
                LOG_ERR("INVALID LENGTH");
                return -1;
            }
            int32_t *msg_data = (int32_t*)&msg->data;

            // Set clock
            display_set_time(msg_data[0], msg_data[1]);
            break;
        default:
            // Unknown value
            break;
    }


    return 0;
} 

uint8_t *data_buffer = NULL;
uint8_t data_buffer_len = 0;
uint8_t chunk_recv_len = 0;

struct hidergod_msg_header data_header;

int hidergod_parse (uint8_t *buffer, size_t len) {
    int err = 0;
    if(data_buffer_len == 0 && len < sizeof(struct hidergod_msg_header)) {
        // Fail too short message
        return -1;
    }

    if(data_buffer_len != 0) {
        // Continue previous message
        //LOG_ERR("MESSAGE PART: %i\n", len);
        memcpy(data_buffer + data_buffer_len, buffer, len);
    }
    else {
        if(buffer[0] != 0x05) {
            LOG_ERR("ERR msg header!");
            err = -2;
            return err;
        }
        // New message
        //LOG_ERR("MESSAGE: %i %i\n", len, buffer[1]);
        memcpy(&data_header, (struct hidergod_msg_header*)buffer, sizeof(struct hidergod_msg_header));
        data_buffer = malloc(data_header.size);
    }
    memcpy(data_buffer + data_buffer_len, buffer, len);
    data_buffer_len += len;

    if(data_buffer_len >= data_header.chunkSize) {
        data_buffer_len = 0;
        // Data length matches - parse it
        switch((enum hidergod_cmd_t)data_header.cmd) {
            case HIDERGOD_CMD_SET_VALUE:
                err = hidergod_set_value(data_buffer + sizeof(struct hidergod_msg_header), data_header.chunkSize);
                break;
            default:
                // Fail unknown cmd
                err = -2;
                break;
        }

        if(data_buffer) {
            free(data_buffer);
            data_buffer = NULL;
        }
    }
    /*
    else if(data_buffer_len > data_header.chunkSize) {
        // Message too long - reset
        LOG_ERR("Message length does not match one defined in header");
        data_buffer_len = 0;
        err = -3;
    }
    */


    return err;
}
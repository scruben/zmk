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
uint16_t data_buffer_len = 0;
uint8_t chunk_recv_len = 0;

struct hidergod_msg_header data_header;

/**
 * @brief Parses a message
 * 
 * @param buffer 
 * @param len 
 * @return int 
 */
int hidergod_parse (uint8_t *buffer, size_t len) {
    int err = 0;
    if(chunk_recv_len == 0 && len < sizeof(struct hidergod_msg_header)) {
        // Fail on too short message
        return -1;
    }

    if(data_buffer_len == 0) {
        // First chunk, copy header to memory
        memcpy(&data_header, buffer, sizeof(struct hidergod_msg_header));
        if(data_header.reportId != 0x05) {
            // Report id must be 0x05
            return -3;
        }
        // Allocate buffer
        if(data_buffer != NULL) {
            free(data_buffer);
            data_buffer = NULL;
        }
        chunk_recv_len = 0;
        data_buffer = malloc(data_header.size);
    }

    size_t cpy_len = 0;
    if(chunk_recv_len == 0) {
        // First part of the chunk
        memcpy(&data_header, buffer, sizeof(struct hidergod_msg_header));
        cpy_len = data_header.chunkSize <= (len - sizeof(struct hidergod_msg_header)) ? data_header.chunkSize : (len - sizeof(struct hidergod_msg_header));
        memcpy(data_buffer + data_buffer_len, buffer + sizeof(struct hidergod_msg_header), cpy_len);
    }
    else {
        // Full chunk not received, add data
        cpy_len = data_header.chunkSize - chunk_recv_len;
        memcpy(data_buffer + data_buffer_len, buffer, cpy_len);
    }
    data_buffer_len += cpy_len;
    chunk_recv_len += cpy_len;
    if(chunk_recv_len >= HIDERGOD_REPORT_DATA_SIZE) {
        // Chunk ended
        chunk_recv_len = 0;
    }

    if(data_buffer_len >= data_header.size) {
        LOG_ERR("MSG PARSED %i vs %i", data_header.size, data_buffer_len);
        data_buffer_len = 0;
        chunk_recv_len = 0;

        for(int i = 0; i < data_header.size / 8; i++) {
            LOG_ERR("%02X %02X %02X %02X %02X %02X %02X %02X", 
                    data_buffer[i * 8 + 0],
                    data_buffer[i * 8 + 1],
                    data_buffer[i * 8 + 2],
                    data_buffer[i * 8 + 3],
                    data_buffer[i * 8 + 4],
                    data_buffer[i * 8 + 5],
                    data_buffer[i * 8 + 6],
                    data_buffer[i * 8 + 7]
            );
        }
        
        // Message ready
        switch((enum hidergod_cmd_t)data_header.cmd) {
            case HIDERGOD_CMD_SET_VALUE:
                err = hidergod_set_value(data_buffer, data_header.size);
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

    return 0;
}
#include "hidergod_parser.h"
#include <display.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(hidergodparse, CONFIG_DISPLAY_LOG_LEVEL);

int hidergod_set_value (uint8_t *data, size_t len) {
    LOG_ERR("SET VALUE FUNC\n");

    if(len < sizeof(struct hidergod_msg_set_value))
        return -3;

    LOG_ERR("LEN OK\n");


    const struct hidergod_msg_set_value *msg = (const struct hidergod_msg_set_value *)data;

    switch(msg->key) {
        case HIDERGOD_VALUE_KEY_TIME:
            // Set clock
            display_set_time(*(int*)&msg->data);
            LOG_ERR("SET VALUE KEY TIME\n");

            break;
        default:
            // Unknown value
            break;
    }


    return 0;
} 

int hidergod_parse (uint8_t *buffer, size_t len) {
    if(len < sizeof(struct hidergod_msg_header)) {
        // Fail too short message
        return -1;
    }

    LOG_ERR("MESSAGE: %i %i\n", len, buffer[0]);

    int err = 0;

    const struct hidergod_msg_header *header = (const struct hidergod_msg_header*)buffer;

    switch((enum hidergod_cmd_t)header->cmd) {
        case HIDERGOD_CMD_SET_VALUE:
            err = hidergod_set_value(buffer + sizeof(struct hidergod_msg_header), header->chunkSize);
            LOG_ERR("SET VALUE\n");

            break;
        default:
            // Fail unknown cmd
            return -2;
    }

    return err;
}
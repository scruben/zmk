#ifndef __HIDERGODM_PARSER_H
#define __HIDERGODM_PARSER_H
#include <stdint.h>
#include <stdlib.h>

#define HIDERGOD_VALUE_KEY_TIME     0x01

// Package header
struct __attribute__((packed)) hidergod_msg_header {
    // Report ID (always 0)
    //uint8_t reportId; // Not received for some reason?
    // Command
    uint8_t cmd;
    // Total message size
    uint16_t size;
    // Chunk size (actual message size)
    uint8_t chunkSize;
    // Message chunk offset
    uint16_t chunkOffset;
    // CRC16
    uint16_t crc;
};

struct __attribute__((packed)) hidergod_msg_set_value {
    // Key
    uint16_t key;
    // Length
    uint8_t length;
    // Data will be at this address
    uint8_t data;
};


// Command
enum hidergod_cmd_t {
    HIDERGOD_CMD_SET_VALUE = 0x01
};

/**
 * @brief Parses HID message
 * 
 * @param buffer 
 * @param len 
 * @return int 
 */
int hidergod_parse (uint8_t *buffer, size_t len);
#endif
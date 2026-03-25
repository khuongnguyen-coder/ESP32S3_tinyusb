#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VENDOR_PACKET_MAGIC_0 0xA5U
#define VENDOR_PACKET_MAGIC_1 0x5AU
#define VENDOR_PACKET_VERSION 0x01U
#define VENDOR_PACKET_MAX_PAYLOAD 48U
#define VENDOR_PACKET_HEADER_SIZE 6U
#define VENDOR_PACKET_CHECKSUM_SIZE 1U
#define VENDOR_PACKET_MAX_FRAME_SIZE (VENDOR_PACKET_HEADER_SIZE + VENDOR_PACKET_MAX_PAYLOAD + VENDOR_PACKET_CHECKSUM_SIZE)

typedef struct {
    uint8_t version;
    uint8_t type;
    uint16_t payload_len;
    uint8_t payload[VENDOR_PACKET_MAX_PAYLOAD];
} vendor_packet_t;

typedef enum {
    VENDOR_PACKET_PARSE_MAGIC_0 = 0,
    VENDOR_PACKET_PARSE_MAGIC_1,
    VENDOR_PACKET_PARSE_VERSION,
    VENDOR_PACKET_PARSE_TYPE,
    VENDOR_PACKET_PARSE_LEN_0,
    VENDOR_PACKET_PARSE_LEN_1,
    VENDOR_PACKET_PARSE_PAYLOAD,
    VENDOR_PACKET_PARSE_CHECKSUM,
} vendor_packet_parse_state_t;

typedef struct {
    vendor_packet_parse_state_t state;
    vendor_packet_t packet;
    uint16_t payload_offset;
    uint8_t checksum;
} vendor_packet_parser_t;

typedef void (*vendor_packet_handler_t)(const vendor_packet_t *packet, void *user_data);

void vendor_packet_parser_init(vendor_packet_parser_t *parser);
void vendor_packet_parser_reset(vendor_packet_parser_t *parser);
void vendor_packet_parser_push(vendor_packet_parser_t *parser,
                               const uint8_t *data,
                               size_t length,
                               vendor_packet_handler_t handler,
                               void *user_data);

size_t vendor_packet_build(uint8_t type,
                           const uint8_t *payload,
                           uint16_t payload_len,
                           uint8_t *out_frame,
                           size_t out_frame_capacity);

#ifdef __cplusplus
}
#endif
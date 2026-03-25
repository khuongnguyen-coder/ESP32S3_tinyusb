#include "vendor_packet_protocol.h"

#include <string.h>

static uint8_t vendor_packet_checksum_update(uint8_t checksum, uint8_t value)
{
    return (uint8_t)(checksum + value);
}

void vendor_packet_parser_init(vendor_packet_parser_t *parser)
{
    vendor_packet_parser_reset(parser);
}

void vendor_packet_parser_reset(vendor_packet_parser_t *parser)
{
    memset(parser, 0, sizeof(*parser));
    parser->state = VENDOR_PACKET_PARSE_MAGIC_0;
}

static void vendor_packet_parser_finish(vendor_packet_parser_t *parser,
                                        vendor_packet_handler_t handler,
                                        void *user_data)
{
    if (handler != NULL) {
        handler(&parser->packet, user_data);
    }
    vendor_packet_parser_reset(parser);
}

void vendor_packet_parser_push(vendor_packet_parser_t *parser,
                               const uint8_t *data,
                               size_t length,
                               vendor_packet_handler_t handler,
                               void *user_data)
{
    for (size_t index = 0; index < length; ++index) {
        uint8_t byte = data[index];

        switch (parser->state) {
        case VENDOR_PACKET_PARSE_MAGIC_0:
            if (byte == VENDOR_PACKET_MAGIC_0) {
                parser->state = VENDOR_PACKET_PARSE_MAGIC_1;
            }
            break;

        case VENDOR_PACKET_PARSE_MAGIC_1:
            if (byte == VENDOR_PACKET_MAGIC_1) {
                parser->state = VENDOR_PACKET_PARSE_VERSION;
                parser->checksum = 0;
                parser->payload_offset = 0;
                parser->packet.payload_len = 0;
            } else if (byte != VENDOR_PACKET_MAGIC_0) {
                parser->state = VENDOR_PACKET_PARSE_MAGIC_0;
            }
            break;

        case VENDOR_PACKET_PARSE_VERSION:
            if (byte != VENDOR_PACKET_VERSION) {
                vendor_packet_parser_reset(parser);
                break;
            }
            parser->packet.version = byte;
            parser->checksum = vendor_packet_checksum_update(parser->checksum, byte);
            parser->state = VENDOR_PACKET_PARSE_TYPE;
            break;

        case VENDOR_PACKET_PARSE_TYPE:
            parser->packet.type = byte;
            parser->checksum = vendor_packet_checksum_update(parser->checksum, byte);
            parser->state = VENDOR_PACKET_PARSE_LEN_0;
            break;

        case VENDOR_PACKET_PARSE_LEN_0:
            parser->packet.payload_len = byte;
            parser->checksum = vendor_packet_checksum_update(parser->checksum, byte);
            parser->state = VENDOR_PACKET_PARSE_LEN_1;
            break;

        case VENDOR_PACKET_PARSE_LEN_1:
            parser->packet.payload_len |= (uint16_t)byte << 8;
            parser->checksum = vendor_packet_checksum_update(parser->checksum, byte);
            if (parser->packet.payload_len > VENDOR_PACKET_MAX_PAYLOAD) {
                vendor_packet_parser_reset(parser);
            } else if (parser->packet.payload_len == 0) {
                parser->state = VENDOR_PACKET_PARSE_CHECKSUM;
            } else {
                parser->state = VENDOR_PACKET_PARSE_PAYLOAD;
            }
            break;

        case VENDOR_PACKET_PARSE_PAYLOAD:
            parser->packet.payload[parser->payload_offset++] = byte;
            parser->checksum = vendor_packet_checksum_update(parser->checksum, byte);
            if (parser->payload_offset >= parser->packet.payload_len) {
                parser->state = VENDOR_PACKET_PARSE_CHECKSUM;
            }
            break;

        case VENDOR_PACKET_PARSE_CHECKSUM:
            if (byte == parser->checksum) {
                vendor_packet_parser_finish(parser, handler, user_data);
            } else {
                vendor_packet_parser_reset(parser);
            }
            break;
        }
    }
}

size_t vendor_packet_build(uint8_t type,
                           const uint8_t *payload,
                           uint16_t payload_len,
                           uint8_t *out_frame,
                           size_t out_frame_capacity)
{
    size_t frame_len = VENDOR_PACKET_HEADER_SIZE + payload_len + VENDOR_PACKET_CHECKSUM_SIZE;
    uint8_t checksum = 0;

    if (payload_len > VENDOR_PACKET_MAX_PAYLOAD || out_frame_capacity < frame_len) {
        return 0;
    }

    out_frame[0] = VENDOR_PACKET_MAGIC_0;
    out_frame[1] = VENDOR_PACKET_MAGIC_1;
    out_frame[2] = VENDOR_PACKET_VERSION;
    out_frame[3] = type;
    out_frame[4] = (uint8_t)(payload_len & 0xFFU);
    out_frame[5] = (uint8_t)(payload_len >> 8);

    checksum = vendor_packet_checksum_update(checksum, out_frame[2]);
    checksum = vendor_packet_checksum_update(checksum, out_frame[3]);
    checksum = vendor_packet_checksum_update(checksum, out_frame[4]);
    checksum = vendor_packet_checksum_update(checksum, out_frame[5]);

    if (payload_len > 0 && payload != NULL) {
        memcpy(&out_frame[VENDOR_PACKET_HEADER_SIZE], payload, payload_len);
        for (uint16_t index = 0; index < payload_len; ++index) {
            checksum = vendor_packet_checksum_update(checksum, payload[index]);
        }
    }

    out_frame[VENDOR_PACKET_HEADER_SIZE + payload_len] = checksum;
    return frame_len;
}
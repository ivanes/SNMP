#include "frame.h"

void frame_parser_init(frame_parser_t *parser)
{
    parser->state = FRAME_SYNC_A;
    parser->length = 0;
    parser->index = 0;
    parser->cksum = 0;
}

int frame_parser_feed(frame_parser_t *parser, uint8_t byte)
{
    switch (parser->state) {
    case FRAME_SYNC_A:
        if (byte == FRAME_SYNC0) {
            parser->state = FRAME_SYNC_B;
        }
        break;
    case FRAME_SYNC_B:
        parser->state = (byte == FRAME_SYNC1) ? FRAME_LEN_HI : FRAME_SYNC_A;
        if (byte == FRAME_SYNC0) {
            parser->state = FRAME_SYNC_B;
        }
        break;
    case FRAME_LEN_HI:
        parser->length = (uint16_t)byte << 8;
        parser->cksum = byte;
        parser->state = FRAME_LEN_LO;
        break;
    case FRAME_LEN_LO:
        parser->length = (uint16_t)(parser->length | byte);
        parser->cksum ^= byte;
        parser->index = 0;
        if (parser->length == 0 || parser->length > FRAME_MAX_PAYLOAD) {
            parser->state = FRAME_SYNC_A;
            break;
        }
        parser->state = FRAME_PAYLOAD;
        break;
    case FRAME_PAYLOAD:
        parser->payload[parser->index++] = byte;
        parser->cksum ^= byte;
        if (parser->index >= parser->length) {
            parser->state = FRAME_CKSUM;
        }
        break;
    case FRAME_CKSUM:
        parser->state = FRAME_SYNC_A;
        return (byte == parser->cksum) ? 1 : 0;
    default:
        parser->state = FRAME_SYNC_A;
        break;
    }
    return 0;
}

int frame_encode(const uint8_t *payload, size_t payload_len, uint8_t *out, size_t out_max, size_t *out_len)
{
    size_t i;
    uint8_t cksum;
    uint8_t len_hi;
    uint8_t len_lo;

    if (payload == NULL || out == NULL || out_len == NULL) {
        return -1;
    }
    if (payload_len == 0 || payload_len > FRAME_MAX_PAYLOAD) {
        return -1;
    }
    if (out_max < payload_len + FRAME_OVERHEAD) {
        return -1;
    }

    len_hi = (uint8_t)((payload_len >> 8) & 0xFFu);
    len_lo = (uint8_t)(payload_len & 0xFFu);
    cksum = len_hi ^ len_lo;
    for (i = 0; i < payload_len; i++) {
        cksum ^= payload[i];
    }

    out[0] = FRAME_SYNC0;
    out[1] = FRAME_SYNC1;
    out[2] = len_hi;
    out[3] = len_lo;
    for (i = 0; i < payload_len; i++) {
        out[4 + i] = payload[i];
    }
    out[4 + payload_len] = cksum;
    *out_len = payload_len + FRAME_OVERHEAD;
    return 0;
}

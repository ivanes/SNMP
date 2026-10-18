#ifndef SNMP_FRAME_H
#define SNMP_FRAME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FRAME_SYNC0      0xA5u
#define FRAME_SYNC1      0x5Au
#define FRAME_MAX_PAYLOAD 1024u
#define FRAME_OVERHEAD   5u

typedef enum {
    FRAME_SYNC_A = 0,
    FRAME_SYNC_B,
    FRAME_LEN_HI,
    FRAME_LEN_LO,
    FRAME_PAYLOAD,
    FRAME_CKSUM
} frame_state_t;

typedef struct {
    frame_state_t state;
    uint16_t length;
    uint16_t index;
    uint8_t cksum;
    uint8_t payload[FRAME_MAX_PAYLOAD];
} frame_parser_t;

void frame_parser_init(frame_parser_t *parser);

/* Returns 1 when a complete valid frame is in parser->payload. */
int frame_parser_feed(frame_parser_t *parser, uint8_t byte);

int frame_encode(const uint8_t *payload,
                 size_t payload_len,
                 uint8_t *out,
                 size_t out_max,
                 size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif

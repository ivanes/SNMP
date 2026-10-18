#ifndef SNMP_FRAME_H /* не включать кадрирование дважды */
#define SNMP_FRAME_H /* отметить, что заголовок кадра открыт */

#include <stddef.h> /* size_t */
#include <stdint.h> /* uint8_t, uint16_t */

#ifdef __cplusplus /* если это C++ */
extern "C" { /* оставить C-имена */
#endif /* конец проверки языка */

#define FRAME_SYNC0      0xA5u /* первый байт синхронизации */
#define FRAME_SYNC1      0x5Au /* второй байт синхронизации */
#define FRAME_MAX_PAYLOAD 1024u /* максимальная длина полезной нагрузки */
#define FRAME_OVERHEAD   5u /* sync + длина + контрольная сумма */

typedef enum { /* состояния разбора кадра */
    FRAME_SYNC_A = 0, /* ждём 0xA5 */
    FRAME_SYNC_B, /* ждём 0x5A */
    FRAME_LEN_HI, /* читаем старший байт длины */
    FRAME_LEN_LO, /* читаем младший байт длины */
    FRAME_PAYLOAD, /* копируем полезные байты */
    FRAME_CKSUM /* проверяем XOR-сумму */
} frame_state_t; /* конец состояний */

typedef struct { /* побайтный разборщик кадра */
    frame_state_t state; /* текущее состояние автомата */
    uint16_t length; /* ожидаемая длина нагрузки */
    uint16_t index; /* сколько байт нагрузки уже принято */
    uint8_t cksum; /* накопленная XOR-сумма */
    uint8_t payload[FRAME_MAX_PAYLOAD]; /* собранная нагрузка */
} frame_parser_t; /* конец разборщика */

void frame_parser_init(frame_parser_t *parser); /* сбросить автомат в поиск синхробайта */

int frame_parser_feed(frame_parser_t *parser, uint8_t byte); /* скормить байт; 1 = кадр готов */

int frame_encode(const uint8_t *payload, /* упаковать нагрузку в кадр A5 5A */
                 size_t payload_len, /* длина нагрузки */
                 uint8_t *out, /* выходной буфер */
                 size_t out_max, /* размер выходного буфера */
                 size_t *out_len); /* итоговая длина кадра */

#ifdef __cplusplus /* если это C++ */
} /* закрыть extern "C" */
#endif /* конец проверки языка */

#endif /* конец охранного include */

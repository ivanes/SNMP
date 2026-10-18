#include "frame.h" /* автомат и кодировщик кадра */

void frame_parser_init(frame_parser_t *parser) /* сбросить разборщик */
{ /* начало frame_parser_init */
    parser->state = FRAME_SYNC_A; /* снова искать 0xA5 */
    parser->length = 0; /* длины ещё нет */
    parser->index = 0; /* нагрузка пустая */
    parser->cksum = 0; /* сумма обнулена */
} /* конец frame_parser_init */

int frame_parser_feed(frame_parser_t *parser, uint8_t byte) /* принять один байт с линии */
{ /* начало frame_parser_feed */
    switch (parser->state) { /* шаг автомата */
    case FRAME_SYNC_A: /* ждём первый синхробайт */
        if (byte == FRAME_SYNC0) { /* пришёл 0xA5 */
            parser->state = FRAME_SYNC_B; /* теперь ждём 0x5A */
        } /* иначе остаёмся здесь */
        break; /* конец SYNC_A */
    case FRAME_SYNC_B: /* ждём второй синхробайт */
        parser->state = (byte == FRAME_SYNC1) ? FRAME_LEN_HI : FRAME_SYNC_A; /* 0x5A — дальше длина */
        if (byte == FRAME_SYNC0) { /* снова 0xA5 — это может быть новое начало */
            parser->state = FRAME_SYNC_B; /* остаться на втором синхробайте */
        } /* конец перекрывающегося A5 */
        break; /* конец SYNC_B */
    case FRAME_LEN_HI: /* старший байт длины */
        parser->length = (uint16_t)byte << 8; /* записать старшие 8 бит */
        parser->cksum = byte; /* начать XOR с этого байта */
        parser->state = FRAME_LEN_LO; /* дальше младший байт */
        break; /* конец LEN_HI */
    case FRAME_LEN_LO: /* младший байт длины */
        parser->length = (uint16_t)(parser->length | byte); /* собрать 16-битную длину */
        parser->cksum ^= byte; /* учесть в сумме */
        parser->index = 0; /* нагрузка с нуля */
        if (parser->length == 0 || parser->length > FRAME_MAX_PAYLOAD) { /* пустой или слишком длинный кадр */
            parser->state = FRAME_SYNC_A; /* отбросить и искать заново */
            break; /* выйти из ветки */
        } /* конец проверки длины */
        parser->state = FRAME_PAYLOAD; /* читать полезные байты */
        break; /* конец LEN_LO */
    case FRAME_PAYLOAD: /* копируем нагрузку */
        parser->payload[parser->index++] = byte; /* сохранить байт */
        parser->cksum ^= byte; /* учесть в сумме */
        if (parser->index >= parser->length) { /* все байты нагрузки пришли */
            parser->state = FRAME_CKSUM; /* осталась контрольная сумма */
        } /* конец проверки полноты */
        break; /* конец PAYLOAD */
    case FRAME_CKSUM: /* последний байт — XOR */
        parser->state = FRAME_SYNC_A; /* после проверки снова искать кадр */
        return (byte == parser->cksum) ? 1 : 0; /* 1 только если сумма совпала */
    default: /* неизвестное состояние */
        parser->state = FRAME_SYNC_A; /* безопасный сброс */
        break; /* конец default */
    } /* конец switch */
    return 0; /* кадр ещё не готов */
} /* конец frame_parser_feed */

int frame_encode(const uint8_t *payload, size_t payload_len, uint8_t *out, size_t out_max, size_t *out_len) /* собрать кадр */
{ /* начало frame_encode */
    size_t i; /* индекс */
    uint8_t cksum; /* XOR-сумма */
    uint8_t len_hi; /* старший байт длины */
    uint8_t len_lo; /* младший байт длины */

    if (payload == NULL || out == NULL || out_len == NULL) { /* обязательные указатели */
        return -1; /* ошибка аргументов */
    } /* конец проверки указателей */
    if (payload_len == 0 || payload_len > FRAME_MAX_PAYLOAD) { /* пусто или слишком много */
        return -1; /* ошибка длины */
    } /* конец проверки длины */
    if (out_max < payload_len + FRAME_OVERHEAD) { /* выходной буфер мал */
        return -1; /* не влезает */
    } /* конец проверки буфера */

    len_hi = (uint8_t)((payload_len >> 8) & 0xFFu); /* старшие 8 бит длины */
    len_lo = (uint8_t)(payload_len & 0xFFu); /* младшие 8 бит длины */
    cksum = len_hi ^ len_lo; /* сумма начинается с байтов длины */
    for (i = 0; i < payload_len; i++) { /* пройти нагрузку */
        cksum ^= payload[i]; /* накопить XOR */
    } /* конец расчёта суммы */

    out[0] = FRAME_SYNC0; /* 0xA5 */
    out[1] = FRAME_SYNC1; /* 0x5A */
    out[2] = len_hi; /* длина, старший байт */
    out[3] = len_lo; /* длина, младший байт */
    for (i = 0; i < payload_len; i++) { /* скопировать нагрузку */
        out[4 + i] = payload[i]; /* сразу после заголовка */
    } /* конец копирования */
    out[4 + payload_len] = cksum; /* контрольная сумма в конце */
    *out_len = payload_len + FRAME_OVERHEAD; /* полная длина кадра */
    return 0; /* успех */
} /* конец frame_encode */

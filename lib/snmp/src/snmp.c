#include "snmp.h" /* публичный API агента */

#include <string.h> /* memcpy, memset, strlen, strcmp */

#define BER_TAG_SEQUENCE 0x30u /* универсальный тег SEQUENCE */
#define SYS_OBJECT_ARCS_N 9 /* число дуг sysObjectID */

static const uint32_t k_sys_object_id[SYS_OBJECT_ARCS_N] = { /* OID устройства 1.3.6.1.4.1.99999.1.1 */
    1, 3, 6, 1, 4, 1, 99999, 1, 1 /* дуги private enterprise */
}; /* конец объявления */

typedef struct { /* курсор BER */
    const uint8_t *cur; /* текущая позиция чтения */
    const uint8_t *end; /* конец буфера чтения */
} ber_in_t; /* курсор чтения BER */

typedef struct { /* курсор BER */
    uint8_t *cur; /* текущая позиция записи */
    uint8_t *end; /* конец буфера записи */
} ber_out_t; /* курсор записи BER */

typedef enum { /* индексы объектов MIB */
    MIB_SYSDESCR = 0, /* MIB_SYSDESCR в таблице MIB */
    MIB_SYSOBJECTID, /* MIB_SYSOBJECTID в таблице MIB */
    MIB_SYSUPTIME, /* MIB_SYSUPTIME в таблице MIB */
    MIB_SYSCONTACT, /* MIB_SYSCONTACT в таблице MIB */
    MIB_SYSNAME, /* MIB_SYSNAME в таблице MIB */
    MIB_SYSLOCATION, /* MIB_SYSLOCATION в таблице MIB */
    MIB_SYSSERVICES, /* MIB_SYSSERVICES в таблице MIB */
    MIB_LEDSTATE, /* MIB_LEDSTATE в таблице MIB */
    MIB_COUNT /* MIB_COUNT в таблице MIB */
} mib_id_t; /* конец индексов MIB */

static const uint32_t k_mib_oids[MIB_COUNT][SNMP_OID_MAX_ARCS] = { /* OID всех листьев агента */
    {1, 3, 6, 1, 2, 1, 1, 1, 0}, /* sysDescr */
    {1, 3, 6, 1, 2, 1, 1, 2, 0}, /* sysObjectID */
    {1, 3, 6, 1, 2, 1, 1, 3, 0}, /* sysUpTime */
    {1, 3, 6, 1, 2, 1, 1, 4, 0}, /* sysContact */
    {1, 3, 6, 1, 2, 1, 1, 5, 0}, /* sysName */
    {1, 3, 6, 1, 2, 1, 1, 6, 0}, /* sysLocation */
    {1, 3, 6, 1, 2, 1, 1, 7, 0}, /* sysServices */
    {1, 3, 6, 1, 4, 1, 99999, 1, 1, 0}, /* лист ledState */
}; /* конец объявления */

static const uint8_t k_mib_oid_len[MIB_COUNT] = { /* длины OID листьев */
    9, 9, 9, 9, 9, 9, 9, 10 /* system.* — по 9 дуг, ledState — 10 */
}; /* конец объявления */

static int oid_cmp(const uint32_t *a, size_t a_len, const uint32_t *b, size_t b_len) /* сравнить два OID лексикографически */
{ /* начало блока */
    size_t n = (a_len < b_len) ? a_len : b_len; /* задать size_t n */
    size_t i; /* шаг oid_cmp */

    for (i = 0; i < n; i++) { /* цикл (i = 0; i < n; i++) */
        if (a[i] < b[i]) { /* если (a[i] < b[i]) */
            return -1; /* вернуть -1 */
        } /* конец блока */
        if (a[i] > b[i]) { /* если (a[i] > b[i]) */
            return 1; /* вернуть 1 */
        } /* конец блока */
    } /* конец блока */
    if (a_len < b_len) { /* если (a_len < b_len) */
        return -1; /* вернуть -1 */
    } /* конец блока */
    if (a_len > b_len) { /* если (a_len > b_len) */
        return 1; /* вернуть 1 */
    } /* конец блока */
    return 0; /* вернуть 0 */
} /* конец блока */

int snmp_oid_from_text(const char *text, uint32_t *arcs, size_t *n_arcs) /* разобрать текстовый OID в массив дуг */
{ /* начало блока */
    size_t count = 0; /* задать size_t count */
    const char *p = text; /* задать const char *p */

    if (text == NULL || arcs == NULL || n_arcs == NULL) { /* если (text == NULL || arcs == NULL || n_arcs == NULL) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */

    while (*p != '\0') { /* пока (*p != '\0') */
        unsigned long value = 0; /* задать unsigned long value */
        int digits = 0; /* функция digits */

        if (*p == '.') { /* если (*p == '.') */
            p++; /* шаг snmp_oid_from_text */
        } /* конец блока */
        if (*p < '0' || *p > '9') { /* если (*p < '0' || *p > '9') */
            return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
        } /* конец блока */
        while (*p >= '0' && *p <= '9') { /* пока (*p >= '0' && *p <= '9') */
            value = (value * 10u) + (unsigned long)(*p - '0'); /* задать value */
            p++; /* шаг snmp_oid_from_text */
            digits++; /* шаг snmp_oid_from_text */
            if (digits > 10) { /* если (digits > 10) */
                return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
            } /* конец блока */
        } /* конец блока */
        if (count >= SNMP_OID_MAX_ARCS) { /* если (count >= SNMP_OID_MAX_ARCS) */
            return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
        } /* конец блока */
        arcs[count++] = (uint32_t)value; /* задать arcs[count++] */
        if (*p != '\0' && *p != '.') { /* если (*p != '\0' && *p != '.') */
            return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
        } /* конец блока */
    } /* конец блока */

    if (count < 2) { /* если (count < 2) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    *n_arcs = count; /* задать n_arcs */
    return SNMP_OK; /* вернуть SNMP_OK */
} /* конец блока */

static int ber_remain(const ber_in_t *in) /* сколько байт ещё можно прочитать */
{ /* начало блока */
    return (int)(in->end - in->cur); /* вернуть (int)(in->end - in->cur) */
} /* конец блока */

static int ber_read_byte(ber_in_t *in, uint8_t *out) /* прочитать один байт BER */
{ /* начало блока */
    if (ber_remain(in) < 1) { /* если (ber_remain(in) < 1) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    *out = *in->cur++; /* задать out */
    return SNMP_OK; /* вернуть SNMP_OK */
} /* конец блока */

static int ber_read_length(ber_in_t *in, size_t *len) /* прочитать поле длины BER */
{ /* начало блока */
    uint8_t first; /* шаг ber_read_length */

    if (ber_read_byte(in, &first) != SNMP_OK) { /* если (ber_read_byte(in, &first) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if ((first & 0x80u) == 0) { /* если ((first & 0x80u) == 0) */
        *len = first; /* задать len */
        return SNMP_OK; /* вернуть SNMP_OK */
    } /* конец блока */

    { /* начало блока */
        unsigned nbytes = first & 0x7Fu; /* задать unsigned nbytes */
        size_t value = 0; /* задать size_t value */
        unsigned i; /* шаг ber_read_length */

        if (nbytes == 0 || nbytes > 2) { /* если (nbytes == 0 || nbytes > 2) */
            return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
        } /* конец блока */
        for (i = 0; i < nbytes; i++) { /* цикл (i = 0; i < nbytes; i++) */
            uint8_t b; /* шаг ber_read_length */
            if (ber_read_byte(in, &b) != SNMP_OK) { /* если (ber_read_byte(in, &b) != SNMP_OK) */
                return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
            } /* конец блока */
            value = (value << 8) | b; /* задать value */
        } /* конец блока */
        *len = value; /* задать len */
        return SNMP_OK; /* вернуть SNMP_OK */
    } /* конец блока */
} /* конец блока */

static int ber_read_header(ber_in_t *in, uint8_t *tag, ber_in_t *body) /* прочитать тег и выделить тело */
{ /* начало блока */
    size_t len; /* шаг ber_read_header */

    if (ber_read_byte(in, tag) != SNMP_OK) { /* если (ber_read_byte(in, tag) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_length(in, &len) != SNMP_OK) { /* если (ber_read_length(in, &len) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if ((size_t)ber_remain(in) < len) { /* если ((size_t)ber_remain(in) < len) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    body->cur = in->cur; /* задать body->cur */
    body->end = in->cur + len; /* задать body->end */
    in->cur += len; /* задать in->cur + */
    return SNMP_OK; /* вернуть SNMP_OK */
} /* конец блока */

static int ber_expect(ber_in_t *in, uint8_t expected, ber_in_t *body) /* прочитать TLV и проверить тег */
{ /* начало блока */
    uint8_t tag; /* шаг ber_expect */
    if (ber_read_header(in, &tag, body) != SNMP_OK || tag != expected) { /* если (ber_read_header(in, &tag, body) != SNMP_OK || tag != expected) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    return SNMP_OK; /* вернуть SNMP_OK */
} /* конец блока */

static int ber_read_int(ber_in_t *in, int32_t *value) /* прочитать INTEGER */
{ /* начало блока */
    ber_in_t body; /* шаг ber_read_int */
    size_t n; /* шаг ber_read_int */
    uint32_t acc = 0; /* задать uint32_t acc */
    uint8_t first; /* шаг ber_read_int */

    if (ber_expect(in, SNMP_TAG_INTEGER, &body) != SNMP_OK) { /* если (ber_expect(in, SNMP_TAG_INTEGER, &body) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    n = (size_t)ber_remain(&body); /* задать n */
    if (n == 0 || n > 4) { /* если (n == 0 || n > 4) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    first = body.cur[0]; /* задать first */
    if ((first & 0x80u) != 0) { /* если ((first & 0x80u) != 0) */
        acc = 0xFFFFFFFFu; /* задать acc */
    } /* конец блока */
    while (ber_remain(&body) > 0) { /* пока (ber_remain(&body) > 0) */
        uint8_t b; /* шаг ber_read_int */
        if (ber_read_byte(&body, &b) != SNMP_OK) { /* если (ber_read_byte(&body, &b) != SNMP_OK) */
            return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
        } /* конец блока */
        acc = (acc << 8) | b; /* задать acc */
    } /* конец блока */
    *value = (int32_t)acc; /* задать value */
    return SNMP_OK; /* вернуть SNMP_OK */
} /* конец блока */

static int ber_read_octets(ber_in_t *in, char *out, size_t out_max, size_t *out_len) /* прочитать OCTET STRING */
{ /* начало блока */
    ber_in_t body; /* шаг ber_read_octets */
    size_t n; /* шаг ber_read_octets */

    if (ber_expect(in, SNMP_TAG_OCTET_STR, &body) != SNMP_OK) { /* если (ber_expect(in, SNMP_TAG_OCTET_STR, &body) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    n = (size_t)ber_remain(&body); /* задать n */
    if (n >= out_max) { /* если (n >= out_max) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    memcpy(out, body.cur, n); /* скопировать байты */
    out[n] = '\0'; /* задать out[n] */
    if (out_len != NULL) { /* если (out_len != NULL) */
        *out_len = n; /* задать out_len */
    } /* конец блока */
    return SNMP_OK; /* вернуть SNMP_OK */
} /* конец блока */

static int ber_read_oid(ber_in_t *in, uint32_t *arcs, size_t *n_arcs) /* прочитать OBJECT IDENTIFIER */
{ /* начало блока */
    ber_in_t body; /* шаг ber_read_oid */
    size_t count = 0; /* задать size_t count */
    uint8_t first; /* шаг ber_read_oid */

    if (ber_expect(in, SNMP_TAG_OID, &body) != SNMP_OK) { /* если (ber_expect(in, SNMP_TAG_OID, &body) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_remain(&body) < 1) { /* если (ber_remain(&body) < 1) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_byte(&body, &first) != SNMP_OK) { /* если (ber_read_byte(&body, &first) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    arcs[count++] = first / 40u; /* задать arcs[count++] */
    arcs[count++] = first % 40u; /* задать arcs[count++] */

    while (ber_remain(&body) > 0) { /* пока (ber_remain(&body) > 0) */
        uint32_t arc = 0; /* задать uint32_t arc */
        int guard = 0; /* функция guard */
        uint8_t b; /* шаг ber_read_oid */
        do { /* повторять */
            if (ber_read_byte(&body, &b) != SNMP_OK) { /* если (ber_read_byte(&body, &b) != SNMP_OK) */
                return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
            } /* конец блока */
            arc = (arc << 7) | (uint32_t)(b & 0x7Fu); /* задать arc */
            if (++guard > 5) { /* если (++guard > 5) */
                return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
            } /* конец блока */
        } while ((b & 0x80u) != 0); /* шаг ber_read_oid */
        if (count >= SNMP_OID_MAX_ARCS) { /* если (count >= SNMP_OID_MAX_ARCS) */
            return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
        } /* конец блока */
        arcs[count++] = arc; /* задать arcs[count++] */
    } /* конец блока */
    *n_arcs = count; /* задать n_arcs */
    return SNMP_OK; /* вернуть SNMP_OK */
} /* конец блока */

static int ber_write_bytes(ber_out_t *out, const uint8_t *data, size_t n) /* записать сырые байты */
{ /* начало блока */
    if ((size_t)(out->end - out->cur) < n) { /* если ((size_t)(out->end - out->cur) < n) */
        return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
    } /* конец блока */
    memcpy(out->cur, data, n); /* скопировать байты */
    out->cur += n; /* задать out->cur + */
    return SNMP_OK; /* вернуть SNMP_OK */
} /* конец блока */

static int ber_write_byte(ber_out_t *out, uint8_t b) /* записать один байт */
{ /* начало блока */
    return ber_write_bytes(out, &b, 1); /* вернуть ber_write_bytes(out, &b, 1) */
} /* конец блока */

static int ber_write_length(ber_out_t *out, size_t len) /* записать длину BER */
{ /* начало блока */
    if (len < 0x80u) { /* если (len < 0x80u) */
        return ber_write_byte(out, (uint8_t)len); /* вернуть ber_write_byte(out, (uint8_t)len) */
    } /* конец блока */
    if (len <= 0xFFu) { /* если (len <= 0xFFu) */
        if (ber_write_byte(out, 0x81u) != SNMP_OK) { /* если (ber_write_byte(out, 0x81u) != SNMP_OK) */
            return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
        } /* конец блока */
        return ber_write_byte(out, (uint8_t)len); /* вернуть ber_write_byte(out, (uint8_t)len) */
    } /* конец блока */
    if (len > 0xFFFFu) { /* если (len > 0xFFFFu) */
        return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
    } /* конец блока */
    if (ber_write_byte(out, 0x82u) != SNMP_OK) { /* если (ber_write_byte(out, 0x82u) != SNMP_OK) */
        return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
    } /* конец блока */
    if (ber_write_byte(out, (uint8_t)((len >> 8) & 0xFFu)) != SNMP_OK) { /* если (ber_write_byte(out, (uint8_t)((len >> 8) & 0xFFu)) != SNMP_OK) */
        return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
    } /* конец блока */
    return ber_write_byte(out, (uint8_t)(len & 0xFFu)); /* вернуть ber_write_byte(out, (uint8_t)(len & 0xFFu)) */
} /* конец блока */

static int ber_write_header(ber_out_t *out, uint8_t tag, const uint8_t *body, size_t body_len) /* записать тег, длину и тело */
{ /* начало блока */
    if (ber_write_byte(out, tag) != SNMP_OK) { /* если (ber_write_byte(out, tag) != SNMP_OK) */
        return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
    } /* конец блока */
    if (ber_write_length(out, body_len) != SNMP_OK) { /* если (ber_write_length(out, body_len) != SNMP_OK) */
        return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
    } /* конец блока */
    return ber_write_bytes(out, body, body_len); /* вернуть ber_write_bytes(out, body, body_len) */
} /* конец блока */

static int ber_write_int_tag(ber_out_t *out, uint8_t tag, int32_t value) /* записать целое с заданным тегом */
{ /* начало блока */
    uint8_t buf[4]; /* шаг ber_write_int_tag */
    uint32_t u = (uint32_t)value; /* задать uint32_t u */
    int start = 0; /* функция start */

    buf[0] = (uint8_t)((u >> 24) & 0xFFu); /* задать buf[0] */
    buf[1] = (uint8_t)((u >> 16) & 0xFFu); /* задать buf[1] */
    buf[2] = (uint8_t)((u >> 8) & 0xFFu); /* задать buf[2] */
    buf[3] = (uint8_t)(u & 0xFFu); /* задать buf[3] */

    while (start < 3) { /* пока (start < 3) */
        int sign_ext = ((buf[start] == 0x00) && ((buf[start + 1] & 0x80u) == 0)); /* функция sign_ext */
        int neg_ext = ((buf[start] == 0xFF) && ((buf[start + 1] & 0x80u) != 0)); /* функция neg_ext */
        if (!sign_ext && !neg_ext) { /* если (!sign_ext && !neg_ext) */
            break; /* выйти из ветки */
        } /* конец блока */
        start++; /* шаг ber_write_int_tag */
    } /* конец блока */
    return ber_write_header(out, tag, buf + start, (size_t)(4 - start)); /* вернуть ber_write_header(out, tag, buf + start, (size_t)(4 - start)) */
} /* конец блока */

static int ber_write_int(ber_out_t *out, int32_t value) /* записать INTEGER */
{ /* начало блока */
    return ber_write_int_tag(out, SNMP_TAG_INTEGER, value); /* вернуть ber_write_int_tag(out, SNMP_TAG_INTEGER, value) */
} /* конец блока */

static int ber_write_octets(ber_out_t *out, const char *s, size_t n) /* записать OCTET STRING */
{ /* начало блока */
    return ber_write_header(out, SNMP_TAG_OCTET_STR, (const uint8_t *)s, n); /* вернуть ber_write_header(out, SNMP_TAG_OCTET_STR, (const uint8_t *)s, n) */
} /* конец блока */

static int ber_write_null(ber_out_t *out) /* записать NULL */
{ /* начало блока */
    return ber_write_header(out, SNMP_TAG_NULL, NULL, 0); /* вернуть ber_write_header(out, SNMP_TAG_NULL, NULL, 0) */
} /* конец блока */

static int ber_write_exception(ber_out_t *out, uint8_t tag) /* записать исключение v2c */
{ /* начало блока */
    return ber_write_header(out, tag, NULL, 0); /* вернуть ber_write_header(out, tag, NULL, 0) */
} /* конец блока */

static int ber_write_oid(ber_out_t *out, const uint32_t *arcs, size_t n_arcs) /* записать OBJECT IDENTIFIER */
{ /* начало блока */
    uint8_t buf[SNMP_OID_MAX_ARCS * 5]; /* шаг ber_write_oid */
    size_t n = 0; /* задать size_t n */
    size_t i; /* шаг ber_write_oid */

    if (n_arcs < 2 || arcs[0] > 2 || arcs[1] > 39) { /* если (n_arcs < 2 || arcs[0] > 2 || arcs[1] > 39) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    buf[n++] = (uint8_t)(40u * arcs[0] + arcs[1]); /* задать buf[n++] */
    for (i = 2; i < n_arcs; i++) { /* цикл (i = 2; i < n_arcs; i++) */
        uint32_t arc = arcs[i]; /* задать uint32_t arc */
        uint8_t tmp[5]; /* шаг ber_write_oid */
        int t = 0; /* функция t */
        tmp[t++] = (uint8_t)(arc & 0x7Fu); /* задать tmp[t++] */
        arc >>= 7; /* задать arc >> */
        while (arc != 0) { /* пока (arc != 0) */
            tmp[t++] = (uint8_t)((arc & 0x7Fu) | 0x80u); /* задать tmp[t++] */
            arc >>= 7; /* задать arc >> */
        } /* конец блока */
        while (t > 0) { /* пока (t > 0) */
            buf[n++] = tmp[--t]; /* задать buf[n++] */
        } /* конец блока */
    } /* конец блока */
    return ber_write_header(out, SNMP_TAG_OID, buf, n); /* вернуть ber_write_header(out, SNMP_TAG_OID, buf, n) */
} /* конец блока */

static int find_exact(const uint32_t *oid, size_t oid_len) /* найти точный OID в MIB */
{ /* начало блока */
    int i; /* функция i; */
    for (i = 0; i < MIB_COUNT; i++) { /* цикл (i = 0; i < MIB_COUNT; i++) */
        if (oid_cmp(oid, oid_len, k_mib_oids[i], k_mib_oid_len[i]) == 0) { /* если (oid_cmp(oid, oid_len, k_mib_oids[i], k_mib_oid_len[i]) == 0) */
            return i; /* вернуть i */
        } /* конец блока */
    } /* конец блока */
    return -1; /* вернуть -1 */
} /* конец блока */

static int find_next(const uint32_t *oid, size_t oid_len) /* найти следующий OID для GETNEXT */
{ /* начало блока */
    int i; /* функция i; */
    int best = -1; /* функция best */

    for (i = 0; i < MIB_COUNT; i++) { /* цикл (i = 0; i < MIB_COUNT; i++) */
        if (oid_cmp(k_mib_oids[i], k_mib_oid_len[i], oid, oid_len) > 0) { /* если (oid_cmp(k_mib_oids[i], k_mib_oid_len[i], oid, oid_len) > 0) */
            if (best < 0 || oid_cmp(k_mib_oids[i], k_mib_oid_len[i], /* если (best < 0 || oid_cmp(k_mib_oids[i], k_mib_oid_len[i], */
                                    k_mib_oids[best], k_mib_oid_len[best]) < 0) { /* шаг find_next */
                best = i; /* задать best */
            } /* конец блока */
        } /* конец блока */
    } /* конец блока */
    return best; /* вернуть best */
} /* конец блока */

static const char *cfg_str(const char *value, const char *fallback) /* функция char */
{ /* начало блока */
    return (value != NULL) ? value : fallback; /* вернуть (value != NULL) ? value : fallback */
} /* конец блока */

static int write_mib_value(ber_out_t *out, const snmp_agent_t *agent, int id) /* закодировать значение листа MIB */
{ /* начало блока */
    const snmp_agent_config_t *cfg = agent->cfg; /* задать const snmp_agent_config_t *cfg */
    const char *s; /* шаг write_mib_value */
    uint32_t uptime; /* шаг write_mib_value */

    switch (id) { /* разобрать объект MIB */
    case MIB_SYSDESCR: /* ветка MIB_SYSDESCR */
        s = cfg_str(cfg->sys_descr, "STM32F411 FreeRTOS SNMP"); /* задать s */
        return ber_write_octets(out, s, strlen(s)); /* вернуть ber_write_octets(out, s, strlen(s)) */
    case MIB_SYSOBJECTID: /* ветка MIB_SYSOBJECTID */
        return ber_write_oid(out, k_sys_object_id, SYS_OBJECT_ARCS_N); /* вернуть ber_write_oid(out, k_sys_object_id, SYS_OBJECT_ARCS_N) */
    case MIB_SYSUPTIME: /* ветка MIB_SYSUPTIME */
        uptime = (cfg->get_uptime_cs != NULL) ? cfg->get_uptime_cs() : 0u; /* задать uptime */
        return ber_write_int_tag(out, SNMP_TAG_TIMETICKS, (int32_t)uptime); /* вернуть ber_write_int_tag(out, SNMP_TAG_TIMETICKS, (int32_t)uptime) */
    case MIB_SYSCONTACT: /* ветка MIB_SYSCONTACT */
        s = cfg_str(cfg->sys_contact, ""); /* задать s */
        return ber_write_octets(out, s, strlen(s)); /* вернуть ber_write_octets(out, s, strlen(s)) */
    case MIB_SYSNAME: /* ветка MIB_SYSNAME */
        s = cfg_str(cfg->sys_name, "snmp-f411"); /* задать s */
        return ber_write_octets(out, s, strlen(s)); /* вернуть ber_write_octets(out, s, strlen(s)) */
    case MIB_SYSLOCATION: /* ветка MIB_SYSLOCATION */
        s = cfg_str(cfg->sys_location, ""); /* задать s */
        return ber_write_octets(out, s, strlen(s)); /* вернуть ber_write_octets(out, s, strlen(s)) */
    case MIB_SYSSERVICES: /* ветка MIB_SYSSERVICES */
        return ber_write_int(out, cfg->sys_services); /* вернуть ber_write_int(out, cfg->sys_services) */
    case MIB_LEDSTATE: /* ветка MIB_LEDSTATE */
        return ber_write_int(out, (cfg->get_led != NULL) ? cfg->get_led() : 0); /* вернуть ber_write_int(out, (cfg->get_led != NULL) ? cfg->get_led() : 0) */
    default: /* неизвестный объект */
        return SNMP_ERR_INTERNAL; /* вернуть SNMP_ERR_INTERNAL */
    } /* конец блока */
} /* конец блока */

static int community_ok(const snmp_agent_t *agent, const char *community, int for_write) /* проверить community на чтение или запись */
{ /* начало блока */
    const char *need; /* шаг community_ok */

    if (for_write) { /* если (for_write) */
        need = cfg_str(agent->cfg->community_write, "private"); /* задать need */
    } else { /* шаг community_ok */
        need = cfg_str(agent->cfg->community_read, "public"); /* задать need */
        if (strcmp(community, need) == 0) { /* если (strcmp(community, need) == 0) */
            return 1; /* вернуть 1 */
        } /* конец блока */
        need = cfg_str(agent->cfg->community_write, "private"); /* задать need */
    } /* конец блока */
    return strcmp(community, need) == 0; /* вернуть strcmp(community, need) == 0 */
} /* конец блока */

static int write_varbind(ber_out_t *out, /* собрать один varbind SEQUENCE */
                         const uint32_t *oid, /* шаг write_varbind */
                         size_t oid_len, /* шаг write_varbind */
                         const uint8_t *value, /* шаг write_varbind */
                         size_t value_len) /* шаг write_varbind */
{ /* начало блока */
    uint8_t body[192]; /* шаг write_varbind */
    ber_out_t inner; /* шаг write_varbind */

    inner.cur = body; /* задать inner.cur */
    inner.end = body + sizeof(body); /* задать inner.end */
    if (ber_write_oid(&inner, oid, oid_len) != SNMP_OK) { /* если (ber_write_oid(&inner, oid, oid_len) != SNMP_OK) */
        return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
    } /* конец блока */
    if (ber_write_bytes(&inner, value, value_len) != SNMP_OK) { /* если (ber_write_bytes(&inner, value, value_len) != SNMP_OK) */
        return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
    } /* конец блока */
    return ber_write_header(out, BER_TAG_SEQUENCE, body, (size_t)(inner.cur - body)); /* вернуть ber_write_header(out, BER_TAG_SEQUENCE, body, (size_t)(inner.cur - body)) */
} /* конец блока */

int snmp_agent_init(snmp_agent_t *agent, const snmp_agent_config_t *cfg) /* привязать настройки к агенту */
{ /* начало блока */
    if (agent == NULL || cfg == NULL) { /* если (agent == NULL || cfg == NULL) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    agent->cfg = cfg; /* задать agent->cfg */
    return SNMP_OK; /* вернуть SNMP_OK */
} /* конец блока */

int snmp_parse_message(const uint8_t *msg, size_t len, snmp_message_t *out) /* разобрать SNMP-сообщение */
{ /* начало блока */
    ber_in_t in; /* шаг snmp_parse_message */
    ber_in_t seq; /* шаг snmp_parse_message */
    ber_in_t pdu; /* шаг snmp_parse_message */
    ber_in_t vb_list; /* шаг snmp_parse_message */
    ber_in_t vb; /* шаг snmp_parse_message */
    ber_in_t value; /* шаг snmp_parse_message */
    uint8_t tag; /* шаг snmp_parse_message */

    if (msg == NULL || out == NULL) { /* если (msg == NULL || out == NULL) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    memset(out, 0, sizeof(*out)); /* обнулить структуру */
    in.cur = msg; /* задать in.cur */
    in.end = msg + len; /* задать in.end */

    if (ber_expect(&in, BER_TAG_SEQUENCE, &seq) != SNMP_OK) { /* если (ber_expect(&in, BER_TAG_SEQUENCE, &seq) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_int(&seq, &out->version) != SNMP_OK) { /* если (ber_read_int(&seq, &out->version) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_octets(&seq, out->community, sizeof(out->community), NULL) != SNMP_OK) { /* если (ber_read_octets(&seq, out->community, sizeof(out->community), NULL) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_header(&seq, &out->pdu_tag, &pdu) != SNMP_OK) { /* если (ber_read_header(&seq, &out->pdu_tag, &pdu) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_int(&pdu, &out->request_id) != SNMP_OK) { /* если (ber_read_int(&pdu, &out->request_id) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_int(&pdu, &out->error_status) != SNMP_OK) { /* если (ber_read_int(&pdu, &out->error_status) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_int(&pdu, &out->error_index) != SNMP_OK) { /* если (ber_read_int(&pdu, &out->error_index) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_expect(&pdu, BER_TAG_SEQUENCE, &vb_list) != SNMP_OK) { /* если (ber_expect(&pdu, BER_TAG_SEQUENCE, &vb_list) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_remain(&vb_list) == 0) { /* если (ber_remain(&vb_list) == 0) */
        return SNMP_OK; /* вернуть SNMP_OK */
    } /* конец блока */
    if (ber_expect(&vb_list, BER_TAG_SEQUENCE, &vb) != SNMP_OK) { /* если (ber_expect(&vb_list, BER_TAG_SEQUENCE, &vb) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_oid(&vb, out->oid, &out->oid_len) != SNMP_OK) { /* если (ber_read_oid(&vb, out->oid, &out->oid_len) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_header(&vb, &tag, &value) != SNMP_OK) { /* если (ber_read_header(&vb, &tag, &value) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    out->value_tag = tag; /* задать out->value_tag */
    if (tag == SNMP_TAG_INTEGER || tag == SNMP_TAG_TIMETICKS) { /* если (tag == SNMP_TAG_INTEGER || tag == SNMP_TAG_TIMETICKS) */
        ber_in_t rewind = value; /* задать ber_in_t rewind */
        uint8_t fake[8]; /* шаг snmp_parse_message */
        ber_out_t wrap; /* шаг snmp_parse_message */
        ber_in_t as_int; /* шаг snmp_parse_message */
        size_t n = (size_t)ber_remain(&value); /* задать size_t n */

        wrap.cur = fake; /* задать wrap.cur */
        wrap.end = fake + sizeof(fake); /* задать wrap.end */
        if (ber_write_header(&wrap, SNMP_TAG_INTEGER, rewind.cur, n) != SNMP_OK) { /* если (ber_write_header(&wrap, SNMP_TAG_INTEGER, rewind.cur, n) != SNMP_OK) */
            return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
        } /* конец блока */
        as_int.cur = fake; /* задать as_int.cur */
        as_int.end = wrap.cur; /* задать as_int.end */
        if (ber_read_int(&as_int, &out->int_value) != SNMP_OK) { /* если (ber_read_int(&as_int, &out->int_value) != SNMP_OK) */
            return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
        } /* конец блока */
    } else if (tag == SNMP_TAG_OCTET_STR) { /* шаг snmp_parse_message */
        size_t n = (size_t)ber_remain(&value); /* задать size_t n */
        if (n >= sizeof(out->str_value)) { /* если (n >= sizeof(out->str_value)) */
            n = sizeof(out->str_value) - 1; /* задать n */
        } /* конец блока */
        memcpy(out->str_value, value.cur, n); /* скопировать байты */
        out->str_value[n] = '\0'; /* задать out->str_value[n] */
        out->str_len = n; /* задать out->str_len */
    } /* конец блока */
    return SNMP_OK; /* вернуть SNMP_OK */
} /* конец блока */

static int parse_request(const uint8_t *request, /* разобрать входящий GET/GETNEXT/SET */
                         size_t request_len, /* шаг parse_request */
                         snmp_message_t *msg, /* шаг parse_request */
                         const uint8_t **value_body, /* шаг parse_request */
                         size_t *value_len) /* шаг parse_request */
{ /* начало блока */
    ber_in_t in; /* шаг parse_request */
    ber_in_t seq; /* шаг parse_request */
    ber_in_t pdu; /* шаг parse_request */
    ber_in_t vb_list; /* шаг parse_request */
    ber_in_t vb; /* шаг parse_request */
    ber_in_t value; /* шаг parse_request */

    memset(msg, 0, sizeof(*msg)); /* обнулить структуру */
    in.cur = request; /* задать in.cur */
    in.end = request + request_len; /* задать in.end */
    if (ber_expect(&in, BER_TAG_SEQUENCE, &seq) != SNMP_OK) { /* если (ber_expect(&in, BER_TAG_SEQUENCE, &seq) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_int(&seq, &msg->version) != SNMP_OK) { /* если (ber_read_int(&seq, &msg->version) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (msg->version != 0 && msg->version != 1) { /* если (msg->version != 0 && msg->version != 1) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_octets(&seq, msg->community, sizeof(msg->community), NULL) != SNMP_OK) { /* если (ber_read_octets(&seq, msg->community, sizeof(msg->community), NULL) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_header(&seq, &msg->pdu_tag, &pdu) != SNMP_OK) { /* если (ber_read_header(&seq, &msg->pdu_tag, &pdu) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (msg->pdu_tag != SNMP_PDU_GET && msg->pdu_tag != SNMP_PDU_GETNEXT && /* если (msg->pdu_tag != SNMP_PDU_GET && msg->pdu_tag != SNMP_PDU_GETNEXT && */
        msg->pdu_tag != SNMP_PDU_SET) { /* задать msg->pdu_tag ! */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_int(&pdu, &msg->request_id) != SNMP_OK) { /* если (ber_read_int(&pdu, &msg->request_id) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_int(&pdu, &msg->error_status) != SNMP_OK) { /* если (ber_read_int(&pdu, &msg->error_status) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_int(&pdu, &msg->error_index) != SNMP_OK) { /* если (ber_read_int(&pdu, &msg->error_index) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_expect(&pdu, BER_TAG_SEQUENCE, &vb_list) != SNMP_OK) { /* если (ber_expect(&pdu, BER_TAG_SEQUENCE, &vb_list) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_expect(&vb_list, BER_TAG_SEQUENCE, &vb) != SNMP_OK) { /* если (ber_expect(&vb_list, BER_TAG_SEQUENCE, &vb) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_oid(&vb, msg->oid, &msg->oid_len) != SNMP_OK) { /* если (ber_read_oid(&vb, msg->oid, &msg->oid_len) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    if (ber_read_header(&vb, &msg->value_tag, &value) != SNMP_OK) { /* если (ber_read_header(&vb, &msg->value_tag, &value) != SNMP_OK) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */
    *value_body = value.cur; /* задать value_body */
    *value_len = (size_t)ber_remain(&value); /* задать value_len */
    if (msg->value_tag == SNMP_TAG_INTEGER) { /* если (msg->value_tag == SNMP_TAG_INTEGER) */
        ber_in_t fake_src; /* шаг parse_request */
        uint8_t wrap[8]; /* шаг parse_request */
        ber_out_t w; /* шаг parse_request */
        ber_in_t as_int; /* шаг parse_request */

        w.cur = wrap; /* задать w.cur */
        w.end = wrap + sizeof(wrap); /* задать w.end */
        if (ber_write_header(&w, SNMP_TAG_INTEGER, value.cur, *value_len) != SNMP_OK) { /* если (ber_write_header(&w, SNMP_TAG_INTEGER, value.cur, *value_len) != SNMP_OK) */
            return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
        } /* конец блока */
        as_int.cur = wrap; /* задать as_int.cur */
        as_int.end = w.cur; /* задать as_int.end */
        (void)fake_src; /* явно не использовать */
        if (ber_read_int(&as_int, &msg->int_value) != SNMP_OK) { /* если (ber_read_int(&as_int, &msg->int_value) != SNMP_OK) */
            return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
        } /* конец блока */
    } /* конец блока */
    return SNMP_OK; /* вернуть SNMP_OK */
} /* конец блока */

static int encode_response(const snmp_message_t *req, /* собрать GetResponse */
                           int32_t error_status, /* шаг encode_response */
                           int32_t error_index, /* шаг encode_response */
                           const uint32_t *oid, /* шаг encode_response */
                           size_t oid_len, /* шаг encode_response */
                           const uint8_t *value, /* шаг encode_response */
                           size_t value_len, /* шаг encode_response */
                           uint8_t *response, /* шаг encode_response */
                           size_t response_max, /* шаг encode_response */
                           size_t *response_len) /* шаг encode_response */
{ /* начало блока */
    uint8_t vb[192]; /* шаг encode_response */
    uint8_t vbl[200]; /* шаг encode_response */
    uint8_t pdu[256]; /* шаг encode_response */
    uint8_t msg[320]; /* шаг encode_response */
    ber_out_t o; /* шаг encode_response */
    size_t comm_len; /* шаг encode_response */

    o.cur = vb; /* задать o.cur */
    o.end = vb + sizeof(vb); /* задать o.end */
    if (write_varbind(&o, oid, oid_len, value, value_len) != SNMP_OK) { /* если (write_varbind(&o, oid, oid_len, value, value_len) != SNMP_OK) */
        return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
    } /* конец блока */

    { /* начало блока */
        ber_out_t list; /* шаг encode_response */
        list.cur = vbl; /* задать list.cur */
        list.end = vbl + sizeof(vbl); /* задать list.end */
        if (ber_write_header(&list, BER_TAG_SEQUENCE, vb, (size_t)(o.cur - vb)) != SNMP_OK) { /* если (ber_write_header(&list, BER_TAG_SEQUENCE, vb, (size_t)(o.cur - vb)) != SNMP_OK) */
            return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
        } /* конец блока */

        { /* начало блока */
            ber_out_t p; /* шаг encode_response */
            p.cur = pdu; /* задать p.cur */
            p.end = pdu + sizeof(pdu); /* задать p.end */
            if (ber_write_int(&p, req->request_id) != SNMP_OK || /* если (ber_write_int(&p, req->request_id) != SNMP_OK || */
                ber_write_int(&p, error_status) != SNMP_OK || /* задать ber_write_int(&p, error_status) ! */
                ber_write_int(&p, error_index) != SNMP_OK || /* задать ber_write_int(&p, error_index) ! */
                ber_write_bytes(&p, vbl, (size_t)(list.cur - vbl)) != SNMP_OK) { /* задать ber_write_bytes(&p, vbl, (size_t)(list.cur - vbl)) ! */
                return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
            } /* конец блока */

            { /* начало блока */
                ber_out_t m; /* шаг encode_response */
                m.cur = msg; /* задать m.cur */
                m.end = msg + sizeof(msg); /* задать m.end */
                comm_len = strlen(req->community); /* длина строки */
                if (ber_write_int(&m, req->version) != SNMP_OK || /* если (ber_write_int(&m, req->version) != SNMP_OK || */
                    ber_write_octets(&m, req->community, comm_len) != SNMP_OK || /* задать ber_write_octets(&m, req->community, comm_len) ! */
                    ber_write_header(&m, SNMP_PDU_RESPONSE, pdu, (size_t)(p.cur - pdu)) != SNMP_OK) { /* задать ber_write_header(&m, SNMP_PDU_RESPONSE, pdu, (size_t)(p.cur - pdu)) ! */
                    return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
                } /* конец блока */

                { /* начало блока */
                    ber_out_t top; /* шаг encode_response */
                    top.cur = response; /* задать top.cur */
                    top.end = response + response_max; /* задать top.end */
                    if (ber_write_header(&top, BER_TAG_SEQUENCE, msg, (size_t)(m.cur - msg)) != SNMP_OK) { /* если (ber_write_header(&top, BER_TAG_SEQUENCE, msg, (size_t)(m.cur - msg)) != SNMP_OK) */
                        return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
                    } /* конец блока */
                    *response_len = (size_t)(top.cur - response); /* задать response_len */
                    return SNMP_OK; /* вернуть SNMP_OK */
                } /* конец блока */
            } /* конец блока */
        } /* конец блока */
    } /* конец блока */
} /* конец блока */

int snmp_agent_process(snmp_agent_t *agent, /* обработать запрос и построить ответ */
                       const uint8_t *request, /* шаг snmp_agent_process */
                       size_t request_len, /* шаг snmp_agent_process */
                       uint8_t *response, /* шаг snmp_agent_process */
                       size_t response_max, /* шаг snmp_agent_process */
                       size_t *response_len) /* шаг snmp_agent_process */
{ /* начало блока */
    snmp_message_t req; /* шаг snmp_agent_process */
    const uint8_t *set_body = NULL; /* задать const uint8_t *set_body */
    size_t set_len = 0; /* задать size_t set_len */
    uint8_t value[128]; /* шаг snmp_agent_process */
    ber_out_t value_out; /* шаг snmp_agent_process */
    int mib; /* функция mib; */
    const uint32_t *out_oid; /* шаг snmp_agent_process */
    size_t out_oid_len; /* шаг snmp_agent_process */
    int for_write; /* функция for_write; */
    int rc; /* функция rc; */

    if (agent == NULL || agent->cfg == NULL || request == NULL || /* если (agent == NULL || agent->cfg == NULL || request == NULL || */
        response == NULL || response_len == NULL) { /* задать response */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */

    rc = parse_request(request, request_len, &req, &set_body, &set_len); /* задать rc */
    if (rc != SNMP_OK) { /* если (rc != SNMP_OK) */
        return rc; /* вернуть rc */
    } /* конец блока */

    for_write = (req.pdu_tag == SNMP_PDU_SET); /* задать for_write */
    if (!community_ok(agent, req.community, for_write)) { /* если (!community_ok(agent, req.community, for_write)) */
        value_out.cur = value; /* задать value_out.cur */
        value_out.end = value + sizeof(value); /* задать value_out.end */
        if (ber_write_null(&value_out) != SNMP_OK) { /* если (ber_write_null(&value_out) != SNMP_OK) */
            return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
        } /* конец блока */
        return encode_response(&req, SNMP_ERRSTAT_AUTH, 1, req.oid, req.oid_len, /* вернуть encode_response(&req, SNMP_ERRSTAT_AUTH, 1, req.oid, req.oid_len, */
                               value, (size_t)(value_out.cur - value), /* шаг snmp_agent_process */
                               response, response_max, response_len); /* шаг snmp_agent_process */
    } /* конец блока */

    if (req.pdu_tag == SNMP_PDU_GETNEXT) { /* если (req.pdu_tag == SNMP_PDU_GETNEXT) */
        mib = find_next(req.oid, req.oid_len); /* задать mib */
        if (mib < 0) { /* если (mib < 0) */
            value_out.cur = value; /* задать value_out.cur */
            value_out.end = value + sizeof(value); /* задать value_out.end */
            if (ber_write_exception(&value_out, SNMP_EXC_ENDOFMIB) != SNMP_OK) { /* если (ber_write_exception(&value_out, SNMP_EXC_ENDOFMIB) != SNMP_OK) */
                return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
            } /* конец блока */
            return encode_response(&req, SNMP_ERRSTAT_NOERROR, 0, req.oid, req.oid_len, /* вернуть encode_response(&req, SNMP_ERRSTAT_NOERROR, 0, req.oid, req.oid_len, */
                                   value, (size_t)(value_out.cur - value), /* шаг snmp_agent_process */
                                   response, response_max, response_len); /* шаг snmp_agent_process */
        } /* конец блока */
        out_oid = k_mib_oids[mib]; /* задать out_oid */
        out_oid_len = k_mib_oid_len[mib]; /* задать out_oid_len */
    } else { /* шаг snmp_agent_process */
        mib = find_exact(req.oid, req.oid_len); /* задать mib */
        out_oid = req.oid; /* задать out_oid */
        out_oid_len = req.oid_len; /* задать out_oid_len */
        if (mib < 0) { /* если (mib < 0) */
            value_out.cur = value; /* задать value_out.cur */
            value_out.end = value + sizeof(value); /* задать value_out.end */
            if (ber_write_exception(&value_out, SNMP_EXC_NOSUCHOBJ) != SNMP_OK) { /* если (ber_write_exception(&value_out, SNMP_EXC_NOSUCHOBJ) != SNMP_OK) */
                return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
            } /* конец блока */
            return encode_response(&req, SNMP_ERRSTAT_NOERROR, 0, out_oid, out_oid_len, /* вернуть encode_response(&req, SNMP_ERRSTAT_NOERROR, 0, out_oid, out_oid_len, */
                                   value, (size_t)(value_out.cur - value), /* шаг snmp_agent_process */
                                   response, response_max, response_len); /* шаг snmp_agent_process */
        } /* конец блока */
    } /* конец блока */

    if (req.pdu_tag == SNMP_PDU_SET) { /* если (req.pdu_tag == SNMP_PDU_SET) */
        if (mib != MIB_LEDSTATE) { /* если (mib != MIB_LEDSTATE) */
            value_out.cur = value; /* задать value_out.cur */
            value_out.end = value + sizeof(value); /* задать value_out.end */
            if (ber_write_null(&value_out) != SNMP_OK) { /* если (ber_write_null(&value_out) != SNMP_OK) */
                return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
            } /* конец блока */
            return encode_response(&req, SNMP_ERRSTAT_NOTWRITABLE, 1, out_oid, out_oid_len, /* вернуть encode_response(&req, SNMP_ERRSTAT_NOTWRITABLE, 1, out_oid, out_oid_len, */
                                   value, (size_t)(value_out.cur - value), /* шаг snmp_agent_process */
                                   response, response_max, response_len); /* шаг snmp_agent_process */
        } /* конец блока */
        if (req.value_tag != SNMP_TAG_INTEGER || (req.int_value != 0 && req.int_value != 1)) { /* если (req.value_tag != SNMP_TAG_INTEGER || (req.int_value != 0 && req.int_value != 1)) */
            value_out.cur = value; /* задать value_out.cur */
            value_out.end = value + sizeof(value); /* задать value_out.end */
            if (ber_write_int(&value_out, req.int_value) != SNMP_OK) { /* если (ber_write_int(&value_out, req.int_value) != SNMP_OK) */
                return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
            } /* конец блока */
            return encode_response(&req, SNMP_ERRSTAT_BADVALUE, 1, out_oid, out_oid_len, /* вернуть encode_response(&req, SNMP_ERRSTAT_BADVALUE, 1, out_oid, out_oid_len, */
                                   value, (size_t)(value_out.cur - value), /* шаг snmp_agent_process */
                                   response, response_max, response_len); /* шаг snmp_agent_process */
        } /* конец блока */
        if (agent->cfg->set_led != NULL && agent->cfg->set_led(req.int_value) != 0) { /* если (agent->cfg->set_led != NULL && agent->cfg->set_led(req.int_value) != 0) */
            value_out.cur = value; /* задать value_out.cur */
            value_out.end = value + sizeof(value); /* задать value_out.end */
            if (ber_write_int(&value_out, req.int_value) != SNMP_OK) { /* если (ber_write_int(&value_out, req.int_value) != SNMP_OK) */
                return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
            } /* конец блока */
            return encode_response(&req, SNMP_ERRSTAT_GENERR, 1, out_oid, out_oid_len, /* вернуть encode_response(&req, SNMP_ERRSTAT_GENERR, 1, out_oid, out_oid_len, */
                                   value, (size_t)(value_out.cur - value), /* шаг snmp_agent_process */
                                   response, response_max, response_len); /* шаг snmp_agent_process */
        } /* конец блока */
    } /* конец блока */

    value_out.cur = value; /* задать value_out.cur */
    value_out.end = value + sizeof(value); /* задать value_out.end */
    if (write_mib_value(&value_out, agent, mib) != SNMP_OK) { /* если (write_mib_value(&value_out, agent, mib) != SNMP_OK) */
        return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
    } /* конец блока */
    return encode_response(&req, SNMP_ERRSTAT_NOERROR, 0, out_oid, out_oid_len, /* вернуть encode_response(&req, SNMP_ERRSTAT_NOERROR, 0, out_oid, out_oid_len, */
                           value, (size_t)(value_out.cur - value), /* шаг snmp_agent_process */
                           response, response_max, response_len); /* шаг snmp_agent_process */
} /* конец блока */

int snmp_build_request(uint8_t pdu_tag, /* собрать тестовый запрос */
                       int32_t request_id, /* шаг snmp_build_request */
                       const char *community, /* шаг snmp_build_request */
                       const uint32_t *oid, /* шаг snmp_build_request */
                       size_t oid_len, /* шаг snmp_build_request */
                       uint8_t value_tag, /* шаг snmp_build_request */
                       const void *value, /* шаг snmp_build_request */
                       size_t value_len, /* шаг snmp_build_request */
                       uint8_t *out, /* шаг snmp_build_request */
                       size_t out_max, /* шаг snmp_build_request */
                       size_t *out_len) /* шаг snmp_build_request */
{ /* начало блока */
    uint8_t val[96]; /* шаг snmp_build_request */
    uint8_t vb[160]; /* шаг snmp_build_request */
    uint8_t vbl[168]; /* шаг snmp_build_request */
    uint8_t pdu[220]; /* шаг snmp_build_request */
    uint8_t msg[280]; /* шаг snmp_build_request */
    ber_out_t o; /* шаг snmp_build_request */
    snmp_message_t dummy; /* шаг snmp_build_request */

    (void)dummy; /* явно не использовать */
    if (community == NULL || oid == NULL || out == NULL || out_len == NULL) { /* если (community == NULL || oid == NULL || out == NULL || out_len == NULL) */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */

    o.cur = val; /* задать o.cur */
    o.end = val + sizeof(val); /* задать o.end */
    if (value_tag == SNMP_TAG_NULL || value == NULL) { /* если (value_tag == SNMP_TAG_NULL || value == NULL) */
        if (ber_write_null(&o) != SNMP_OK) { /* если (ber_write_null(&o) != SNMP_OK) */
            return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
        } /* конец блока */
    } else if (value_tag == SNMP_TAG_INTEGER) { /* шаг snmp_build_request */
        int32_t v; /* шаг snmp_build_request */
        memcpy(&v, value, sizeof(v)); /* скопировать байты */
        if (ber_write_int(&o, v) != SNMP_OK) { /* если (ber_write_int(&o, v) != SNMP_OK) */
            return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
        } /* конец блока */
    } else if (value_tag == SNMP_TAG_OCTET_STR) { /* шаг snmp_build_request */
        if (ber_write_octets(&o, (const char *)value, value_len) != SNMP_OK) { /* если (ber_write_octets(&o, (const char *)value, value_len) != SNMP_OK) */
            return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
        } /* конец блока */
    } else { /* шаг snmp_build_request */
        return SNMP_ERR_PARSE; /* вернуть SNMP_ERR_PARSE */
    } /* конец блока */

    { /* начало блока */
        ber_out_t vbo; /* шаг snmp_build_request */
        vbo.cur = vb; /* задать vbo.cur */
        vbo.end = vb + sizeof(vb); /* задать vbo.end */
        if (write_varbind(&vbo, oid, oid_len, val, (size_t)(o.cur - val)) != SNMP_OK) { /* если (write_varbind(&vbo, oid, oid_len, val, (size_t)(o.cur - val)) != SNMP_OK) */
            return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
        } /* конец блока */

        { /* начало блока */
            ber_out_t list; /* шаг snmp_build_request */
            list.cur = vbl; /* задать list.cur */
            list.end = vbl + sizeof(vbl); /* задать list.end */
            if (ber_write_header(&list, BER_TAG_SEQUENCE, vb, (size_t)(vbo.cur - vb)) != SNMP_OK) { /* если (ber_write_header(&list, BER_TAG_SEQUENCE, vb, (size_t)(vbo.cur - vb)) != SNMP_OK) */
                return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
            } /* конец блока */

            { /* начало блока */
                ber_out_t p; /* шаг snmp_build_request */
                p.cur = pdu; /* задать p.cur */
                p.end = pdu + sizeof(pdu); /* задать p.end */
                if (ber_write_int(&p, request_id) != SNMP_OK || /* если (ber_write_int(&p, request_id) != SNMP_OK || */
                    ber_write_int(&p, 0) != SNMP_OK || /* задать ber_write_int(&p, 0) ! */
                    ber_write_int(&p, 0) != SNMP_OK || /* задать ber_write_int(&p, 0) ! */
                    ber_write_bytes(&p, vbl, (size_t)(list.cur - vbl)) != SNMP_OK) { /* задать ber_write_bytes(&p, vbl, (size_t)(list.cur - vbl)) ! */
                    return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
                } /* конец блока */

                { /* начало блока */
                    ber_out_t m; /* шаг snmp_build_request */
                    m.cur = msg; /* задать m.cur */
                    m.end = msg + sizeof(msg); /* задать m.end */
                    if (ber_write_int(&m, 1) != SNMP_OK || /* если (ber_write_int(&m, 1) != SNMP_OK || */
                        ber_write_octets(&m, community, strlen(community)) != SNMP_OK || /* длина строки */
                        ber_write_header(&m, pdu_tag, pdu, (size_t)(p.cur - pdu)) != SNMP_OK) { /* задать ber_write_header(&m, pdu_tag, pdu, (size_t)(p.cur - pdu)) ! */
                        return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
                    } /* конец блока */

                    { /* начало блока */
                        ber_out_t top; /* шаг snmp_build_request */
                        top.cur = out; /* задать top.cur */
                        top.end = out + out_max; /* задать top.end */
                        if (ber_write_header(&top, BER_TAG_SEQUENCE, msg, (size_t)(m.cur - msg)) != SNMP_OK) { /* если (ber_write_header(&top, BER_TAG_SEQUENCE, msg, (size_t)(m.cur - msg)) != SNMP_OK) */
                            return SNMP_ERR_TOO_BIG; /* вернуть SNMP_ERR_TOO_BIG */
                        } /* конец блока */
                        *out_len = (size_t)(top.cur - out); /* задать out_len */
                        return SNMP_OK; /* вернуть SNMP_OK */
                    } /* конец блока */
                } /* конец блока */
            } /* конец блока */
        } /* конец блока */
    } /* конец блока */
} /* конец блока */

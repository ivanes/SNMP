#include "snmp.h"

#include <string.h>

#define BER_TAG_SEQUENCE 0x30u
#define SYS_OBJECT_ARCS_N 9

static const uint32_t k_sys_object_id[SYS_OBJECT_ARCS_N] = {
    1, 3, 6, 1, 4, 1, 99999, 1, 1
};

typedef struct {
    const uint8_t *cur;
    const uint8_t *end;
} ber_in_t;

typedef struct {
    uint8_t *cur;
    uint8_t *end;
} ber_out_t;

typedef enum {
    MIB_SYSDESCR = 0,
    MIB_SYSOBJECTID,
    MIB_SYSUPTIME,
    MIB_SYSCONTACT,
    MIB_SYSNAME,
    MIB_SYSLOCATION,
    MIB_SYSSERVICES,
    MIB_LEDSTATE,
    MIB_COUNT
} mib_id_t;

static const uint32_t k_mib_oids[MIB_COUNT][SNMP_OID_MAX_ARCS] = {
    {1, 3, 6, 1, 2, 1, 1, 1, 0},
    {1, 3, 6, 1, 2, 1, 1, 2, 0},
    {1, 3, 6, 1, 2, 1, 1, 3, 0},
    {1, 3, 6, 1, 2, 1, 1, 4, 0},
    {1, 3, 6, 1, 2, 1, 1, 5, 0},
    {1, 3, 6, 1, 2, 1, 1, 6, 0},
    {1, 3, 6, 1, 2, 1, 1, 7, 0},
    {1, 3, 6, 1, 4, 1, 99999, 1, 1, 0},
};

static const uint8_t k_mib_oid_len[MIB_COUNT] = {
    9, 9, 9, 9, 9, 9, 9, 10
};

static int oid_cmp(const uint32_t *a, size_t a_len, const uint32_t *b, size_t b_len)
{
    size_t n = (a_len < b_len) ? a_len : b_len;
    size_t i;

    for (i = 0; i < n; i++) {
        if (a[i] < b[i]) {
            return -1;
        }
        if (a[i] > b[i]) {
            return 1;
        }
    }
    if (a_len < b_len) {
        return -1;
    }
    if (a_len > b_len) {
        return 1;
    }
    return 0;
}

int snmp_oid_from_text(const char *text, uint32_t *arcs, size_t *n_arcs)
{
    size_t count = 0;
    const char *p = text;

    if (text == NULL || arcs == NULL || n_arcs == NULL) {
        return SNMP_ERR_PARSE;
    }

    while (*p != '\0') {
        unsigned long value = 0;
        int digits = 0;

        if (*p == '.') {
            p++;
        }
        if (*p < '0' || *p > '9') {
            return SNMP_ERR_PARSE;
        }
        while (*p >= '0' && *p <= '9') {
            value = (value * 10u) + (unsigned long)(*p - '0');
            p++;
            digits++;
            if (digits > 10) {
                return SNMP_ERR_PARSE;
            }
        }
        if (count >= SNMP_OID_MAX_ARCS) {
            return SNMP_ERR_PARSE;
        }
        arcs[count++] = (uint32_t)value;
        if (*p != '\0' && *p != '.') {
            return SNMP_ERR_PARSE;
        }
    }

    if (count < 2) {
        return SNMP_ERR_PARSE;
    }
    *n_arcs = count;
    return SNMP_OK;
}

static int ber_remain(const ber_in_t *in)
{
    return (int)(in->end - in->cur);
}

static int ber_read_byte(ber_in_t *in, uint8_t *out)
{
    if (ber_remain(in) < 1) {
        return SNMP_ERR_PARSE;
    }
    *out = *in->cur++;
    return SNMP_OK;
}

static int ber_read_length(ber_in_t *in, size_t *len)
{
    uint8_t first;

    if (ber_read_byte(in, &first) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if ((first & 0x80u) == 0) {
        *len = first;
        return SNMP_OK;
    }

    {
        unsigned nbytes = first & 0x7Fu;
        size_t value = 0;
        unsigned i;

        if (nbytes == 0 || nbytes > 2) {
            return SNMP_ERR_PARSE;
        }
        for (i = 0; i < nbytes; i++) {
            uint8_t b;
            if (ber_read_byte(in, &b) != SNMP_OK) {
                return SNMP_ERR_PARSE;
            }
            value = (value << 8) | b;
        }
        *len = value;
        return SNMP_OK;
    }
}

static int ber_read_header(ber_in_t *in, uint8_t *tag, ber_in_t *body)
{
    size_t len;

    if (ber_read_byte(in, tag) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_length(in, &len) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if ((size_t)ber_remain(in) < len) {
        return SNMP_ERR_PARSE;
    }
    body->cur = in->cur;
    body->end = in->cur + len;
    in->cur += len;
    return SNMP_OK;
}

static int ber_expect(ber_in_t *in, uint8_t expected, ber_in_t *body)
{
    uint8_t tag;
    if (ber_read_header(in, &tag, body) != SNMP_OK || tag != expected) {
        return SNMP_ERR_PARSE;
    }
    return SNMP_OK;
}

static int ber_read_int(ber_in_t *in, int32_t *value)
{
    ber_in_t body;
    size_t n;
    uint32_t acc = 0;
    uint8_t first;

    if (ber_expect(in, SNMP_TAG_INTEGER, &body) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    n = (size_t)ber_remain(&body);
    if (n == 0 || n > 4) {
        return SNMP_ERR_PARSE;
    }
    first = body.cur[0];
    if ((first & 0x80u) != 0) {
        acc = 0xFFFFFFFFu;
    }
    while (ber_remain(&body) > 0) {
        uint8_t b;
        if (ber_read_byte(&body, &b) != SNMP_OK) {
            return SNMP_ERR_PARSE;
        }
        acc = (acc << 8) | b;
    }
    *value = (int32_t)acc;
    return SNMP_OK;
}

static int ber_read_octets(ber_in_t *in, char *out, size_t out_max, size_t *out_len)
{
    ber_in_t body;
    size_t n;

    if (ber_expect(in, SNMP_TAG_OCTET_STR, &body) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    n = (size_t)ber_remain(&body);
    if (n >= out_max) {
        return SNMP_ERR_PARSE;
    }
    memcpy(out, body.cur, n);
    out[n] = '\0';
    if (out_len != NULL) {
        *out_len = n;
    }
    return SNMP_OK;
}

static int ber_read_oid(ber_in_t *in, uint32_t *arcs, size_t *n_arcs)
{
    ber_in_t body;
    size_t count = 0;
    uint8_t first;

    if (ber_expect(in, SNMP_TAG_OID, &body) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_remain(&body) < 1) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_byte(&body, &first) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    arcs[count++] = first / 40u;
    arcs[count++] = first % 40u;

    while (ber_remain(&body) > 0) {
        uint32_t arc = 0;
        int guard = 0;
        uint8_t b;
        do {
            if (ber_read_byte(&body, &b) != SNMP_OK) {
                return SNMP_ERR_PARSE;
            }
            arc = (arc << 7) | (uint32_t)(b & 0x7Fu);
            if (++guard > 5) {
                return SNMP_ERR_PARSE;
            }
        } while ((b & 0x80u) != 0);
        if (count >= SNMP_OID_MAX_ARCS) {
            return SNMP_ERR_PARSE;
        }
        arcs[count++] = arc;
    }
    *n_arcs = count;
    return SNMP_OK;
}

static int ber_write_bytes(ber_out_t *out, const uint8_t *data, size_t n)
{
    if ((size_t)(out->end - out->cur) < n) {
        return SNMP_ERR_TOO_BIG;
    }
    memcpy(out->cur, data, n);
    out->cur += n;
    return SNMP_OK;
}

static int ber_write_byte(ber_out_t *out, uint8_t b)
{
    return ber_write_bytes(out, &b, 1);
}

static int ber_write_length(ber_out_t *out, size_t len)
{
    if (len < 0x80u) {
        return ber_write_byte(out, (uint8_t)len);
    }
    if (len <= 0xFFu) {
        if (ber_write_byte(out, 0x81u) != SNMP_OK) {
            return SNMP_ERR_TOO_BIG;
        }
        return ber_write_byte(out, (uint8_t)len);
    }
    if (len > 0xFFFFu) {
        return SNMP_ERR_TOO_BIG;
    }
    if (ber_write_byte(out, 0x82u) != SNMP_OK) {
        return SNMP_ERR_TOO_BIG;
    }
    if (ber_write_byte(out, (uint8_t)((len >> 8) & 0xFFu)) != SNMP_OK) {
        return SNMP_ERR_TOO_BIG;
    }
    return ber_write_byte(out, (uint8_t)(len & 0xFFu));
}

static int ber_write_header(ber_out_t *out, uint8_t tag, const uint8_t *body, size_t body_len)
{
    if (ber_write_byte(out, tag) != SNMP_OK) {
        return SNMP_ERR_TOO_BIG;
    }
    if (ber_write_length(out, body_len) != SNMP_OK) {
        return SNMP_ERR_TOO_BIG;
    }
    return ber_write_bytes(out, body, body_len);
}

static int ber_write_int_tag(ber_out_t *out, uint8_t tag, int32_t value)
{
    uint8_t buf[4];
    uint32_t u = (uint32_t)value;
    int start = 0;

    buf[0] = (uint8_t)((u >> 24) & 0xFFu);
    buf[1] = (uint8_t)((u >> 16) & 0xFFu);
    buf[2] = (uint8_t)((u >> 8) & 0xFFu);
    buf[3] = (uint8_t)(u & 0xFFu);

    while (start < 3) {
        int sign_ext = ((buf[start] == 0x00) && ((buf[start + 1] & 0x80u) == 0));
        int neg_ext = ((buf[start] == 0xFF) && ((buf[start + 1] & 0x80u) != 0));
        if (!sign_ext && !neg_ext) {
            break;
        }
        start++;
    }
    return ber_write_header(out, tag, buf + start, (size_t)(4 - start));
}

static int ber_write_int(ber_out_t *out, int32_t value)
{
    return ber_write_int_tag(out, SNMP_TAG_INTEGER, value);
}

static int ber_write_octets(ber_out_t *out, const char *s, size_t n)
{
    return ber_write_header(out, SNMP_TAG_OCTET_STR, (const uint8_t *)s, n);
}

static int ber_write_null(ber_out_t *out)
{
    return ber_write_header(out, SNMP_TAG_NULL, NULL, 0);
}

static int ber_write_exception(ber_out_t *out, uint8_t tag)
{
    return ber_write_header(out, tag, NULL, 0);
}

static int ber_write_oid(ber_out_t *out, const uint32_t *arcs, size_t n_arcs)
{
    uint8_t buf[SNMP_OID_MAX_ARCS * 5];
    size_t n = 0;
    size_t i;

    if (n_arcs < 2 || arcs[0] > 2 || arcs[1] > 39) {
        return SNMP_ERR_PARSE;
    }
    buf[n++] = (uint8_t)(40u * arcs[0] + arcs[1]);
    for (i = 2; i < n_arcs; i++) {
        uint32_t arc = arcs[i];
        uint8_t tmp[5];
        int t = 0;
        tmp[t++] = (uint8_t)(arc & 0x7Fu);
        arc >>= 7;
        while (arc != 0) {
            tmp[t++] = (uint8_t)((arc & 0x7Fu) | 0x80u);
            arc >>= 7;
        }
        while (t > 0) {
            buf[n++] = tmp[--t];
        }
    }
    return ber_write_header(out, SNMP_TAG_OID, buf, n);
}

static int find_exact(const uint32_t *oid, size_t oid_len)
{
    int i;
    for (i = 0; i < MIB_COUNT; i++) {
        if (oid_cmp(oid, oid_len, k_mib_oids[i], k_mib_oid_len[i]) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_next(const uint32_t *oid, size_t oid_len)
{
    int i;
    int best = -1;

    for (i = 0; i < MIB_COUNT; i++) {
        if (oid_cmp(k_mib_oids[i], k_mib_oid_len[i], oid, oid_len) > 0) {
            if (best < 0 || oid_cmp(k_mib_oids[i], k_mib_oid_len[i],
                                    k_mib_oids[best], k_mib_oid_len[best]) < 0) {
                best = i;
            }
        }
    }
    return best;
}

static const char *cfg_str(const char *value, const char *fallback)
{
    return (value != NULL) ? value : fallback;
}

static int write_mib_value(ber_out_t *out, const snmp_agent_t *agent, int id)
{
    const snmp_agent_config_t *cfg = agent->cfg;
    const char *s;
    uint32_t uptime;

    switch (id) {
    case MIB_SYSDESCR:
        s = cfg_str(cfg->sys_descr, "STM32F411 FreeRTOS SNMP");
        return ber_write_octets(out, s, strlen(s));
    case MIB_SYSOBJECTID:
        return ber_write_oid(out, k_sys_object_id, SYS_OBJECT_ARCS_N);
    case MIB_SYSUPTIME:
        uptime = (cfg->get_uptime_cs != NULL) ? cfg->get_uptime_cs() : 0u;
        return ber_write_int_tag(out, SNMP_TAG_TIMETICKS, (int32_t)uptime);
    case MIB_SYSCONTACT:
        s = cfg_str(cfg->sys_contact, "");
        return ber_write_octets(out, s, strlen(s));
    case MIB_SYSNAME:
        s = cfg_str(cfg->sys_name, "snmp-f411");
        return ber_write_octets(out, s, strlen(s));
    case MIB_SYSLOCATION:
        s = cfg_str(cfg->sys_location, "");
        return ber_write_octets(out, s, strlen(s));
    case MIB_SYSSERVICES:
        return ber_write_int(out, cfg->sys_services);
    case MIB_LEDSTATE:
        return ber_write_int(out, (cfg->get_led != NULL) ? cfg->get_led() : 0);
    default:
        return SNMP_ERR_INTERNAL;
    }
}

static int community_ok(const snmp_agent_t *agent, const char *community, int for_write)
{
    const char *need;

    if (for_write) {
        need = cfg_str(agent->cfg->community_write, "private");
    } else {
        need = cfg_str(agent->cfg->community_read, "public");
        if (strcmp(community, need) == 0) {
            return 1;
        }
        need = cfg_str(agent->cfg->community_write, "private");
    }
    return strcmp(community, need) == 0;
}

static int write_varbind(ber_out_t *out,
                         const uint32_t *oid,
                         size_t oid_len,
                         const uint8_t *value,
                         size_t value_len)
{
    uint8_t body[192];
    ber_out_t inner;

    inner.cur = body;
    inner.end = body + sizeof(body);
    if (ber_write_oid(&inner, oid, oid_len) != SNMP_OK) {
        return SNMP_ERR_TOO_BIG;
    }
    if (ber_write_bytes(&inner, value, value_len) != SNMP_OK) {
        return SNMP_ERR_TOO_BIG;
    }
    return ber_write_header(out, BER_TAG_SEQUENCE, body, (size_t)(inner.cur - body));
}

int snmp_agent_init(snmp_agent_t *agent, const snmp_agent_config_t *cfg)
{
    if (agent == NULL || cfg == NULL) {
        return SNMP_ERR_PARSE;
    }
    agent->cfg = cfg;
    return SNMP_OK;
}

int snmp_parse_message(const uint8_t *msg, size_t len, snmp_message_t *out)
{
    ber_in_t in;
    ber_in_t seq;
    ber_in_t pdu;
    ber_in_t vb_list;
    ber_in_t vb;
    ber_in_t value;
    uint8_t tag;

    if (msg == NULL || out == NULL) {
        return SNMP_ERR_PARSE;
    }
    memset(out, 0, sizeof(*out));
    in.cur = msg;
    in.end = msg + len;

    if (ber_expect(&in, BER_TAG_SEQUENCE, &seq) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_int(&seq, &out->version) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_octets(&seq, out->community, sizeof(out->community), NULL) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_header(&seq, &out->pdu_tag, &pdu) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_int(&pdu, &out->request_id) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_int(&pdu, &out->error_status) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_int(&pdu, &out->error_index) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_expect(&pdu, BER_TAG_SEQUENCE, &vb_list) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_remain(&vb_list) == 0) {
        return SNMP_OK;
    }
    if (ber_expect(&vb_list, BER_TAG_SEQUENCE, &vb) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_oid(&vb, out->oid, &out->oid_len) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_header(&vb, &tag, &value) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    out->value_tag = tag;
    if (tag == SNMP_TAG_INTEGER || tag == SNMP_TAG_TIMETICKS) {
        ber_in_t rewind = value;
        uint8_t fake[8];
        ber_out_t wrap;
        ber_in_t as_int;
        size_t n = (size_t)ber_remain(&value);

        wrap.cur = fake;
        wrap.end = fake + sizeof(fake);
        if (ber_write_header(&wrap, SNMP_TAG_INTEGER, rewind.cur, n) != SNMP_OK) {
            return SNMP_ERR_PARSE;
        }
        as_int.cur = fake;
        as_int.end = wrap.cur;
        if (ber_read_int(&as_int, &out->int_value) != SNMP_OK) {
            return SNMP_ERR_PARSE;
        }
    } else if (tag == SNMP_TAG_OCTET_STR) {
        size_t n = (size_t)ber_remain(&value);
        if (n >= sizeof(out->str_value)) {
            n = sizeof(out->str_value) - 1;
        }
        memcpy(out->str_value, value.cur, n);
        out->str_value[n] = '\0';
        out->str_len = n;
    }
    return SNMP_OK;
}

static int parse_request(const uint8_t *request,
                         size_t request_len,
                         snmp_message_t *msg,
                         const uint8_t **value_body,
                         size_t *value_len)
{
    ber_in_t in;
    ber_in_t seq;
    ber_in_t pdu;
    ber_in_t vb_list;
    ber_in_t vb;
    ber_in_t value;

    memset(msg, 0, sizeof(*msg));
    in.cur = request;
    in.end = request + request_len;
    if (ber_expect(&in, BER_TAG_SEQUENCE, &seq) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_int(&seq, &msg->version) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (msg->version != 0 && msg->version != 1) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_octets(&seq, msg->community, sizeof(msg->community), NULL) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_header(&seq, &msg->pdu_tag, &pdu) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (msg->pdu_tag != SNMP_PDU_GET && msg->pdu_tag != SNMP_PDU_GETNEXT &&
        msg->pdu_tag != SNMP_PDU_SET) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_int(&pdu, &msg->request_id) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_int(&pdu, &msg->error_status) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_int(&pdu, &msg->error_index) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_expect(&pdu, BER_TAG_SEQUENCE, &vb_list) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_expect(&vb_list, BER_TAG_SEQUENCE, &vb) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_oid(&vb, msg->oid, &msg->oid_len) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    if (ber_read_header(&vb, &msg->value_tag, &value) != SNMP_OK) {
        return SNMP_ERR_PARSE;
    }
    *value_body = value.cur;
    *value_len = (size_t)ber_remain(&value);
    if (msg->value_tag == SNMP_TAG_INTEGER) {
        ber_in_t fake_src;
        uint8_t wrap[8];
        ber_out_t w;
        ber_in_t as_int;

        w.cur = wrap;
        w.end = wrap + sizeof(wrap);
        if (ber_write_header(&w, SNMP_TAG_INTEGER, value.cur, *value_len) != SNMP_OK) {
            return SNMP_ERR_PARSE;
        }
        as_int.cur = wrap;
        as_int.end = w.cur;
        (void)fake_src;
        if (ber_read_int(&as_int, &msg->int_value) != SNMP_OK) {
            return SNMP_ERR_PARSE;
        }
    }
    return SNMP_OK;
}

static int encode_response(const snmp_message_t *req,
                           int32_t error_status,
                           int32_t error_index,
                           const uint32_t *oid,
                           size_t oid_len,
                           const uint8_t *value,
                           size_t value_len,
                           uint8_t *response,
                           size_t response_max,
                           size_t *response_len)
{
    uint8_t vb[192];
    uint8_t vbl[200];
    uint8_t pdu[256];
    uint8_t msg[320];
    ber_out_t o;
    size_t comm_len;

    o.cur = vb;
    o.end = vb + sizeof(vb);
    if (write_varbind(&o, oid, oid_len, value, value_len) != SNMP_OK) {
        return SNMP_ERR_TOO_BIG;
    }

    {
        ber_out_t list;
        list.cur = vbl;
        list.end = vbl + sizeof(vbl);
        if (ber_write_header(&list, BER_TAG_SEQUENCE, vb, (size_t)(o.cur - vb)) != SNMP_OK) {
            return SNMP_ERR_TOO_BIG;
        }

        {
            ber_out_t p;
            p.cur = pdu;
            p.end = pdu + sizeof(pdu);
            if (ber_write_int(&p, req->request_id) != SNMP_OK ||
                ber_write_int(&p, error_status) != SNMP_OK ||
                ber_write_int(&p, error_index) != SNMP_OK ||
                ber_write_bytes(&p, vbl, (size_t)(list.cur - vbl)) != SNMP_OK) {
                return SNMP_ERR_TOO_BIG;
            }

            {
                ber_out_t m;
                m.cur = msg;
                m.end = msg + sizeof(msg);
                comm_len = strlen(req->community);
                if (ber_write_int(&m, req->version) != SNMP_OK ||
                    ber_write_octets(&m, req->community, comm_len) != SNMP_OK ||
                    ber_write_header(&m, SNMP_PDU_RESPONSE, pdu, (size_t)(p.cur - pdu)) != SNMP_OK) {
                    return SNMP_ERR_TOO_BIG;
                }

                {
                    ber_out_t top;
                    top.cur = response;
                    top.end = response + response_max;
                    if (ber_write_header(&top, BER_TAG_SEQUENCE, msg, (size_t)(m.cur - msg)) != SNMP_OK) {
                        return SNMP_ERR_TOO_BIG;
                    }
                    *response_len = (size_t)(top.cur - response);
                    return SNMP_OK;
                }
            }
        }
    }
}

int snmp_agent_process(snmp_agent_t *agent,
                       const uint8_t *request,
                       size_t request_len,
                       uint8_t *response,
                       size_t response_max,
                       size_t *response_len)
{
    snmp_message_t req;
    const uint8_t *set_body = NULL;
    size_t set_len = 0;
    uint8_t value[128];
    ber_out_t value_out;
    int mib;
    const uint32_t *out_oid;
    size_t out_oid_len;
    int for_write;
    int rc;

    if (agent == NULL || agent->cfg == NULL || request == NULL ||
        response == NULL || response_len == NULL) {
        return SNMP_ERR_PARSE;
    }

    rc = parse_request(request, request_len, &req, &set_body, &set_len);
    if (rc != SNMP_OK) {
        return rc;
    }

    for_write = (req.pdu_tag == SNMP_PDU_SET);
    if (!community_ok(agent, req.community, for_write)) {
        value_out.cur = value;
        value_out.end = value + sizeof(value);
        if (ber_write_null(&value_out) != SNMP_OK) {
            return SNMP_ERR_TOO_BIG;
        }
        return encode_response(&req, SNMP_ERRSTAT_AUTH, 1, req.oid, req.oid_len,
                               value, (size_t)(value_out.cur - value),
                               response, response_max, response_len);
    }

    if (req.pdu_tag == SNMP_PDU_GETNEXT) {
        mib = find_next(req.oid, req.oid_len);
        if (mib < 0) {
            value_out.cur = value;
            value_out.end = value + sizeof(value);
            if (ber_write_exception(&value_out, SNMP_EXC_ENDOFMIB) != SNMP_OK) {
                return SNMP_ERR_TOO_BIG;
            }
            return encode_response(&req, SNMP_ERRSTAT_NOERROR, 0, req.oid, req.oid_len,
                                   value, (size_t)(value_out.cur - value),
                                   response, response_max, response_len);
        }
        out_oid = k_mib_oids[mib];
        out_oid_len = k_mib_oid_len[mib];
    } else {
        mib = find_exact(req.oid, req.oid_len);
        out_oid = req.oid;
        out_oid_len = req.oid_len;
        if (mib < 0) {
            value_out.cur = value;
            value_out.end = value + sizeof(value);
            if (ber_write_exception(&value_out, SNMP_EXC_NOSUCHOBJ) != SNMP_OK) {
                return SNMP_ERR_TOO_BIG;
            }
            return encode_response(&req, SNMP_ERRSTAT_NOERROR, 0, out_oid, out_oid_len,
                                   value, (size_t)(value_out.cur - value),
                                   response, response_max, response_len);
        }
    }

    if (req.pdu_tag == SNMP_PDU_SET) {
        if (mib != MIB_LEDSTATE) {
            value_out.cur = value;
            value_out.end = value + sizeof(value);
            if (ber_write_null(&value_out) != SNMP_OK) {
                return SNMP_ERR_TOO_BIG;
            }
            return encode_response(&req, SNMP_ERRSTAT_NOTWRITABLE, 1, out_oid, out_oid_len,
                                   value, (size_t)(value_out.cur - value),
                                   response, response_max, response_len);
        }
        if (req.value_tag != SNMP_TAG_INTEGER || (req.int_value != 0 && req.int_value != 1)) {
            value_out.cur = value;
            value_out.end = value + sizeof(value);
            if (ber_write_int(&value_out, req.int_value) != SNMP_OK) {
                return SNMP_ERR_TOO_BIG;
            }
            return encode_response(&req, SNMP_ERRSTAT_BADVALUE, 1, out_oid, out_oid_len,
                                   value, (size_t)(value_out.cur - value),
                                   response, response_max, response_len);
        }
        if (agent->cfg->set_led != NULL && agent->cfg->set_led(req.int_value) != 0) {
            value_out.cur = value;
            value_out.end = value + sizeof(value);
            if (ber_write_int(&value_out, req.int_value) != SNMP_OK) {
                return SNMP_ERR_TOO_BIG;
            }
            return encode_response(&req, SNMP_ERRSTAT_GENERR, 1, out_oid, out_oid_len,
                                   value, (size_t)(value_out.cur - value),
                                   response, response_max, response_len);
        }
    }

    value_out.cur = value;
    value_out.end = value + sizeof(value);
    if (write_mib_value(&value_out, agent, mib) != SNMP_OK) {
        return SNMP_ERR_TOO_BIG;
    }
    return encode_response(&req, SNMP_ERRSTAT_NOERROR, 0, out_oid, out_oid_len,
                           value, (size_t)(value_out.cur - value),
                           response, response_max, response_len);
}

int snmp_build_request(uint8_t pdu_tag,
                       int32_t request_id,
                       const char *community,
                       const uint32_t *oid,
                       size_t oid_len,
                       uint8_t value_tag,
                       const void *value,
                       size_t value_len,
                       uint8_t *out,
                       size_t out_max,
                       size_t *out_len)
{
    uint8_t val[96];
    uint8_t vb[160];
    uint8_t vbl[168];
    uint8_t pdu[220];
    uint8_t msg[280];
    ber_out_t o;
    snmp_message_t dummy;

    (void)dummy;
    if (community == NULL || oid == NULL || out == NULL || out_len == NULL) {
        return SNMP_ERR_PARSE;
    }

    o.cur = val;
    o.end = val + sizeof(val);
    if (value_tag == SNMP_TAG_NULL || value == NULL) {
        if (ber_write_null(&o) != SNMP_OK) {
            return SNMP_ERR_TOO_BIG;
        }
    } else if (value_tag == SNMP_TAG_INTEGER) {
        int32_t v;
        memcpy(&v, value, sizeof(v));
        if (ber_write_int(&o, v) != SNMP_OK) {
            return SNMP_ERR_TOO_BIG;
        }
    } else if (value_tag == SNMP_TAG_OCTET_STR) {
        if (ber_write_octets(&o, (const char *)value, value_len) != SNMP_OK) {
            return SNMP_ERR_TOO_BIG;
        }
    } else {
        return SNMP_ERR_PARSE;
    }

    {
        ber_out_t vbo;
        vbo.cur = vb;
        vbo.end = vb + sizeof(vb);
        if (write_varbind(&vbo, oid, oid_len, val, (size_t)(o.cur - val)) != SNMP_OK) {
            return SNMP_ERR_TOO_BIG;
        }

        {
            ber_out_t list;
            list.cur = vbl;
            list.end = vbl + sizeof(vbl);
            if (ber_write_header(&list, BER_TAG_SEQUENCE, vb, (size_t)(vbo.cur - vb)) != SNMP_OK) {
                return SNMP_ERR_TOO_BIG;
            }

            {
                ber_out_t p;
                p.cur = pdu;
                p.end = pdu + sizeof(pdu);
                if (ber_write_int(&p, request_id) != SNMP_OK ||
                    ber_write_int(&p, 0) != SNMP_OK ||
                    ber_write_int(&p, 0) != SNMP_OK ||
                    ber_write_bytes(&p, vbl, (size_t)(list.cur - vbl)) != SNMP_OK) {
                    return SNMP_ERR_TOO_BIG;
                }

                {
                    ber_out_t m;
                    m.cur = msg;
                    m.end = msg + sizeof(msg);
                    if (ber_write_int(&m, 1) != SNMP_OK ||
                        ber_write_octets(&m, community, strlen(community)) != SNMP_OK ||
                        ber_write_header(&m, pdu_tag, pdu, (size_t)(p.cur - pdu)) != SNMP_OK) {
                        return SNMP_ERR_TOO_BIG;
                    }

                    {
                        ber_out_t top;
                        top.cur = out;
                        top.end = out + out_max;
                        if (ber_write_header(&top, BER_TAG_SEQUENCE, msg, (size_t)(m.cur - msg)) != SNMP_OK) {
                            return SNMP_ERR_TOO_BIG;
                        }
                        *out_len = (size_t)(top.cur - out);
                        return SNMP_OK;
                    }
                }
            }
        }
    }
}

#include "frame.h" /* заголовок "frame.h" */
#include "snmp.h" /* заголовок "snmp.h" */

#include <stdio.h> /* заголовок <stdio.h> */
#include <string.h> /* заголовок <string.h> */

static int g_led; /* состояние светодиода в тестах */
static uint32_t g_uptime = 1234; /* поддельный sysUpTime */
static int g_failures; /* счётчик проваленных проверок */

static void check(int cond, const char *name) /* напечатать PASS или FAIL */
{ /* начало блока */
    if (!cond) { /* если (!cond) */
        printf("FAIL  %s\n", name); /* проверка не прошла */
        g_failures++; /* счётчик проваленных проверок */
    } else { /* шаг теста */
        printf("PASS  %s\n", name); /* проверка прошла */
    } /* конец блока */
} /* конец блока */

static uint32_t get_uptime(void) /* колбэк uptime для агента */
{ /* начало блока */
    return g_uptime; /* вернуть g_uptime */
} /* конец блока */

static int get_led(void) /* колбэк чтения светодиода */
{ /* начало блока */
    return g_led; /* вернуть g_led */
} /* конец блока */

static int set_led(int on) /* колбэк записи светодиода */
{ /* начало блока */
    g_led = on ? 1 : 0; /* задать g_led */
    return 0; /* вернуть 0 */
} /* конец блока */

static void make_agent(snmp_agent_t *agent, snmp_agent_config_t *cfg) /* собрать тестового агента */
{ /* начало блока */
    memset(cfg, 0, sizeof(*cfg)); /* шаг теста */
    cfg->sys_descr = "unit-test-agent"; /* задать cfg->sys_descr */
    cfg->sys_contact = "qa@example"; /* задать cfg->sys_contact */
    cfg->sys_name = "testhost"; /* задать cfg->sys_name */
    cfg->sys_location = "lab"; /* задать cfg->sys_location */
    cfg->sys_services = 72; /* задать cfg->sys_services */
    cfg->community_read = "public"; /* задать cfg->community_read */
    cfg->community_write = "private"; /* задать cfg->community_write */
    cfg->get_uptime_cs = get_uptime; /* задать cfg->get_uptime_cs */
    cfg->get_led = get_led; /* задать cfg->get_led */
    cfg->set_led = set_led; /* задать cfg->set_led */
    check(snmp_agent_init(agent, cfg) == SNMP_OK, "agent_init"); /* проверка условия теста */
} /* конец блока */

static int query(snmp_agent_t *agent, /* построить запрос, обработать, разобрать ответ */
                 uint8_t pdu, /* шаг теста */
                 const char *community, /* шаг теста */
                 const char *oid_text, /* шаг теста */
                 uint8_t value_tag, /* шаг теста */
                 const void *value, /* шаг теста */
                 size_t value_len, /* шаг теста */
                 snmp_message_t *resp) /* вызов библиотеки SNMP */
{ /* начало блока */
    uint32_t oid[SNMP_OID_MAX_ARCS]; /* шаг теста */
    size_t oid_len = 0; /* задать size_t oid_len */
    uint8_t req[256]; /* шаг теста */
    uint8_t out[256]; /* шаг теста */
    size_t req_len = 0; /* задать size_t req_len */
    size_t out_len = 0; /* задать size_t out_len */

    if (snmp_oid_from_text(oid_text, oid, &oid_len) != SNMP_OK) { /* если (snmp_oid_from_text(oid_text, oid, &oid_len) != SNMP_OK) */
        return -1; /* вернуть -1 */
    } /* конец блока */
    if (snmp_build_request(pdu, 42, community, oid, oid_len, value_tag, value, value_len, /* если (snmp_build_request(pdu, 42, community, oid, oid_len, value_tag, value, value_len, */
                           req, sizeof(req), &req_len) != SNMP_OK) { /* задать req, sizeof(req), &req_len) ! */
        return -1; /* вернуть -1 */
    } /* конец блока */
    if (snmp_agent_process(agent, req, req_len, out, sizeof(out), &out_len) != SNMP_OK) { /* если (snmp_agent_process(agent, req, req_len, out, sizeof(out), &out_len) != SNMP_OK) */
        return -1; /* вернуть -1 */
    } /* конец блока */
    return snmp_parse_message(out, out_len, resp); /* вернуть snmp_parse_message(out, out_len, resp) */
} /* конец блока */

static void test_oid_parse(void) /* разбор текстового OID */
{ /* начало блока */
    uint32_t arcs[SNMP_OID_MAX_ARCS]; /* шаг теста */
    size_t n = 0; /* задать size_t n */

    check(snmp_oid_from_text("1.3.6.1.2.1.1.1.0", arcs, &n) == SNMP_OK, "oid_parse_sysdescr"); /* проверка условия теста */
    check(n == 9 && arcs[0] == 1 && arcs[8] == 0, "oid_parse_values"); /* проверка условия теста */
    check(snmp_oid_from_text("1", arcs, &n) == SNMP_ERR_PARSE, "oid_parse_too_short"); /* проверка условия теста */
    check(snmp_oid_from_text("1.3.x", arcs, &n) == SNMP_ERR_PARSE, "oid_parse_bad_char"); /* проверка условия теста */
} /* конец блока */

static void test_get_sysdescr(snmp_agent_t *agent) /* GET sysDescr */
{ /* начало блока */
    snmp_message_t resp; /* вызов библиотеки SNMP */

    check(query(agent, SNMP_PDU_GET, "public", "1.3.6.1.2.1.1.1.0", /* проверка условия теста */
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK, /* задать SNMP_TAG_NULL, NULL, 0, &resp) */
          "get_sysdescr_process"); /* шаг теста */
    check(resp.pdu_tag == SNMP_PDU_RESPONSE, "get_sysdescr_pdu"); /* проверка условия теста */
    check(resp.error_status == SNMP_ERRSTAT_NOERROR, "get_sysdescr_status"); /* проверка условия теста */
    check(resp.request_id == 42, "get_sysdescr_id"); /* проверка условия теста */
    check(resp.value_tag == SNMP_TAG_OCTET_STR, "get_sysdescr_tag"); /* проверка условия теста */
    check(strcmp(resp.str_value, "unit-test-agent") == 0, "get_sysdescr_value"); /* проверка условия теста */
} /* конец блока */

static void test_get_uptime(snmp_agent_t *agent) /* GET sysUpTime */
{ /* начало блока */
    snmp_message_t resp; /* вызов библиотеки SNMP */

    g_uptime = 0x123456; /* задать g_uptime */
    check(query(agent, SNMP_PDU_GET, "public", "1.3.6.1.2.1.1.3.0", /* проверка условия теста */
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK, /* задать SNMP_TAG_NULL, NULL, 0, &resp) */
          "get_uptime_process"); /* шаг теста */
    check(resp.value_tag == SNMP_TAG_TIMETICKS, "get_uptime_tag"); /* проверка условия теста */
    check(resp.int_value == (int32_t)0x123456, "get_uptime_value"); /* проверка условия теста */
} /* конец блока */

static void test_getnext_walk(snmp_agent_t *agent) /* обход GETNEXT */
{ /* начало блока */
    snmp_message_t resp; /* вызов библиотеки SNMP */

    check(query(agent, SNMP_PDU_GETNEXT, "public", "1.3.6.1.2.1.1", /* проверка условия теста */
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK, /* задать SNMP_TAG_NULL, NULL, 0, &resp) */
          "getnext_system_process"); /* шаг теста */
    check(resp.oid_len == 9 && resp.oid[7] == 1 && resp.oid[8] == 0, "getnext_to_sysdescr"); /* проверка условия теста */

    check(query(agent, SNMP_PDU_GETNEXT, "public", "1.3.6.1.2.1.1.7.0", /* проверка условия теста */
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK, /* задать SNMP_TAG_NULL, NULL, 0, &resp) */
          "getnext_after_sysservices"); /* шаг теста */
    check(resp.oid_len == 10 && resp.oid[6] == 99999, "getnext_to_led"); /* проверка условия теста */

    check(query(agent, SNMP_PDU_GETNEXT, "public", "1.3.6.1.4.1.99999.1.1.0", /* проверка условия теста */
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK, /* задать SNMP_TAG_NULL, NULL, 0, &resp) */
          "getnext_end_process"); /* шаг теста */
    check(resp.value_tag == SNMP_EXC_ENDOFMIB, "getnext_end_of_mib"); /* проверка условия теста */
} /* конец блока */

static void test_unknown_oid(snmp_agent_t *agent) /* неизвестный OID */
{ /* начало блока */
    snmp_message_t resp; /* вызов библиотеки SNMP */

    check(query(agent, SNMP_PDU_GET, "public", "1.3.6.1.2.1.1.99.0", /* проверка условия теста */
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK, /* задать SNMP_TAG_NULL, NULL, 0, &resp) */
          "get_unknown_process"); /* шаг теста */
    check(resp.value_tag == SNMP_EXC_NOSUCHOBJ, "get_unknown_exc"); /* проверка условия теста */
} /* конец блока */

static void test_set_led(snmp_agent_t *agent) /* SET ledState */
{ /* начало блока */
    snmp_message_t resp; /* вызов библиотеки SNMP */
    int32_t on = 1; /* задать int32_t on */

    g_led = 0; /* задать g_led */
    check(query(agent, SNMP_PDU_SET, "private", "1.3.6.1.4.1.99999.1.1.0", /* проверка условия теста */
                SNMP_TAG_INTEGER, &on, sizeof(on), &resp) == SNMP_OK, /* задать SNMP_TAG_INTEGER, &on, sizeof(on), &resp) */
          "set_led_process"); /* шаг теста */
    check(resp.error_status == SNMP_ERRSTAT_NOERROR, "set_led_status"); /* проверка условия теста */
    check(g_led == 1, "set_led_applied"); /* проверка условия теста */
    check(resp.int_value == 1, "set_led_echo"); /* проверка условия теста */
} /* конец блока */

static void test_set_readonly(snmp_agent_t *agent) /* SET только для чтения */
{ /* начало блока */
    snmp_message_t resp; /* вызов библиотеки SNMP */
    int32_t value = 1; /* задать int32_t value */

    check(query(agent, SNMP_PDU_SET, "private", "1.3.6.1.2.1.1.7.0", /* проверка условия теста */
                SNMP_TAG_INTEGER, &value, sizeof(value), &resp) == SNMP_OK, /* задать SNMP_TAG_INTEGER, &value, sizeof(value), &resp) */
          "set_readonly_process"); /* шаг теста */
    check(resp.error_status == SNMP_ERRSTAT_NOTWRITABLE, "set_readonly_status"); /* проверка условия теста */
} /* конец блока */

static void test_bad_community(snmp_agent_t *agent) /* неверная community */
{ /* начало блока */
    snmp_message_t resp; /* вызов библиотеки SNMP */

    check(query(agent, SNMP_PDU_GET, "wrong", "1.3.6.1.2.1.1.1.0", /* проверка условия теста */
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK, /* задать SNMP_TAG_NULL, NULL, 0, &resp) */
          "bad_community_process"); /* шаг теста */
    check(resp.error_status == SNMP_ERRSTAT_AUTH, "bad_community_status"); /* проверка условия теста */
} /* конец блока */

static void test_set_requires_private(snmp_agent_t *agent) /* SET с public запрещён */
{ /* начало блока */
    snmp_message_t resp; /* вызов библиотеки SNMP */
    int32_t on = 1; /* задать int32_t on */

    check(query(agent, SNMP_PDU_SET, "public", "1.3.6.1.4.1.99999.1.1.0", /* проверка условия теста */
                SNMP_TAG_INTEGER, &on, sizeof(on), &resp) == SNMP_OK, /* задать SNMP_TAG_INTEGER, &on, sizeof(on), &resp) */
          "set_public_process"); /* шаг теста */
    check(resp.error_status == SNMP_ERRSTAT_AUTH, "set_public_denied"); /* проверка условия теста */
} /* конец блока */

static void test_sysname_and_location(snmp_agent_t *agent) /* GET sysName и sysLocation */
{ /* начало блока */
    snmp_message_t resp; /* вызов библиотеки SNMP */

    check(query(agent, SNMP_PDU_GET, "public", "1.3.6.1.2.1.1.5.0", /* проверка условия теста */
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK, /* задать SNMP_TAG_NULL, NULL, 0, &resp) */
          "get_sysname_process"); /* шаг теста */
    check(strcmp(resp.str_value, "testhost") == 0, "get_sysname_value"); /* проверка условия теста */

    check(query(agent, SNMP_PDU_GET, "public", "1.3.6.1.2.1.1.6.0", /* проверка условия теста */
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK, /* задать SNMP_TAG_NULL, NULL, 0, &resp) */
          "get_syslocation_process"); /* шаг теста */
    check(strcmp(resp.str_value, "lab") == 0, "get_syslocation_value"); /* проверка условия теста */
} /* конец блока */

static void test_frame_roundtrip(void) /* кадр туда и обратно */
{ /* начало блока */
    const uint8_t payload[] = {0x30, 0x03, 0x02, 0x01, 0x01}; /* задать const uint8_t payload[] */
    uint8_t wire[64]; /* шаг теста */
    size_t wire_len = 0; /* задать size_t wire_len */
    frame_parser_t parser; /* вызов библиотеки SNMP */
    size_t i; /* шаг теста */
    int complete = 0; /* задать int complete */

    check(frame_encode(payload, sizeof(payload), wire, sizeof(wire), &wire_len) == 0, "frame_encode"); /* проверка условия теста */
    check(wire_len == sizeof(payload) + FRAME_OVERHEAD, "frame_len"); /* проверка условия теста */
    check(wire[0] == FRAME_SYNC0 && wire[1] == FRAME_SYNC1, "frame_sync"); /* проверка условия теста */

    frame_parser_init(&parser); /* вызов библиотеки SNMP */
    for (i = 0; i < wire_len; i++) { /* пройти байты кадра */
        if (frame_parser_feed(&parser, wire[i])) { /* если (frame_parser_feed(&parser, wire[i])) */
            complete = 1; /* задать complete */
        } /* конец блока */
    } /* конец блока */
    check(complete == 1, "frame_parse_complete"); /* проверка условия теста */
    check(parser.length == sizeof(payload), "frame_parse_len"); /* проверка условия теста */
    check(memcmp(parser.payload, payload, sizeof(payload)) == 0, "frame_parse_payload"); /* проверка условия теста */
} /* конец блока */

static void test_frame_bad_checksum(void) /* кадр с плохой суммой */
{ /* начало блока */
    const uint8_t payload[] = {1, 2, 3, 4}; /* задать const uint8_t payload[] */
    uint8_t wire[32]; /* шаг теста */
    size_t wire_len = 0; /* задать size_t wire_len */
    frame_parser_t parser; /* вызов библиотеки SNMP */
    size_t i; /* шаг теста */
    int complete = 0; /* задать int complete */

    check(frame_encode(payload, sizeof(payload), wire, sizeof(wire), &wire_len) == 0, "frame_bad_cksum_encode"); /* проверка условия теста */
    wire[wire_len - 1] ^= 0xFF; /* задать wire[wire_len - 1] ^ */

    frame_parser_init(&parser); /* вызов библиотеки SNMP */
    for (i = 0; i < wire_len; i++) { /* пройти байты кадра */
        if (frame_parser_feed(&parser, wire[i])) { /* если (frame_parser_feed(&parser, wire[i])) */
            complete = 1; /* задать complete */
        } /* конец блока */
    } /* конец блока */
    check(complete == 0, "frame_bad_cksum_rejected"); /* проверка условия теста */
} /* конец блока */

static void test_garbage_request(snmp_agent_t *agent) /* мусор вместо BER */
{ /* начало блока */
    uint8_t junk[] = {0x00, 0x01, 0x02, 0x03}; /* задать uint8_t junk[] */
    uint8_t out[64]; /* шаг теста */
    size_t out_len = 0; /* задать size_t out_len */

    check(snmp_agent_process(agent, junk, sizeof(junk), out, sizeof(out), &out_len) == SNMP_ERR_PARSE, /* проверка условия теста */
          "garbage_rejected"); /* шаг теста */
} /* конец блока */

int main(void) /* запустить все проверки */
{ /* начало блока */
    snmp_agent_t agent; /* вызов библиотеки SNMP */
    snmp_agent_config_t cfg; /* вызов библиотеки SNMP */

    make_agent(&agent, &cfg); /* подготовить агента */
    test_oid_parse(); /* выполнить набор проверок */
    test_get_sysdescr(&agent); /* выполнить набор проверок */
    test_get_uptime(&agent); /* выполнить набор проверок */
    test_getnext_walk(&agent); /* выполнить набор проверок */
    test_unknown_oid(&agent); /* выполнить набор проверок */
    test_set_led(&agent); /* выполнить набор проверок */
    test_set_readonly(&agent); /* выполнить набор проверок */
    test_bad_community(&agent); /* выполнить набор проверок */
    test_set_requires_private(&agent); /* выполнить набор проверок */
    test_sysname_and_location(&agent); /* выполнить набор проверок */
    test_frame_roundtrip(); /* выполнить набор проверок */
    test_frame_bad_checksum(); /* выполнить набор проверок */
    test_garbage_request(&agent); /* выполнить набор проверок */

    if (g_failures != 0) { /* счётчик проваленных проверок */
        printf("\n%d test(s) failed\n", g_failures); /* счётчик проваленных проверок */
        return 1; /* вернуть 1 */
    } /* конец блока */
    printf("\nAll tests passed\n"); /* все проверки успешны */
    return 0; /* вернуть 0 */
} /* конец блока */

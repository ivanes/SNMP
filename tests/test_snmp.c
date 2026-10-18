#include "frame.h"
#include "snmp.h"

#include <stdio.h>
#include <string.h>

static int g_led;
static uint32_t g_uptime = 1234;
static int g_failures;

static void check(int cond, const char *name)
{
    if (!cond) {
        printf("FAIL  %s\n", name);
        g_failures++;
    } else {
        printf("PASS  %s\n", name);
    }
}

static uint32_t get_uptime(void)
{
    return g_uptime;
}

static int get_led(void)
{
    return g_led;
}

static int set_led(int on)
{
    g_led = on ? 1 : 0;
    return 0;
}

static void make_agent(snmp_agent_t *agent, snmp_agent_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->sys_descr = "unit-test-agent";
    cfg->sys_contact = "qa@example";
    cfg->sys_name = "testhost";
    cfg->sys_location = "lab";
    cfg->sys_services = 72;
    cfg->community_read = "public";
    cfg->community_write = "private";
    cfg->get_uptime_cs = get_uptime;
    cfg->get_led = get_led;
    cfg->set_led = set_led;
    check(snmp_agent_init(agent, cfg) == SNMP_OK, "agent_init");
}

static int query(snmp_agent_t *agent,
                 uint8_t pdu,
                 const char *community,
                 const char *oid_text,
                 uint8_t value_tag,
                 const void *value,
                 size_t value_len,
                 snmp_message_t *resp)
{
    uint32_t oid[SNMP_OID_MAX_ARCS];
    size_t oid_len = 0;
    uint8_t req[256];
    uint8_t out[256];
    size_t req_len = 0;
    size_t out_len = 0;

    if (snmp_oid_from_text(oid_text, oid, &oid_len) != SNMP_OK) {
        return -1;
    }
    if (snmp_build_request(pdu, 42, community, oid, oid_len, value_tag, value, value_len,
                           req, sizeof(req), &req_len) != SNMP_OK) {
        return -1;
    }
    if (snmp_agent_process(agent, req, req_len, out, sizeof(out), &out_len) != SNMP_OK) {
        return -1;
    }
    return snmp_parse_message(out, out_len, resp);
}

static void test_oid_parse(void)
{
    uint32_t arcs[SNMP_OID_MAX_ARCS];
    size_t n = 0;

    check(snmp_oid_from_text("1.3.6.1.2.1.1.1.0", arcs, &n) == SNMP_OK, "oid_parse_sysdescr");
    check(n == 9 && arcs[0] == 1 && arcs[8] == 0, "oid_parse_values");
    check(snmp_oid_from_text("1", arcs, &n) == SNMP_ERR_PARSE, "oid_parse_too_short");
    check(snmp_oid_from_text("1.3.x", arcs, &n) == SNMP_ERR_PARSE, "oid_parse_bad_char");
}

static void test_get_sysdescr(snmp_agent_t *agent)
{
    snmp_message_t resp;

    check(query(agent, SNMP_PDU_GET, "public", "1.3.6.1.2.1.1.1.0",
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK,
          "get_sysdescr_process");
    check(resp.pdu_tag == SNMP_PDU_RESPONSE, "get_sysdescr_pdu");
    check(resp.error_status == SNMP_ERRSTAT_NOERROR, "get_sysdescr_status");
    check(resp.request_id == 42, "get_sysdescr_id");
    check(resp.value_tag == SNMP_TAG_OCTET_STR, "get_sysdescr_tag");
    check(strcmp(resp.str_value, "unit-test-agent") == 0, "get_sysdescr_value");
}

static void test_get_uptime(snmp_agent_t *agent)
{
    snmp_message_t resp;

    g_uptime = 0x123456;
    check(query(agent, SNMP_PDU_GET, "public", "1.3.6.1.2.1.1.3.0",
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK,
          "get_uptime_process");
    check(resp.value_tag == SNMP_TAG_TIMETICKS, "get_uptime_tag");
    check(resp.int_value == (int32_t)0x123456, "get_uptime_value");
}

static void test_getnext_walk(snmp_agent_t *agent)
{
    snmp_message_t resp;

    check(query(agent, SNMP_PDU_GETNEXT, "public", "1.3.6.1.2.1.1",
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK,
          "getnext_system_process");
    check(resp.oid_len == 9 && resp.oid[7] == 1 && resp.oid[8] == 0, "getnext_to_sysdescr");

    check(query(agent, SNMP_PDU_GETNEXT, "public", "1.3.6.1.2.1.1.7.0",
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK,
          "getnext_after_sysservices");
    check(resp.oid_len == 10 && resp.oid[6] == 99999, "getnext_to_led");

    check(query(agent, SNMP_PDU_GETNEXT, "public", "1.3.6.1.4.1.99999.1.1.0",
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK,
          "getnext_end_process");
    check(resp.value_tag == SNMP_EXC_ENDOFMIB, "getnext_end_of_mib");
}

static void test_unknown_oid(snmp_agent_t *agent)
{
    snmp_message_t resp;

    check(query(agent, SNMP_PDU_GET, "public", "1.3.6.1.2.1.1.99.0",
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK,
          "get_unknown_process");
    check(resp.value_tag == SNMP_EXC_NOSUCHOBJ, "get_unknown_exc");
}

static void test_set_led(snmp_agent_t *agent)
{
    snmp_message_t resp;
    int32_t on = 1;

    g_led = 0;
    check(query(agent, SNMP_PDU_SET, "private", "1.3.6.1.4.1.99999.1.1.0",
                SNMP_TAG_INTEGER, &on, sizeof(on), &resp) == SNMP_OK,
          "set_led_process");
    check(resp.error_status == SNMP_ERRSTAT_NOERROR, "set_led_status");
    check(g_led == 1, "set_led_applied");
    check(resp.int_value == 1, "set_led_echo");
}

static void test_set_readonly(snmp_agent_t *agent)
{
    snmp_message_t resp;
    int32_t value = 1;

    check(query(agent, SNMP_PDU_SET, "private", "1.3.6.1.2.1.1.7.0",
                SNMP_TAG_INTEGER, &value, sizeof(value), &resp) == SNMP_OK,
          "set_readonly_process");
    check(resp.error_status == SNMP_ERRSTAT_NOTWRITABLE, "set_readonly_status");
}

static void test_bad_community(snmp_agent_t *agent)
{
    snmp_message_t resp;

    check(query(agent, SNMP_PDU_GET, "wrong", "1.3.6.1.2.1.1.1.0",
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK,
          "bad_community_process");
    check(resp.error_status == SNMP_ERRSTAT_AUTH, "bad_community_status");
}

static void test_set_requires_private(snmp_agent_t *agent)
{
    snmp_message_t resp;
    int32_t on = 1;

    check(query(agent, SNMP_PDU_SET, "public", "1.3.6.1.4.1.99999.1.1.0",
                SNMP_TAG_INTEGER, &on, sizeof(on), &resp) == SNMP_OK,
          "set_public_process");
    check(resp.error_status == SNMP_ERRSTAT_AUTH, "set_public_denied");
}

static void test_sysname_and_location(snmp_agent_t *agent)
{
    snmp_message_t resp;

    check(query(agent, SNMP_PDU_GET, "public", "1.3.6.1.2.1.1.5.0",
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK,
          "get_sysname_process");
    check(strcmp(resp.str_value, "testhost") == 0, "get_sysname_value");

    check(query(agent, SNMP_PDU_GET, "public", "1.3.6.1.2.1.1.6.0",
                SNMP_TAG_NULL, NULL, 0, &resp) == SNMP_OK,
          "get_syslocation_process");
    check(strcmp(resp.str_value, "lab") == 0, "get_syslocation_value");
}

static void test_frame_roundtrip(void)
{
    const uint8_t payload[] = {0x30, 0x03, 0x02, 0x01, 0x01};
    uint8_t wire[64];
    size_t wire_len = 0;
    frame_parser_t parser;
    size_t i;
    int complete = 0;

    check(frame_encode(payload, sizeof(payload), wire, sizeof(wire), &wire_len) == 0, "frame_encode");
    check(wire_len == sizeof(payload) + FRAME_OVERHEAD, "frame_len");
    check(wire[0] == FRAME_SYNC0 && wire[1] == FRAME_SYNC1, "frame_sync");

    frame_parser_init(&parser);
    for (i = 0; i < wire_len; i++) {
        if (frame_parser_feed(&parser, wire[i])) {
            complete = 1;
        }
    }
    check(complete == 1, "frame_parse_complete");
    check(parser.length == sizeof(payload), "frame_parse_len");
    check(memcmp(parser.payload, payload, sizeof(payload)) == 0, "frame_parse_payload");
}

static void test_frame_bad_checksum(void)
{
    const uint8_t payload[] = {1, 2, 3, 4};
    uint8_t wire[32];
    size_t wire_len = 0;
    frame_parser_t parser;
    size_t i;
    int complete = 0;

    check(frame_encode(payload, sizeof(payload), wire, sizeof(wire), &wire_len) == 0, "frame_bad_cksum_encode");
    wire[wire_len - 1] ^= 0xFF;

    frame_parser_init(&parser);
    for (i = 0; i < wire_len; i++) {
        if (frame_parser_feed(&parser, wire[i])) {
            complete = 1;
        }
    }
    check(complete == 0, "frame_bad_cksum_rejected");
}

static void test_garbage_request(snmp_agent_t *agent)
{
    uint8_t junk[] = {0x00, 0x01, 0x02, 0x03};
    uint8_t out[64];
    size_t out_len = 0;

    check(snmp_agent_process(agent, junk, sizeof(junk), out, sizeof(out), &out_len) == SNMP_ERR_PARSE,
          "garbage_rejected");
}

int main(void)
{
    snmp_agent_t agent;
    snmp_agent_config_t cfg;

    make_agent(&agent, &cfg);
    test_oid_parse();
    test_get_sysdescr(&agent);
    test_get_uptime(&agent);
    test_getnext_walk(&agent);
    test_unknown_oid(&agent);
    test_set_led(&agent);
    test_set_readonly(&agent);
    test_bad_community(&agent);
    test_set_requires_private(&agent);
    test_sysname_and_location(&agent);
    test_frame_roundtrip();
    test_frame_bad_checksum();
    test_garbage_request(&agent);

    if (g_failures != 0) {
        printf("\n%d test(s) failed\n", g_failures);
        return 1;
    }
    printf("\nAll tests passed\n");
    return 0;
}

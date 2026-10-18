#ifndef SNMP_H
#define SNMP_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    SNMP_OK = 0,
    SNMP_ERR_PARSE = -1,
    SNMP_ERR_TOO_BIG = -2,
    SNMP_ERR_AUTH = -3,
    SNMP_ERR_INTERNAL = -4
};

#define SNMP_PDU_GET      0xA0u
#define SNMP_PDU_GETNEXT  0xA1u
#define SNMP_PDU_RESPONSE 0xA2u
#define SNMP_PDU_SET      0xA3u

#define SNMP_TAG_INTEGER   0x02u
#define SNMP_TAG_OCTET_STR 0x04u
#define SNMP_TAG_NULL      0x05u
#define SNMP_TAG_OID       0x06u
#define SNMP_TAG_TIMETICKS 0x43u
#define SNMP_EXC_NOSUCHOBJ 0x80u
#define SNMP_EXC_NOSUCHINS 0x81u
#define SNMP_EXC_ENDOFMIB  0x82u

#define SNMP_ERRSTAT_NOERROR    0
#define SNMP_ERRSTAT_TOOBIG     1
#define SNMP_ERRSTAT_NOSUCHNAME 2
#define SNMP_ERRSTAT_BADVALUE   3
#define SNMP_ERRSTAT_READONLY   4
#define SNMP_ERRSTAT_GENERR     5
#define SNMP_ERRSTAT_AUTH       16
#define SNMP_ERRSTAT_NOTWRITABLE 17

#define SNMP_OID_MAX_ARCS 16
#define SNMP_STR_MAX      96
#define SNMP_COMMUNITY_MAX 32

typedef uint32_t (*snmp_uptime_fn)(void);
typedef int (*snmp_led_get_fn)(void);
typedef int (*snmp_led_set_fn)(int on);

typedef struct {
    const char *sys_descr;
    const char *sys_contact;
    const char *sys_name;
    const char *sys_location;
    int sys_services;
    const char *community_read;
    const char *community_write;
    snmp_uptime_fn get_uptime_cs;
    snmp_led_get_fn get_led;
    snmp_led_set_fn set_led;
} snmp_agent_config_t;

typedef struct {
    const snmp_agent_config_t *cfg;
} snmp_agent_t;

int snmp_agent_init(snmp_agent_t *agent, const snmp_agent_config_t *cfg);

int snmp_agent_process(snmp_agent_t *agent,
                       const uint8_t *request,
                       size_t request_len,
                       uint8_t *response,
                       size_t response_max,
                       size_t *response_len);

int snmp_oid_from_text(const char *text, uint32_t *arcs, size_t *n_arcs);

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
                       size_t *out_len);

typedef struct {
    int32_t version;
    char community[SNMP_COMMUNITY_MAX];
    uint8_t pdu_tag;
    int32_t request_id;
    int32_t error_status;
    int32_t error_index;
    uint32_t oid[SNMP_OID_MAX_ARCS];
    size_t oid_len;
    uint8_t value_tag;
    int32_t int_value;
    char str_value[SNMP_STR_MAX];
    size_t str_len;
} snmp_message_t;

int snmp_parse_message(const uint8_t *msg, size_t len, snmp_message_t *out);

#ifdef __cplusplus
}
#endif

#endif

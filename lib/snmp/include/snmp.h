#ifndef SNMP_H /* не включать этот файл дважды */
#define SNMP_H /* отметить, что заголовок SNMP уже открыт */

#include <stddef.h> /* типы size_t и NULL */
#include <stdint.h> /* фиксированные целые uint8_t, int32_t, uint32_t */

#ifdef __cplusplus /* если файл компилирует C++ */
extern "C" { /* не искажать имена функций */
#endif /* конец проверки языка */

enum { /* коды возврата агента */
    SNMP_OK = 0, /* запрос разобран и обработан */
    SNMP_ERR_PARSE = -1, /* битый BER или неверные поля */
    SNMP_ERR_TOO_BIG = -2, /* ответ не влез в буфер */
    SNMP_ERR_AUTH = -3, /* неверная community */
    SNMP_ERR_INTERNAL = -4 /* внутренняя ошибка агента */
}; /* конец кодов возврата */

#define SNMP_PDU_GET      0xA0u /* PDU GetRequest */
#define SNMP_PDU_GETNEXT  0xA1u /* PDU GetNextRequest */
#define SNMP_PDU_RESPONSE 0xA2u /* PDU GetResponse */
#define SNMP_PDU_SET      0xA3u /* PDU SetRequest */

#define SNMP_TAG_INTEGER   0x02u /* BER INTEGER */
#define SNMP_TAG_OCTET_STR 0x04u /* BER OCTET STRING */
#define SNMP_TAG_NULL      0x05u /* BER NULL */
#define SNMP_TAG_OID       0x06u /* BER OBJECT IDENTIFIER */
#define SNMP_TAG_TIMETICKS 0x43u /* TimeTicks (сотые доли секунды) */
#define SNMP_EXC_NOSUCHOBJ 0x80u /* исключение noSuchObject */
#define SNMP_EXC_NOSUCHINS 0x81u /* исключение noSuchInstance */
#define SNMP_EXC_ENDOFMIB  0x82u /* исключение endOfMibView */

#define SNMP_ERRSTAT_NOERROR    0 /* error-status: ошибки нет */
#define SNMP_ERRSTAT_TOOBIG     1 /* error-status: слишком большой PDU */
#define SNMP_ERRSTAT_NOSUCHNAME 2 /* error-status: нет такого имени */
#define SNMP_ERRSTAT_BADVALUE   3 /* error-status: недопустимое значение */
#define SNMP_ERRSTAT_READONLY   4 /* error-status: объект только для чтения */
#define SNMP_ERRSTAT_GENERR     5 /* error-status: общая ошибка */
#define SNMP_ERRSTAT_AUTH       16 /* error-status: ошибка авторизации */
#define SNMP_ERRSTAT_NOTWRITABLE 17 /* error-status: объект нельзя писать */

#define SNMP_OID_MAX_ARCS 16 /* максимум дуг в OID */
#define SNMP_STR_MAX      96 /* максимум символов в строковом значении */
#define SNMP_COMMUNITY_MAX 32 /* максимум символов community */

typedef uint32_t (*snmp_uptime_fn)(void); /* колбэк sysUpTime в сотых секунды */
typedef int (*snmp_led_get_fn)(void); /* колбэк чтения состояния светодиода */
typedef int (*snmp_led_set_fn)(int on); /* колбэк записи светодиода 0/1 */

typedef struct { /* настройки конкретного агента */
    const char *sys_descr; /* текстовое описание устройства */
    const char *sys_contact; /* контакт администратора */
    const char *sys_name; /* имя узла */
    const char *sys_location; /* место установки */
    int sys_services; /* битовая маска услуг OSI */
    const char *community_read; /* community для чтения, обычно public */
    const char *community_write; /* community для записи, обычно private */
    snmp_uptime_fn get_uptime_cs; /* как узнать время работы */
    snmp_led_get_fn get_led; /* как прочитать светодиод */
    snmp_led_set_fn set_led; /* как записать светодиод */
} snmp_agent_config_t; /* конец настроек агента */

typedef struct { /* рабочий экземпляр агента */
    const snmp_agent_config_t *cfg; /* указатель на настройки */
} snmp_agent_t; /* конец экземпляра */

int snmp_agent_init(snmp_agent_t *agent, const snmp_agent_config_t *cfg); /* привязать настройки к агенту */

int snmp_agent_process(snmp_agent_t *agent, /* обработать один SNMP-запрос */
                       const uint8_t *request, /* входящий BER */
                       size_t request_len, /* длина запроса */
                       uint8_t *response, /* буфер ответа */
                       size_t response_max, /* размер буфера ответа */
                       size_t *response_len); /* фактическая длина ответа */

int snmp_oid_from_text(const char *text, uint32_t *arcs, size_t *n_arcs); /* разобрать OID вида 1.3.6.1 */

int snmp_build_request(uint8_t pdu_tag, /* собрать тестовый запрос */
                       int32_t request_id, /* идентификатор запроса */
                       const char *community, /* community */
                       const uint32_t *oid, /* дуги OID */
                       size_t oid_len, /* число дуг */
                       uint8_t value_tag, /* тип значения (NULL/INTEGER/строка) */
                       const void *value, /* указатель на значение SET */
                       size_t value_len, /* длина значения */
                       uint8_t *out, /* выходной BER */
                       size_t out_max, /* размер выходного буфера */
                       size_t *out_len); /* длина собранного запроса */

typedef struct { /* разобранное SNMP-сообщение */
    int32_t version; /* 0 = v1, 1 = v2c */
    char community[SNMP_COMMUNITY_MAX]; /* community из пакета */
    uint8_t pdu_tag; /* тип PDU */
    int32_t request_id; /* request-id */
    int32_t error_status; /* error-status */
    int32_t error_index; /* error-index */
    uint32_t oid[SNMP_OID_MAX_ARCS]; /* первый OID в varbind */
    size_t oid_len; /* число дуг OID */
    uint8_t value_tag; /* тег значения */
    int32_t int_value; /* целое значение, если есть */
    char str_value[SNMP_STR_MAX]; /* строковое значение, если есть */
    size_t str_len; /* длина строки */
} snmp_message_t; /* конец разобранного сообщения */

int snmp_parse_message(const uint8_t *msg, size_t len, snmp_message_t *out); /* разобрать BER в структуру */

#ifdef __cplusplus /* снова проверка C++ */
} /* закрыть extern "C" */
#endif /* конец проверки языка */

#endif /* конец охранного include */

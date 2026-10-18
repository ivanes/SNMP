#include "app_tasks.h" /* прототип app_tasks_start */

#include "board.h" /* BOARD_NAME */
#include "bsp.h" /* светодиод и UART */
#include "frame.h" /* упаковка ответа в кадр */
#include "snmp.h" /* агент SNMPv2c */

#include "FreeRTOS.h" /* типы FreeRTOS */
#include "task.h" /* xTaskCreate, задержки, тики */

static volatile int g_led_override = -1; /* -1: мигать; 0/1: светодиод зафиксирован SET */

static uint32_t app_uptime_cs(void) /* sysUpTime в сотых секунды */
{ /* начало колбэка uptime */
    return (uint32_t)(xTaskGetTickCount() / 10u); /* 1000 Гц → разделить на 10 */
} /* конец колбэка uptime */

static int app_led_get(void) /* SNMP GET ledState */
{ /* начало чтения светодиода */
    return bsp_led_get(); /* вернуть логическое состояние */
} /* конец чтения светодиода */

static int app_led_set(int on) /* SNMP SET ledState */
{ /* начало записи светодиода */
    g_led_override = on ? 1 : 0; /* больше не мигать автоматически */
    bsp_led_set(on); /* сразу применить к пину */
    return 0; /* успех для агента */
} /* конец записи светодиода */

static void led_task(void *arg) /* задача-индикатор «система жива» */
{ /* начало led_task */
    (void)arg; /* аргумент не используем */
    for (;;) { /* вечный цикл задачи */
        if (g_led_override < 0) { /* если SET ещё не приходил */
            bsp_led_toggle(); /* моргнуть */
        } /* конец проверки режима */
        vTaskDelay(pdMS_TO_TICKS(500)); /* пауза 500 мс */
    } /* конец вечного цикла */
} /* конец led_task */

static void snmp_task(void *arg) /* задача обработки SNMP-кадров */
{ /* начало snmp_task */
    static uint8_t request[FRAME_MAX_PAYLOAD]; /* принятый BER */
    static uint8_t response[FRAME_MAX_PAYLOAD]; /* собранный BER-ответ */
    static uint8_t framed[FRAME_MAX_PAYLOAD + FRAME_OVERHEAD]; /* ответ в кадре UART */
    snmp_agent_t agent; /* экземпляр агента */
    snmp_agent_config_t cfg; /* его настройки */
    size_t req_len; /* длина принятого BER */
    size_t resp_len; /* длина ответа */
    size_t frame_len; /* длина кадра на провод */

    (void)arg; /* аргумент не используем */
    cfg.sys_descr = "STM32F411 FreeRTOS SNMPv2c agent"; /* sysDescr */
    cfg.sys_contact = ""; /* sysContact пустой */
    cfg.sys_name = "snmp-f411"; /* sysName */
    cfg.sys_location = BOARD_NAME; /* плата как место установки */
    cfg.sys_services = 72; /* типичная маска applications+end-to-end */
    cfg.community_read = "public"; /* чтение */
    cfg.community_write = "private"; /* запись */
    cfg.get_uptime_cs = app_uptime_cs; /* откуда брать uptime */
    cfg.get_led = app_led_get; /* откуда читать светодиод */
    cfg.set_led = app_led_set; /* куда писать светодиод */
    (void)snmp_agent_init(&agent, &cfg); /* привязать настройки */

    for (;;) { /* ждать кадры и отвечать */
        if (!bsp_uart_recv_frame(request, sizeof(request), &req_len, 0xFFFFFFFFu)) { /* кадр не пришёл */
            continue; /* ждать снова */
        } /* конец проверки приёма */
        if (snmp_agent_process(&agent, request, req_len, response, sizeof(response), &resp_len) != SNMP_OK) { /* агент не смог ответить */
            continue; /* молча отбросить битый запрос */
        } /* конец обработки PDU */
        if (frame_encode(response, resp_len, framed, sizeof(framed), &frame_len) == 0) { /* ответ упакован */
            bsp_uart_write(framed, frame_len); /* отдать кадр в UART */
        } /* конец отправки */
    } /* конец цикла задачи */
} /* конец snmp_task */

void app_tasks_start(void) /* создать обе прикладные задачи */
{ /* начало app_tasks_start */
    BaseType_t ok; /* результат xTaskCreate */

    ok = xTaskCreate(led_task, "led", 128, NULL, 1, NULL); /* мигание, низкий приоритет */
    configASSERT(ok == pdPASS); /* задача led должна создаться */
    ok = xTaskCreate(snmp_task, "snmp", 512, NULL, 3, NULL); /* SNMP важнее светодиода */
    configASSERT(ok == pdPASS); /* задача snmp должна создаться */
} /* конец app_tasks_start */

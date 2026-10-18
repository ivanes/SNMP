#include "app_tasks.h"

#include "board.h"
#include "bsp.h"
#include "frame.h"
#include "snmp.h"

#include "FreeRTOS.h"
#include "task.h"

static volatile int g_led_override = -1;

static uint32_t app_uptime_cs(void)
{
    return (uint32_t)(xTaskGetTickCount() / 10u);
}

static int app_led_get(void)
{
    return bsp_led_get();
}

static int app_led_set(int on)
{
    g_led_override = on ? 1 : 0;
    bsp_led_set(on);
    return 0;
}

static void led_task(void *arg)
{
    (void)arg;
    for (;;) {
        if (g_led_override < 0) {
            bsp_led_toggle();
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void snmp_task(void *arg)
{
    static uint8_t request[FRAME_MAX_PAYLOAD];
    static uint8_t response[FRAME_MAX_PAYLOAD];
    static uint8_t framed[FRAME_MAX_PAYLOAD + FRAME_OVERHEAD];
    snmp_agent_t agent;
    snmp_agent_config_t cfg;
    size_t req_len;
    size_t resp_len;
    size_t frame_len;

    (void)arg;
    cfg.sys_descr = "STM32F411 FreeRTOS SNMPv2c agent";
    cfg.sys_contact = "";
    cfg.sys_name = "snmp-f411";
    cfg.sys_location = BOARD_NAME;
    cfg.sys_services = 72;
    cfg.community_read = "public";
    cfg.community_write = "private";
    cfg.get_uptime_cs = app_uptime_cs;
    cfg.get_led = app_led_get;
    cfg.set_led = app_led_set;
    (void)snmp_agent_init(&agent, &cfg);

    for (;;) {
        if (!bsp_uart_recv_frame(request, sizeof(request), &req_len, 0xFFFFFFFFu)) {
            continue;
        }
        if (snmp_agent_process(&agent, request, req_len, response, sizeof(response), &resp_len) != SNMP_OK) {
            continue;
        }
        if (frame_encode(response, resp_len, framed, sizeof(framed), &frame_len) == 0) {
            bsp_uart_write(framed, frame_len);
        }
    }
}

void app_tasks_start(void)
{
    BaseType_t ok;

    ok = xTaskCreate(led_task, "led", 128, NULL, 1, NULL);
    configASSERT(ok == pdPASS);
    ok = xTaskCreate(snmp_task, "snmp", 512, NULL, 3, NULL);
    configASSERT(ok == pdPASS);
}

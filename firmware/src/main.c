#include "app_tasks.h" /* создание прикладных задач */
#include "board.h" /* имя платы и пины */
#include "bsp.h" /* инициализация железа */

#include "FreeRTOS.h" /* типы и макросы ядра */
#include "task.h" /* vTaskStartScheduler */

int main(void) /* точка входа после Reset_Handler */
{ /* начало main */
    bsp_init(); /* настроить LED, UART и очередь кадров */
    bsp_uart_write_str("\r\nSNMP-F411 FreeRTOS\r\n"); /* баннер прошивки */
    bsp_uart_write_str(BOARD_NAME "\r\n"); /* какая плата собрана */
    bsp_uart_write_str("USART 115200 8N1, frame A5 5A len payload cksum\r\n"); /* формат линии */
    bsp_uart_write_str("SNMPv2c communities: public / private\r\n"); /* community */

    app_tasks_start(); /* создать задачи led и snmp */
    vTaskStartScheduler(); /* отдать управление FreeRTOS */

    for (;;) { /* сюда попадём только если планировщик не стартовал */
    } /* крутиться бесконечно */
} /* конец main */

void vApplicationStackOverflowHook(TaskHandle_t task, char *name) /* стек задачи исчерпан */
{ /* начало хука переполнения */
    (void)task; /* хэндл не используем */
    (void)name; /* имя не используем */
    taskDISABLE_INTERRUPTS(); /* запретить прерывания */
    for (;;) { /* безопасный стоп */
    } /* конец стопа */
} /* конец хука переполнения */

void vApplicationMallocFailedHook(void) /* куча FreeRTOS кончилась */
{ /* начало хука malloc */
    taskDISABLE_INTERRUPTS(); /* запретить прерывания */
    for (;;) { /* безопасный стоп */
    } /* конец стопа */
} /* конец хука malloc */

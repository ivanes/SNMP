#include "app_tasks.h"
#include "board.h"
#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

int main(void)
{
    bsp_init();
    bsp_uart_write_str("\r\nSNMP-F411 FreeRTOS\r\n");
    bsp_uart_write_str(BOARD_NAME "\r\n");
    bsp_uart_write_str("USART 115200 8N1, frame A5 5A len payload cksum\r\n");
    bsp_uart_write_str("SNMPv2c communities: public / private\r\n");

    app_tasks_start();
    vTaskStartScheduler();

    for (;;) {
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

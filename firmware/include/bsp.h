#ifndef BSP_H
#define BSP_H

#include <stddef.h>
#include <stdint.h>

void bsp_init(void);
void bsp_led_set(int on);
int bsp_led_get(void);
void bsp_led_toggle(void);
void bsp_uart_write(const uint8_t *data, size_t len);
void bsp_uart_write_str(const char *s);
int bsp_uart_recv_frame(uint8_t *dst, size_t max_len, size_t *len, uint32_t timeout_ms);

#endif

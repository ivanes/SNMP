#ifndef BSP_H /* не включать BSP дважды */
#define BSP_H /* отметить, что BSP уже открыт */

#include <stddef.h> /* size_t */
#include <stdint.h> /* uint8_t, uint32_t */

void bsp_init(void); /* запустить GPIO, UART, очередь кадров */
void bsp_led_set(int on); /* зажечь (1) или погасить (0) светодиод */
int bsp_led_get(void); /* вернуть логическое состояние светодиода */
void bsp_led_toggle(void); /* инвертировать светодиод */
void bsp_uart_write(const uint8_t *data, size_t len); /* отправить сырые байты в UART */
void bsp_uart_write_str(const char *s); /* отправить C-строку в UART */
int bsp_uart_recv_frame(uint8_t *dst, size_t max_len, size_t *len, uint32_t timeout_ms); /* ждать кадр из очереди */

#endif /* конец охранного include */

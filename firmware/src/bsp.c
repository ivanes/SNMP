#include "bsp.h" /* публичные функции платы */

#include "board.h" /* пины LED и UART */
#include "frame.h" /* автомат кадра A5 5A */

#include "FreeRTOS.h" /* configASSERT */
#include "queue.h" /* очередь принятых кадров */

#include <string.h> /* memcpy */

typedef struct { /* один принятый кадр для очереди */
    uint16_t length; /* длина нагрузки */
    uint8_t data[FRAME_MAX_PAYLOAD]; /* байты нагрузки */
} uart_frame_t; /* конец элемента очереди */

static QueueHandle_t g_frame_queue; /* очередь из ISR в задачу SNMP */
static frame_parser_t g_parser; /* побайтный разборщик */
static volatile int g_led_on; /* логическое состояние светодиода */

static void gpio_set_af(GPIO_TypeDef *gpio, uint32_t pin, uint32_t af) /* назначить альтернативную функцию пину */
{ /* начало gpio_set_af */
    uint32_t shift = (pin & 7u) * 4u; /* сдвиг внутри AFR[0] или AFR[1] */
    uint32_t afr_index = pin >> 3u; /* 0 для пинов 0..7, 1 для 8..15 */
    gpio->AFR[afr_index] &= ~(0xFu << shift); /* стереть старую AF */
    gpio->AFR[afr_index] |= (af & 0xFu) << shift; /* записать новую AF */
} /* конец gpio_set_af */

static void led_init(void) /* настроить пин светодиода как выход */
{ /* начало led_init */
    BOARD_LED_CLK_ENABLE(); /* включить тактирование порта LED */
    (void)BOARD_LED_GPIO->MODER; /* барьер после включения тактирования */
    BOARD_LED_GPIO->MODER &= ~(3u << (BOARD_LED_PIN * 2u)); /* очистить режим пина */
    BOARD_LED_GPIO->MODER |= (1u << (BOARD_LED_PIN * 2u)); /* режим выхода */
    BOARD_LED_GPIO->OTYPER &= ~(1u << BOARD_LED_PIN); /* push-pull */
    BOARD_LED_GPIO->OSPEEDR |= (3u << (BOARD_LED_PIN * 2u)); /* высокая скорость */
    BOARD_LED_GPIO->PUPDR &= ~(3u << (BOARD_LED_PIN * 2u)); /* без подтяжки */
    bsp_led_set(0); /* погасить при старте */
} /* конец led_init */

static void uart_init(void) /* настроить UART 115200 8N1 */
{ /* начало uart_init */
    GPIO_TypeDef *gpio = BOARD_UART_GPIO; /* порт TX/RX */

    BOARD_UART_GPIO_CLK_ENABLE(); /* тактирование GPIO UART */
    BOARD_UART_CLK_ENABLE(); /* тактирование самого USART */
    (void)gpio->MODER; /* барьер после включения тактирования */

    gpio->MODER &= ~((3u << (BOARD_UART_TX_PIN * 2u)) | (3u << (BOARD_UART_RX_PIN * 2u))); /* очистить режимы TX/RX */
    gpio->MODER |= (2u << (BOARD_UART_TX_PIN * 2u)) | (2u << (BOARD_UART_RX_PIN * 2u)); /* альтернативная функция */
    gpio->OTYPER &= ~((1u << BOARD_UART_TX_PIN) | (1u << BOARD_UART_RX_PIN)); /* push-pull */
    gpio->OSPEEDR |= (3u << (BOARD_UART_TX_PIN * 2u)) | (3u << (BOARD_UART_RX_PIN * 2u)); /* высокая скорость */
    gpio->PUPDR &= ~((3u << (BOARD_UART_TX_PIN * 2u)) | (3u << (BOARD_UART_RX_PIN * 2u))); /* убрать старые подтяжки */
    gpio->PUPDR |= (1u << (BOARD_UART_RX_PIN * 2u)); /* подтянуть RX к питанию */
    gpio_set_af(gpio, BOARD_UART_TX_PIN, BOARD_UART_AF); /* TX = USART AF7 */
    gpio_set_af(gpio, BOARD_UART_RX_PIN, BOARD_UART_AF); /* RX = USART AF7 */

    BOARD_UART->CR1 = 0; /* выключить USART перед настройкой */
    BOARD_UART->BRR = BOARD_UART_PCLK_HZ / BOARD_UART_BAUD; /* делитель бод */
    BOARD_UART->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; /* TX, RX, включить */
} /* конец uart_init */

void bsp_init(void) /* полная инициализация платы */
{ /* начало bsp_init */
    frame_parser_init(&g_parser); /* автомат кадра в исходное состояние */
    g_frame_queue = xQueueCreate(2, sizeof(uart_frame_t)); /* два слота под кадры */
    configASSERT(g_frame_queue != NULL); /* очередь обязана существовать */

    led_init(); /* светодиод */
    uart_init(); /* UART */

    NVIC_SetPriority(BOARD_UART_IRQn, 6); /* приоритет безопасный для FromISR */
    NVIC_EnableIRQ(BOARD_UART_IRQn); /* разрешить IRQ UART */
    BOARD_UART->CR1 |= USART_CR1_RXNEIE; /* прерывание по приёму байта */
} /* конец bsp_init */

void bsp_led_set(int on) /* записать логическое состояние светодиода */
{ /* начало bsp_led_set */
    g_led_on = on ? 1 : 0; /* запомнить логику 0/1 */
    if (BOARD_LED_ACTIVE_LOW) { /* Black Pill: горит нулём */
        if (g_led_on) { /* нужно зажечь */
            BOARD_LED_GPIO->BSRR = (1u << BOARD_LED_PIN) << 16; /* сбросить пин */
        } else { /* нужно погасить */
            BOARD_LED_GPIO->BSRR = (1u << BOARD_LED_PIN); /* установить пин */
        } /* конец active-low */
    } else { /* Nucleo: горит единицей */
        if (g_led_on) { /* нужно зажечь */
            BOARD_LED_GPIO->BSRR = (1u << BOARD_LED_PIN); /* установить пин */
        } else { /* нужно погасить */
            BOARD_LED_GPIO->BSRR = (1u << BOARD_LED_PIN) << 16; /* сбросить пин */
        } /* конец active-high */
    } /* конец выбора полярности */
} /* конец bsp_led_set */

int bsp_led_get(void) /* прочитать логическое состояние */
{ /* начало bsp_led_get */
    return g_led_on; /* 0 или 1 */
} /* конец bsp_led_get */

void bsp_led_toggle(void) /* инвертировать светодиод */
{ /* начало bsp_led_toggle */
    bsp_led_set(!g_led_on); /* записать противоположное значение */
} /* конец bsp_led_toggle */

void bsp_uart_write(const uint8_t *data, size_t len) /* отправить буфер в UART */
{ /* начало bsp_uart_write */
    size_t i; /* индекс байта */
    for (i = 0; i < len; i++) { /* по каждому байту */
        while ((BOARD_UART->SR & USART_SR_TXE) == 0u) { /* ждать пустой DR */
        } /* конец ожидания TXE */
        BOARD_UART->DR = data[i]; /* записать байт */
    } /* конец цикла */
} /* конец bsp_uart_write */

void bsp_uart_write_str(const char *s) /* отправить строку */
{ /* начало bsp_uart_write_str */
    bsp_uart_write((const uint8_t *)s, strlen(s)); /* длина по нуль-терминатору */
} /* конец bsp_uart_write_str */

int bsp_uart_recv_frame(uint8_t *dst, size_t max_len, size_t *len, uint32_t timeout_ms) /* взять кадр из очереди */
{ /* начало bsp_uart_recv_frame */
    uart_frame_t frame; /* локальная копия кадра */
    TickType_t ticks = (timeout_ms == 0xFFFFFFFFu) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms); /* бесконечно или мс */

    if (xQueueReceive(g_frame_queue, &frame, ticks) != pdTRUE) { /* очередь пуста по таймауту */
        return 0; /* кадра нет */
    } /* конец ожидания очереди */
    if (frame.length > max_len) { /* не влезает в буфер вызывающего */
        return 0; /* отбросить */
    } /* конец проверки размера */
    memcpy(dst, frame.data, frame.length); /* скопировать нагрузку */
    *len = frame.length; /* отдать длину */
    return 1; /* кадр готов */
} /* конец bsp_uart_recv_frame */

void BOARD_UART_IRQHandler(void) /* IRQ приёма UART */
{ /* начало IRQ */
    uint32_t sr = BOARD_UART->SR; /* флаги статуса */
    BaseType_t woken = pdFALSE; /* нужно ли переключить задачу */

    if ((sr & USART_SR_RXNE) != 0u) { /* пришёл байт */
        uint8_t byte = (uint8_t)BOARD_UART->DR; /* прочитать и снять RXNE */
        if (frame_parser_feed(&g_parser, byte) != 0) { /* собран целый кадр */
            uart_frame_t frame; /* элемент для очереди */
            frame.length = g_parser.length; /* длина нагрузки */
            memcpy(frame.data, g_parser.payload, frame.length); /* скопировать данные */
            (void)xQueueSendFromISR(g_frame_queue, &frame, &woken); /* отдать задаче SNMP */
        } /* конец готового кадра */
    } /* конец RXNE */

    if ((sr & (USART_SR_ORE | USART_SR_FE | USART_SR_NE)) != 0u) { /* overrun / framing / noise */
        (void)BOARD_UART->DR; /* прочитать DR, чтобы снять ошибку */
    } /* конец сброса ошибок */

    portYIELD_FROM_ISR(woken); /* если SNMP ждала кадр — переключиться */
} /* конец IRQ */

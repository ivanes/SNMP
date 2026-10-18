#include "bsp.h"

#include "board.h"
#include "frame.h"

#include "FreeRTOS.h"
#include "queue.h"

#include <string.h>

typedef struct {
    uint16_t length;
    uint8_t data[FRAME_MAX_PAYLOAD];
} uart_frame_t;

static QueueHandle_t g_frame_queue;
static frame_parser_t g_parser;
static volatile int g_led_on;

static void gpio_set_af(GPIO_TypeDef *gpio, uint32_t pin, uint32_t af)
{
    uint32_t shift = (pin & 7u) * 4u;
    uint32_t afr_index = pin >> 3u;
    gpio->AFR[afr_index] &= ~(0xFu << shift);
    gpio->AFR[afr_index] |= (af & 0xFu) << shift;
}

static void led_init(void)
{
    BOARD_LED_CLK_ENABLE();
    (void)BOARD_LED_GPIO->MODER;
    BOARD_LED_GPIO->MODER &= ~(3u << (BOARD_LED_PIN * 2u));
    BOARD_LED_GPIO->MODER |= (1u << (BOARD_LED_PIN * 2u));
    BOARD_LED_GPIO->OTYPER &= ~(1u << BOARD_LED_PIN);
    BOARD_LED_GPIO->OSPEEDR |= (3u << (BOARD_LED_PIN * 2u));
    BOARD_LED_GPIO->PUPDR &= ~(3u << (BOARD_LED_PIN * 2u));
    bsp_led_set(0);
}

static void uart_init(void)
{
    GPIO_TypeDef *gpio = BOARD_UART_GPIO;

    BOARD_UART_GPIO_CLK_ENABLE();
    BOARD_UART_CLK_ENABLE();
    (void)gpio->MODER;

    gpio->MODER &= ~((3u << (BOARD_UART_TX_PIN * 2u)) | (3u << (BOARD_UART_RX_PIN * 2u)));
    gpio->MODER |= (2u << (BOARD_UART_TX_PIN * 2u)) | (2u << (BOARD_UART_RX_PIN * 2u));
    gpio->OTYPER &= ~((1u << BOARD_UART_TX_PIN) | (1u << BOARD_UART_RX_PIN));
    gpio->OSPEEDR |= (3u << (BOARD_UART_TX_PIN * 2u)) | (3u << (BOARD_UART_RX_PIN * 2u));
    gpio->PUPDR &= ~((3u << (BOARD_UART_TX_PIN * 2u)) | (3u << (BOARD_UART_RX_PIN * 2u)));
    gpio->PUPDR |= (1u << (BOARD_UART_RX_PIN * 2u));
    gpio_set_af(gpio, BOARD_UART_TX_PIN, BOARD_UART_AF);
    gpio_set_af(gpio, BOARD_UART_RX_PIN, BOARD_UART_AF);

    BOARD_UART->CR1 = 0;
    BOARD_UART->BRR = BOARD_UART_PCLK_HZ / BOARD_UART_BAUD;
    BOARD_UART->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void bsp_init(void)
{
    frame_parser_init(&g_parser);
    g_frame_queue = xQueueCreate(2, sizeof(uart_frame_t));
    configASSERT(g_frame_queue != NULL);

    led_init();
    uart_init();

    NVIC_SetPriority(BOARD_UART_IRQn, 6);
    NVIC_EnableIRQ(BOARD_UART_IRQn);
    BOARD_UART->CR1 |= USART_CR1_RXNEIE;
}

void bsp_led_set(int on)
{
    g_led_on = on ? 1 : 0;
    if (BOARD_LED_ACTIVE_LOW) {
        if (g_led_on) {
            BOARD_LED_GPIO->BSRR = (1u << BOARD_LED_PIN) << 16;
        } else {
            BOARD_LED_GPIO->BSRR = (1u << BOARD_LED_PIN);
        }
    } else {
        if (g_led_on) {
            BOARD_LED_GPIO->BSRR = (1u << BOARD_LED_PIN);
        } else {
            BOARD_LED_GPIO->BSRR = (1u << BOARD_LED_PIN) << 16;
        }
    }
}

int bsp_led_get(void)
{
    return g_led_on;
}

void bsp_led_toggle(void)
{
    bsp_led_set(!g_led_on);
}

void bsp_uart_write(const uint8_t *data, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++) {
        while ((BOARD_UART->SR & USART_SR_TXE) == 0u) {
        }
        BOARD_UART->DR = data[i];
    }
}

void bsp_uart_write_str(const char *s)
{
    bsp_uart_write((const uint8_t *)s, strlen(s));
}

int bsp_uart_recv_frame(uint8_t *dst, size_t max_len, size_t *len, uint32_t timeout_ms)
{
    uart_frame_t frame;
    TickType_t ticks = (timeout_ms == 0xFFFFFFFFu) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

    if (xQueueReceive(g_frame_queue, &frame, ticks) != pdTRUE) {
        return 0;
    }
    if (frame.length > max_len) {
        return 0;
    }
    memcpy(dst, frame.data, frame.length);
    *len = frame.length;
    return 1;
}

void BOARD_UART_IRQHandler(void)
{
    uint32_t sr = BOARD_UART->SR;
    BaseType_t woken = pdFALSE;

    if ((sr & USART_SR_RXNE) != 0u) {
        uint8_t byte = (uint8_t)BOARD_UART->DR;
        if (frame_parser_feed(&g_parser, byte) != 0) {
            uart_frame_t frame;
            frame.length = g_parser.length;
            memcpy(frame.data, g_parser.payload, frame.length);
            (void)xQueueSendFromISR(g_frame_queue, &frame, &woken);
        }
    }

    if ((sr & (USART_SR_ORE | USART_SR_FE | USART_SR_NE)) != 0u) {
        (void)BOARD_UART->DR;
    }

    portYIELD_FROM_ISR(woken);
}

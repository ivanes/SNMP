#ifndef BOARD_H
#define BOARD_H

#include "stm32f4xx.h"

#if defined(BOARD_BLACKPILL)
#define BOARD_NAME              "WeAct Black Pill STM32F411CE"
#define BOARD_LED_GPIO          GPIOC
#define BOARD_LED_PIN           13u
#define BOARD_LED_ACTIVE_LOW    1
#define BOARD_LED_CLK_ENABLE()  (RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN)
#define BOARD_UART              USART1
#define BOARD_UART_IRQn         USART1_IRQn
#define BOARD_UART_IRQHandler   USART1_IRQHandler
#define BOARD_UART_CLK_ENABLE() (RCC->APB2ENR |= RCC_APB2ENR_USART1EN)
#define BOARD_UART_GPIO         GPIOA
#define BOARD_UART_GPIO_CLK_ENABLE() (RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN)
#define BOARD_UART_TX_PIN       9u
#define BOARD_UART_RX_PIN       10u
#define BOARD_UART_AF           7u
#define BOARD_UART_PCLK_HZ      100000000u
#else
#define BOARD_NAME              "NUCLEO-F411RE"
#define BOARD_LED_GPIO          GPIOA
#define BOARD_LED_PIN           5u
#define BOARD_LED_ACTIVE_LOW    0
#define BOARD_LED_CLK_ENABLE()  (RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN)
#define BOARD_UART              USART2
#define BOARD_UART_IRQn         USART2_IRQn
#define BOARD_UART_IRQHandler   USART2_IRQHandler
#define BOARD_UART_CLK_ENABLE() (RCC->APB1ENR |= RCC_APB1ENR_USART2EN)
#define BOARD_UART_GPIO         GPIOA
#define BOARD_UART_GPIO_CLK_ENABLE() (RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN)
#define BOARD_UART_TX_PIN       2u
#define BOARD_UART_RX_PIN       3u
#define BOARD_UART_AF           7u
#define BOARD_UART_PCLK_HZ      50000000u
#endif

#define BOARD_UART_BAUD         115200u
#define BOARD_SYSCLK_HZ         100000000u

#endif

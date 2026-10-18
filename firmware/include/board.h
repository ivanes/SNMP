#ifndef BOARD_H /* не включать описание платы дважды */
#define BOARD_H /* отметить, что описание платы открыто */

#include "stm32f4xx.h" /* регистры STM32F411 */

#if defined(BOARD_BLACKPILL) /* сборка под WeAct Black Pill */
#define BOARD_NAME              "WeAct Black Pill STM32F411CE" /* строка для sysLocation */
#define BOARD_LED_GPIO          GPIOC /* светодиод на порте C */
#define BOARD_LED_PIN           13u /* вывод PC13 */
#define BOARD_LED_ACTIVE_LOW    1 /* на этой плате светодиод горит нулём */
#define BOARD_LED_CLK_ENABLE()  (RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN) /* включить тактирование GPIOC */
#define BOARD_UART              USART1 /* консоль на USART1 */
#define BOARD_UART_IRQn         USART1_IRQn /* номер прерывания USART1 */
#define BOARD_UART_IRQHandler   USART1_IRQHandler /* имя обработчика USART1 */
#define BOARD_UART_CLK_ENABLE() (RCC->APB2ENR |= RCC_APB2ENR_USART1EN) /* включить тактирование USART1 */
#define BOARD_UART_GPIO         GPIOA /* TX/RX на порте A */
#define BOARD_UART_GPIO_CLK_ENABLE() (RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN) /* включить тактирование GPIOA */
#define BOARD_UART_TX_PIN       9u /* PA9 = TX */
#define BOARD_UART_RX_PIN       10u /* PA10 = RX */
#define BOARD_UART_AF           7u /* альтернативная функция USART */
#define BOARD_UART_PCLK_HZ      100000000u /* USART1 сидит на APB2 = 100 МГц */
#else /* иначе плата Nucleo-F411RE */
#define BOARD_NAME              "NUCLEO-F411RE" /* строка для sysLocation */
#define BOARD_LED_GPIO          GPIOA /* светодиод LD2 на порте A */
#define BOARD_LED_PIN           5u /* вывод PA5 */
#define BOARD_LED_ACTIVE_LOW    0 /* LD2 горит единицей */
#define BOARD_LED_CLK_ENABLE()  (RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN) /* включить тактирование GPIOA */
#define BOARD_UART              USART2 /* VCP ST-LINK на USART2 */
#define BOARD_UART_IRQn         USART2_IRQn /* номер прерывания USART2 */
#define BOARD_UART_IRQHandler   USART2_IRQHandler /* имя обработчика USART2 */
#define BOARD_UART_CLK_ENABLE() (RCC->APB1ENR |= RCC_APB1ENR_USART2EN) /* включить тактирование USART2 */
#define BOARD_UART_GPIO         GPIOA /* TX/RX на порте A */
#define BOARD_UART_GPIO_CLK_ENABLE() (RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN) /* включить тактирование GPIOA */
#define BOARD_UART_TX_PIN       2u /* PA2 = TX */
#define BOARD_UART_RX_PIN       3u /* PA3 = RX */
#define BOARD_UART_AF           7u /* альтернативная функция USART */
#define BOARD_UART_PCLK_HZ      50000000u /* USART2 сидит на APB1 = 50 МГц */
#endif /* конец выбора платы */

#define BOARD_UART_BAUD         115200u /* скорость консоли */
#define BOARD_SYSCLK_HZ         100000000u /* частота ядра после PLL */

#endif /* конец охранного include */

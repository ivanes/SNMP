#include "board.h" /* BOARD_SYSCLK_HZ и регистры */

uint32_t SystemCoreClock = 16000000u; /* после сброса ядро на HSI 16 МГц */

const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9}; /* сдвиги AHB, требует CMSIS */
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4}; /* сдвиги APB, требует CMSIS */

static void clock_init(void) /* разогнать ядро до 100 МГц от HSI */
{ /* начало clock_init */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN; /* включить тактирование PWR */
    (void)RCC->APB1ENR; /* дождаться, пока шина оживёт */
    PWR->CR |= PWR_CR_VOS; /* масштаб напряжения 1 для 100 МГц */

    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_3WS; /* кэш и 3 wait state */

    RCC->PLLCFGR = (16u << RCC_PLLCFGR_PLLM_Pos) | /* HSI 16 МГц / 16 = 1 МГц */
                   (200u << RCC_PLLCFGR_PLLN_Pos) | /* VCO = 200 МГц */
                   (0u << RCC_PLLCFGR_PLLP_Pos) | /* /2 → SYSCLK 100 МГц */
                   RCC_PLLCFGR_PLLSRC_HSI | /* источник PLL — HSI */
                   (4u << RCC_PLLCFGR_PLLQ_Pos); /* PLLQ = 50 МГц, USB не используем */

    RCC->CR |= RCC_CR_PLLON; /* запустить PLL */
    while ((RCC->CR & RCC_CR_PLLRDY) == 0u) { /* ждать готовности */
    } /* конец ожидания PLL */

    RCC->CFGR = RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1 | RCC_CFGR_SW_PLL; /* AHB 100, APB1 50, APB2 100 */
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) { /* ждать переключения */
    } /* конец ожидания SWS */

    SystemCoreClock = BOARD_SYSCLK_HZ; /* сообщить FreeRTOS новую частоту */
} /* конец clock_init */

void SystemInit(void) /* вызывается из Reset_Handler до main */
{ /* начало SystemInit */
    SCB->CPACR |= (3UL << 20) | (3UL << 22); /* разрешить FPU CP10/CP11 */
    __DSB(); /* барьер данных */
    __ISB(); /* барьер инструкций */

    RCC->CR |= RCC_CR_HSION; /* включить HSI */
    while ((RCC->CR & RCC_CR_HSIRDY) == 0u) { /* ждать HSI */
    } /* конец ожидания HSI */

    RCC->CFGR = 0; /* переключиться обратно на HSI */
    RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_CSSON | RCC_CR_HSEON); /* выключить PLL/HSE */
    RCC->PLLCFGR = 0x24003010u; /* сброс PLLCFGR в значение по умолчанию */
    RCC->CIR = 0; /* выключить прерывания RCC */

    clock_init(); /* выставить 100 МГц */
    NVIC_SetPriorityGrouping(3); /* 4 бита preempt, 0 subpriority */
} /* конец SystemInit */

void SystemCoreClockUpdate(void) /* CMSIS требует эту функцию */
{ /* начало SystemCoreClockUpdate */
    SystemCoreClock = BOARD_SYSCLK_HZ; /* мы всегда на фиксированных 100 МГц */
} /* конец SystemCoreClockUpdate */

#include "board.h"

uint32_t SystemCoreClock = 16000000u;

const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};

static void clock_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;
    PWR->CR |= PWR_CR_VOS;

    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_3WS;

    /* HSI 16 MHz / 16 * 200 / 2 = 100 MHz */
    RCC->PLLCFGR = (16u << RCC_PLLCFGR_PLLM_Pos) |
                   (200u << RCC_PLLCFGR_PLLN_Pos) |
                   (0u << RCC_PLLCFGR_PLLP_Pos) |
                   RCC_PLLCFGR_PLLSRC_HSI |
                   (4u << RCC_PLLCFGR_PLLQ_Pos);

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0u) {
    }

    RCC->CFGR = RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1 | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
    }

    SystemCoreClock = BOARD_SYSCLK_HZ;
}

void SystemInit(void)
{
    SCB->CPACR |= (3UL << 20) | (3UL << 22);
    __DSB();
    __ISB();

    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0u) {
    }

    RCC->CFGR = 0;
    RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_CSSON | RCC_CR_HSEON);
    RCC->PLLCFGR = 0x24003010u;
    RCC->CIR = 0;

    clock_init();
    NVIC_SetPriorityGrouping(3);
}

void SystemCoreClockUpdate(void)
{
    SystemCoreClock = BOARD_SYSCLK_HZ;
}

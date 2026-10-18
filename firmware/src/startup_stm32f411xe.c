#include <stdint.h> /* типы uint32_t */

extern uint32_t _sidata[]; /* копия .data во флеш */
extern uint32_t _sdata[]; /* начало .data в RAM */
extern uint32_t _edata[]; /* конец .data в RAM */
extern uint32_t _sbss[]; /* начало BSS */
extern uint32_t _ebss[]; /* конец BSS */
extern uint32_t _estack[]; /* вершина стека */

extern int main(void); /* вход в приложение */
extern void SystemInit(void); /* тактирование и FPU */
extern void vPortSVCHandler(void); /* SVC FreeRTOS */
extern void xPortPendSVHandler(void); /* переключение задач FreeRTOS */
extern void xPortSysTickHandler(void); /* тик FreeRTOS */

void Reset_Handler(void); /* обработчик сброса */
void Default_Handler(void); /* заглушка неиспользуемого IRQ */

void NMI_Handler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый NMI_Handler, по умолчанию висит в Default_Handler */
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый HardFault_Handler, по умолчанию висит в Default_Handler */
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый MemManage_Handler, по умолчанию висит в Default_Handler */
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый BusFault_Handler, по умолчанию висит в Default_Handler */
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый UsageFault_Handler, по умолчанию висит в Default_Handler */
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DebugMon_Handler, по умолчанию висит в Default_Handler */
void WWDG_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый WWDG_IRQHandler, по умолчанию висит в Default_Handler */
void PVD_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый PVD_IRQHandler, по умолчанию висит в Default_Handler */
void TAMP_STAMP_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый TAMP_STAMP_IRQHandler, по умолчанию висит в Default_Handler */
void RTC_WKUP_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый RTC_WKUP_IRQHandler, по умолчанию висит в Default_Handler */
void FLASH_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый FLASH_IRQHandler, по умолчанию висит в Default_Handler */
void RCC_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый RCC_IRQHandler, по умолчанию висит в Default_Handler */
void EXTI0_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый EXTI0_IRQHandler, по умолчанию висит в Default_Handler */
void EXTI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый EXTI1_IRQHandler, по умолчанию висит в Default_Handler */
void EXTI2_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый EXTI2_IRQHandler, по умолчанию висит в Default_Handler */
void EXTI3_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый EXTI3_IRQHandler, по умолчанию висит в Default_Handler */
void EXTI4_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый EXTI4_IRQHandler, по умолчанию висит в Default_Handler */
void DMA1_Stream0_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA1_Stream0_IRQHandler, по умолчанию висит в Default_Handler */
void DMA1_Stream1_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA1_Stream1_IRQHandler, по умолчанию висит в Default_Handler */
void DMA1_Stream2_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA1_Stream2_IRQHandler, по умолчанию висит в Default_Handler */
void DMA1_Stream3_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA1_Stream3_IRQHandler, по умолчанию висит в Default_Handler */
void DMA1_Stream4_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA1_Stream4_IRQHandler, по умолчанию висит в Default_Handler */
void DMA1_Stream5_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA1_Stream5_IRQHandler, по умолчанию висит в Default_Handler */
void DMA1_Stream6_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA1_Stream6_IRQHandler, по умолчанию висит в Default_Handler */
void ADC_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый ADC_IRQHandler, по умолчанию висит в Default_Handler */
void EXTI9_5_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый EXTI9_5_IRQHandler, по умолчанию висит в Default_Handler */
void TIM1_BRK_TIM9_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый TIM1_BRK_TIM9_IRQHandler, по умолчанию висит в Default_Handler */
void TIM1_UP_TIM10_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый TIM1_UP_TIM10_IRQHandler, по умолчанию висит в Default_Handler */
void TIM1_TRG_COM_TIM11_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый TIM1_TRG_COM_TIM11_IRQHandler, по умолчанию висит в Default_Handler */
void TIM1_CC_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый TIM1_CC_IRQHandler, по умолчанию висит в Default_Handler */
void TIM2_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый TIM2_IRQHandler, по умолчанию висит в Default_Handler */
void TIM3_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый TIM3_IRQHandler, по умолчанию висит в Default_Handler */
void TIM4_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый TIM4_IRQHandler, по умолчанию висит в Default_Handler */
void I2C1_EV_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый I2C1_EV_IRQHandler, по умолчанию висит в Default_Handler */
void I2C1_ER_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый I2C1_ER_IRQHandler, по умолчанию висит в Default_Handler */
void I2C2_EV_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый I2C2_EV_IRQHandler, по умолчанию висит в Default_Handler */
void I2C2_ER_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый I2C2_ER_IRQHandler, по умолчанию висит в Default_Handler */
void SPI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый SPI1_IRQHandler, по умолчанию висит в Default_Handler */
void SPI2_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый SPI2_IRQHandler, по умолчанию висит в Default_Handler */
void USART1_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый USART1_IRQHandler, по умолчанию висит в Default_Handler */
void USART2_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый USART2_IRQHandler, по умолчанию висит в Default_Handler */
void EXTI15_10_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый EXTI15_10_IRQHandler, по умолчанию висит в Default_Handler */
void RTC_Alarm_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый RTC_Alarm_IRQHandler, по умолчанию висит в Default_Handler */
void OTG_FS_WKUP_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый OTG_FS_WKUP_IRQHandler, по умолчанию висит в Default_Handler */
void DMA1_Stream7_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA1_Stream7_IRQHandler, по умолчанию висит в Default_Handler */
void SDIO_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый SDIO_IRQHandler, по умолчанию висит в Default_Handler */
void TIM5_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый TIM5_IRQHandler, по умолчанию висит в Default_Handler */
void SPI3_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый SPI3_IRQHandler, по умолчанию висит в Default_Handler */
void DMA2_Stream0_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA2_Stream0_IRQHandler, по умолчанию висит в Default_Handler */
void DMA2_Stream1_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA2_Stream1_IRQHandler, по умолчанию висит в Default_Handler */
void DMA2_Stream2_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA2_Stream2_IRQHandler, по умолчанию висит в Default_Handler */
void DMA2_Stream3_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA2_Stream3_IRQHandler, по умолчанию висит в Default_Handler */
void DMA2_Stream4_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA2_Stream4_IRQHandler, по умолчанию висит в Default_Handler */
void OTG_FS_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый OTG_FS_IRQHandler, по умолчанию висит в Default_Handler */
void DMA2_Stream5_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA2_Stream5_IRQHandler, по умолчанию висит в Default_Handler */
void DMA2_Stream6_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA2_Stream6_IRQHandler, по умолчанию висит в Default_Handler */
void DMA2_Stream7_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый DMA2_Stream7_IRQHandler, по умолчанию висит в Default_Handler */
void USART6_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый USART6_IRQHandler, по умолчанию висит в Default_Handler */
void I2C3_EV_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый I2C3_EV_IRQHandler, по умолчанию висит в Default_Handler */
void I2C3_ER_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый I2C3_ER_IRQHandler, по умолчанию висит в Default_Handler */
void FPU_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый FPU_IRQHandler, по умолчанию висит в Default_Handler */
void SPI4_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый SPI4_IRQHandler, по умолчанию висит в Default_Handler */
void SPI5_IRQHandler(void) __attribute__((weak, alias("Default_Handler"))); /* слабый SPI5_IRQHandler, по умолчанию висит в Default_Handler */

__attribute__((section(".isr_vector"), used)) /* положить таблицу в секцию .isr_vector */
const void *const g_pfnVectors[] = { /* таблица векторов STM32F411 */
    (void *)_estack, /* начальный SP */
    Reset_Handler, /* Reset */
    NMI_Handler, /* NMI_Handler */
    HardFault_Handler, /* HardFault_Handler */
    MemManage_Handler, /* MemManage_Handler */
    BusFault_Handler, /* BusFault_Handler */
    UsageFault_Handler, /* UsageFault_Handler */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    vPortSVCHandler, /* SVC */
    DebugMon_Handler, /* DebugMon_Handler */
    0, /* зарезервированный вектор */
    xPortPendSVHandler, /* PendSV */
    xPortSysTickHandler, /* SysTick */
    WWDG_IRQHandler, /* WWDG_IRQHandler */
    PVD_IRQHandler, /* PVD_IRQHandler */
    TAMP_STAMP_IRQHandler, /* TAMP_STAMP_IRQHandler */
    RTC_WKUP_IRQHandler, /* RTC_WKUP_IRQHandler */
    FLASH_IRQHandler, /* FLASH_IRQHandler */
    RCC_IRQHandler, /* RCC_IRQHandler */
    EXTI0_IRQHandler, /* EXTI0_IRQHandler */
    EXTI1_IRQHandler, /* EXTI1_IRQHandler */
    EXTI2_IRQHandler, /* EXTI2_IRQHandler */
    EXTI3_IRQHandler, /* EXTI3_IRQHandler */
    EXTI4_IRQHandler, /* EXTI4_IRQHandler */
    DMA1_Stream0_IRQHandler, /* DMA1_Stream0_IRQHandler */
    DMA1_Stream1_IRQHandler, /* DMA1_Stream1_IRQHandler */
    DMA1_Stream2_IRQHandler, /* DMA1_Stream2_IRQHandler */
    DMA1_Stream3_IRQHandler, /* DMA1_Stream3_IRQHandler */
    DMA1_Stream4_IRQHandler, /* DMA1_Stream4_IRQHandler */
    DMA1_Stream5_IRQHandler, /* DMA1_Stream5_IRQHandler */
    DMA1_Stream6_IRQHandler, /* DMA1_Stream6_IRQHandler */
    ADC_IRQHandler, /* ADC_IRQHandler */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    EXTI9_5_IRQHandler, /* EXTI9_5_IRQHandler */
    TIM1_BRK_TIM9_IRQHandler, /* TIM1_BRK_TIM9_IRQHandler */
    TIM1_UP_TIM10_IRQHandler, /* TIM1_UP_TIM10_IRQHandler */
    TIM1_TRG_COM_TIM11_IRQHandler, /* TIM1_TRG_COM_TIM11_IRQHandler */
    TIM1_CC_IRQHandler, /* TIM1_CC_IRQHandler */
    TIM2_IRQHandler, /* TIM2_IRQHandler */
    TIM3_IRQHandler, /* TIM3_IRQHandler */
    TIM4_IRQHandler, /* TIM4_IRQHandler */
    I2C1_EV_IRQHandler, /* I2C1_EV_IRQHandler */
    I2C1_ER_IRQHandler, /* I2C1_ER_IRQHandler */
    I2C2_EV_IRQHandler, /* I2C2_EV_IRQHandler */
    I2C2_ER_IRQHandler, /* I2C2_ER_IRQHandler */
    SPI1_IRQHandler, /* SPI1_IRQHandler */
    SPI2_IRQHandler, /* SPI2_IRQHandler */
    USART1_IRQHandler, /* USART1_IRQHandler */
    USART2_IRQHandler, /* USART2_IRQHandler */
    0, /* зарезервированный вектор */
    EXTI15_10_IRQHandler, /* EXTI15_10_IRQHandler */
    RTC_Alarm_IRQHandler, /* RTC_Alarm_IRQHandler */
    OTG_FS_WKUP_IRQHandler, /* OTG_FS_WKUP_IRQHandler */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    DMA1_Stream7_IRQHandler, /* DMA1_Stream7_IRQHandler */
    0, /* зарезервированный вектор */
    SDIO_IRQHandler, /* SDIO_IRQHandler */
    TIM5_IRQHandler, /* TIM5_IRQHandler */
    SPI3_IRQHandler, /* SPI3_IRQHandler */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    DMA2_Stream0_IRQHandler, /* DMA2_Stream0_IRQHandler */
    DMA2_Stream1_IRQHandler, /* DMA2_Stream1_IRQHandler */
    DMA2_Stream2_IRQHandler, /* DMA2_Stream2_IRQHandler */
    DMA2_Stream3_IRQHandler, /* DMA2_Stream3_IRQHandler */
    DMA2_Stream4_IRQHandler, /* DMA2_Stream4_IRQHandler */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    OTG_FS_IRQHandler, /* OTG_FS_IRQHandler */
    DMA2_Stream5_IRQHandler, /* DMA2_Stream5_IRQHandler */
    DMA2_Stream6_IRQHandler, /* DMA2_Stream6_IRQHandler */
    DMA2_Stream7_IRQHandler, /* DMA2_Stream7_IRQHandler */
    USART6_IRQHandler, /* USART6_IRQHandler */
    I2C3_EV_IRQHandler, /* I2C3_EV_IRQHandler */
    I2C3_ER_IRQHandler, /* I2C3_ER_IRQHandler */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    FPU_IRQHandler, /* FPU_IRQHandler */
    0, /* зарезервированный вектор */
    0, /* зарезервированный вектор */
    SPI4_IRQHandler, /* SPI4_IRQHandler */
    SPI5_IRQHandler, /* SPI5_IRQHandler */
}; /* конец таблицы векторов */

void Reset_Handler(void) /* обработчик сброса */
{ /* начало блока */
    uint32_t *src = _sidata; /* откуда копировать .data */
    uint32_t *dst = _sdata; /* куда копировать .data */

    while (dst < _edata) { /* копировать слова .data */
        *dst++ = *src++; /* скопировать одно слово */
    } /* конец функции */
    dst = _sbss; /* перейти к BSS */
    while (dst < _ebss) { /* обнулить BSS */
        *dst++ = 0; /* записать ноль */
    } /* конец функции */

    SystemInit(); /* включить FPU и PLL */
    (void)main(); /* запустить программу */
    for (;;) { /* если main вернулся — крутиться */
    } /* конец функции */
} /* конец функции */

void Default_Handler(void) /* заглушка неиспользуемого IRQ */
{ /* начало блока */
    for (;;) { /* если main вернулся — крутиться */
    } /* конец функции */
} /* конец функции */

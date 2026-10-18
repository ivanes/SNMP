#ifndef FREERTOS_CONFIG_H /* не включать конфиг FreeRTOS дважды */
#define FREERTOS_CONFIG_H /* отметить, что конфиг уже открыт */

#include <stdint.h> /* uint16_t и size_t */

extern uint32_t SystemCoreClock; /* текущая частота ядра из system_stm32f4xx.c */

#define configUSE_PREEMPTION                       1 /* вытесняющая многозадачность */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION    1 /* выбирать задачу через CLZ */
#define configUSE_IDLE_HOOK                        0 /* idle-хук не нужен */
#define configUSE_TICK_HOOK                        0 /* tick-хук не нужен */
#define configCPU_CLOCK_HZ                         (SystemCoreClock) /* тактовая для SysTick */
#define configTICK_RATE_HZ                         ((TickType_t)1000) /* 1 тик = 1 мс */
#define configMAX_PRIORITIES                       5 /* пять уровней приоритета */
#define configMINIMAL_STACK_SIZE                   ((uint16_t)128) /* стек idle в словах */
#define configTOTAL_HEAP_SIZE                      ((size_t)(24 * 1024)) /* куча heap_4: 24 КБ */
#define configMAX_TASK_NAME_LEN                    16 /* длина имени задачи */
#define configUSE_16_BIT_TICKS                     0 /* 32-битный счётчик тиков */
#define configIDLE_SHOULD_YIELD                    1 /* idle уступает задачам того же приоритета */
#define configUSE_MUTEXES                          1 /* включить мьютексы */
#define configUSE_COUNTING_SEMAPHORES              1 /* включить считающие семафоры */
#define configUSE_RECURSIVE_MUTEXES                0 /* рекурсивные мьютексы не используем */
#define configQUEUE_REGISTRY_SIZE                  8 /* реестр очередей для отладки */
#define configUSE_QUEUE_SETS                       0 /* наборы очередей не нужны */
#define configUSE_TIME_SLICING                     1 /* делить квант между равными приоритетами */
#define configUSE_NEWLIB_REENTRANT                 0 /* без reent newlib */
#define configENABLE_BACKWARD_COMPATIBILITY        0 /* старые имена API не нужны */
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS    0 /* TLS-указатели не используем */
#define configSTACK_DEPTH_TYPE                     uint16_t /* тип глубины стека */
#define configMESSAGE_BUFFER_LENGTH_TYPE           size_t /* тип длины message buffer */

#define configUSE_TIMERS                           1 /* сервис программных таймеров */
#define configTIMER_TASK_PRIORITY                  2 /* приоритет задачи таймеров */
#define configTIMER_QUEUE_LENGTH                   8 /* очередь команд таймеров */
#define configTIMER_TASK_STACK_DEPTH               256 /* стек задачи таймеров в словах */

#define configCHECK_FOR_STACK_OVERFLOW             2 /* метод 2: проверка канарейки стека */
#define configUSE_MALLOC_FAILED_HOOK               1 /* хук при нехватке кучи */
#define configUSE_APPLICATION_TASK_TAG             0 /* теги задач не нужны */

#define configPRIO_BITS                            4 /* STM32F4: 4 бита приоритета NVIC */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY    15 /* самый низкий приоритет IRQ */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5 /* из IRQ <= 5 нельзя звать FromISR */
#define configKERNEL_INTERRUPT_PRIORITY /* приоритет ядра в формате NVIC */ \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS)) /* сдвиг в старшие биты */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY /* порог безопасных FromISR */ \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS)) /* сдвиг в старшие биты */

#define configASSERT(x) /* если условие ложно — остановить МК */ \
    do { /* однократный блок */ \
        if ((x) == 0) { /* проверка провалилась */ \
            __asm volatile ("cpsid i" ::: "memory"); /* запретить прерывания */ \
            for (;;) { /* крутиться здесь */ \
            } /* конец вечного цикла */ \
        } /* конец if */ \
    } while (0) /* макрос как оператор */

#define INCLUDE_vTaskPrioritySet                   1 /* разрешить менять приоритет */
#define INCLUDE_uxTaskPriorityGet                  1 /* разрешить читать приоритет */
#define INCLUDE_vTaskDelete                        1 /* разрешить удаление задач */
#define INCLUDE_vTaskSuspend                       1 /* разрешить suspend/resume */
#define INCLUDE_xResumeFromISR                     1 /* resume из прерывания */
#define INCLUDE_vTaskDelayUntil                    1 /* абсолютная задержка */
#define INCLUDE_vTaskDelay                         1 /* относительная задержка */
#define INCLUDE_xTaskGetSchedulerState             1 /* состояние планировщика */
#define INCLUDE_xTaskGetCurrentTaskHandle          1 /* хэндл текущей задачи */
#define INCLUDE_uxTaskGetStackHighWaterMark        1 /* запас стека */
#define INCLUDE_xTaskGetIdleTaskHandle             0 /* хэндл idle не нужен */
#define INCLUDE_eTaskGetState                      1 /* состояние задачи */
#define INCLUDE_xEventGroupSetBitFromISR           1 /* event group из ISR */
#define INCLUDE_xTimerPendFunctionCall             1 /* отложенный вызов через таймер */
#define INCLUDE_xTaskAbortDelay                    0 /* abort delay не используем */
#define INCLUDE_xTaskGetHandle                     0 /* поиск задачи по имени не нужен */

#endif /* конец конфига FreeRTOS */

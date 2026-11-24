/**
 * FreeRTOSConfig.h — конфігурація ядра ЛР4 (FreeRTOS-Kernel V11.1.0, порт GCC/ARM_CM0).
 *
 * Обладнання те саме, що в ЛР3: STM32C031C6 (Cortex-M0+, 48 МГц, 32 КБ Flash, 12 КБ RAM).
 * Кожне відхилення від типового значення пояснене поруч; зведення — docs/rtos-design.md §2.
 *
 * Варіанти збірки (make):  TICK_HZ=100   — сценарій 7.2.6 «вплив частоти тіку»
 *                          HEAP_BYTES=N  — сценарій 7.2.5 «вичерпання heap ОС»
 *                          STACK_CHECK=0 — сценарій 7.2.2 без контролю переповнення
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

/* ---------- планувальник ---------- */
#define configCPU_CLOCK_HZ                      48000000u
#ifndef LR4_TICK_HZ
#define LR4_TICK_HZ                             1000
#endif
/* 1 кГц: період IMU 5 мс = 5 тіків; той самий 1 мс, що був у HAL (uwTick) в ЛР3 */
#define configTICK_RATE_HZ                      ( ( TickType_t ) LR4_TICK_HZ )
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1       /* telem і oled мають рівний пріоритет */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0       /* на ARMv6-M немає CLZ */
#define configUSE_TICKLESS_IDLE                 0
/* 7 рівнів: 0 idle, 1 long_op, 2 telem/oled, 3 cmd, 4 journal, 5 imu, 6 btn (rtos-design.md §1) */
#define configMAX_PRIORITIES                    7
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 64 )   /* слів: лише для idle */
#define configMAX_TASK_NAME_LEN                 8
#define configTICK_TYPE_WIDTH_IN_BITS           TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configQUEUE_REGISTRY_SIZE               0
#define configUSE_QUEUE_SETS                    0
#define configUSE_NEWLIB_REENTRANT              0       /* malloc/printf newlib не використовуються */
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configUSE_MINI_LIST_ITEM                1
#define configSTACK_DEPTH_TYPE                  uint16_t
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t
#define configHEAP_CLEAR_MEMORY_ON_FREE         0

/* ---------- пам'ять: лише динамічна, heap_4 ---------- */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#ifndef LR4_HEAP_BYTES
#define LR4_HEAP_BYTES                          5888
#endif
/* Стеки задач + TCB + черги + м'ютекси; виміряне використання — команда `heap` */
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) LR4_HEAP_BYTES )
#define configAPPLICATION_ALLOCATED_HEAP        0

/* ---------- хуки і діагностика (методичка §4.11) ---------- */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     1       /* HAL_IncTick + debounce кнопки */
#ifndef LR4_STACK_CHECK
#define LR4_STACK_CHECK                         2
#endif
#define configCHECK_FOR_STACK_OVERFLOW          LR4_STACK_CHECK
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0
#define configUSE_TRACE_FACILITY                0       /* `tasks` — через API одного дескриптора (Flash) */
#define configUSE_STATS_FORMATTING_FUNCTIONS    0       /* друкуємо самі, у форматі key=value */
#define configGENERATE_RUN_TIME_STATS           1
#define configRUN_TIME_COUNTER_TYPE             uint32_t
/* Лічильник run-time stats: мікросекунди (plat_us, 32 біти → переповнення через 71.6 хв).
   DWT CYCCNT на Cortex-M0+ немає; такти (plat_cycles) переповнились би за 89.5 с. */
uint32_t plat_us(void);
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
#define portGET_RUN_TIME_COUNTER_VALUE()        plat_us()

/* ---------- службові задачі ядра: не потрібні ---------- */
#define configUSE_TIMERS                        0
#define configUSE_CO_ROUTINES                   0

/* ---------- порт ARMv6-M ---------- */
#define configENABLE_MPU                        0
#define configENABLE_FPU                        0
#define configENABLE_TRUSTZONE                  0
#define configRUN_FREERTOS_SECURE_ONLY          0
#define configCHECK_HANDLER_INSTALLATION        1
/* Cortex-M0+ має 2 біти пріоритету (0x00, 0x40, 0x80, 0xC0). Порт CM0 не використовує
   BASEPRI: критична секція вимикає ВСІ переривання (PRIMASK), тож обмеження
   configMAX_SYSCALL_INTERRUPT_PRIORITY тут немає — API FromISR можна викликати з будь-якого
   ISR. Ядро (SysTick, PendSV) — найнижчий пріоритет. */
#define configKERNEL_INTERRUPT_PRIORITY         0xC0

/* ---------- INCLUDE_ ---------- */
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskDelayUntil                 1
#define INCLUDE_vTaskDelete                     1       /* лише тестові задачі сценаріїв 7.2 */
#define INCLUDE_vTaskSuspend                    1       /* portMAX_DELAY = безстрокове очікування */
#define INCLUDE_vTaskPrioritySet                0
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTaskGetHandle                  0
#define INCLUDE_xTimerPendFunctionCall          0
#define INCLUDE_xSemaphoreGetMutexHolder        0
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskResumeFromISR              0

/* ---------- assert і trace ---------- */
void os_assert_failed(const char *file, int line);
#define configASSERT( x )    do { if( ( x ) == 0 ) os_assert_failed( __FILE__, __LINE__ ); } while( 0 )

/* Хронологія задач (методичка §6.5): кожне перемикання — у кільцевий буфер з міткою часу,
   опційно — службові піни (команда `trace pins`). Platform/os_freertos.c */
void os_trace_switched_in(void);
#define traceTASK_SWITCHED_IN()    os_trace_switched_in()

#endif /* FREERTOS_CONFIG_H */

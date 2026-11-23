#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>
#define configCPU_CLOCK_HZ 8000000UL
#define configTICK_RATE_HZ 1000U
#define configUSE_PREEMPTION 1
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configMAX_PRIORITIES 4
#define configMINIMAL_STACK_SIZE 128
#define configTOTAL_HEAP_SIZE (10U * 1024U)
#define configMAX_TASK_NAME_LEN 12
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1
#define configUSE_TIMERS 0
#define configUSE_MUTEXES 0
#define configCHECK_FOR_STACK_OVERFLOW 2
#define INCLUDE_vTaskDelay 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_vTaskSuspend 1
#define configPRIO_BITS 4
#define configKERNEL_INTERRUPT_PRIORITY (15U << 4)
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (5U << 4)
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

void app_fault(void);
#define configASSERT(condition) do { if (!(condition)) app_fault(); } while (0)

#endif

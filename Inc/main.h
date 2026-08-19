
#ifndef MAIN_H_
#define MAIN_H_

#include <stdint.h>

/* scheduler settings */
#define MAX_TASKS           5U
#define TICK_HZ             1000U

/* stack sizes */
#define SIZE_TASK_STACK     1024U
#define SIZE_SCHED_STACK    1024U

/* SRAM settings */
#define SRAM_START          0x20000000U
#define SIZE_SRAM           (36U * 1024U)
#define SRAM_END            (SRAM_START + SIZE_SRAM)

/* task stack locations */
#define T1_STACK_START      SRAM_END
#define T2_STACK_START      (SRAM_END - (1U * SIZE_TASK_STACK))
#define T3_STACK_START      (SRAM_END - (2U * SIZE_TASK_STACK))
#define T4_STACK_START      (SRAM_END - (3U * SIZE_TASK_STACK))
#define IDLE_STACK_START    (SRAM_END - (4U * SIZE_TASK_STACK))
#define SCHED_STACK_START   (SRAM_END - (5U * SIZE_TASK_STACK))

/* clock settings */
#define HSI_CLK             16000000U
#define SYSTICK_TIM_CLK     HSI_CLK

/* interrupt control */
#define INTERRUPT_DISABLE()                         \
    do                                              \
    {                                               \
        __asm volatile("MOV R0, #0x1");             \
        __asm volatile("MSR PRIMASK, R0");          \
    } while (0)

#define INTERRUPT_ENABLE()                          \
    do                                              \
    {                                               \
        __asm volatile("MOV R0, #0x0");             \
        __asm volatile("MSR PRIMASK, R0");          \
    } while (0)

#endif /* MAIN_H_ */

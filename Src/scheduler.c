/*
 * scheduler.c
 *
 * Preemptive task scheduler implementation for STM32G0.
 *
 * The scheduler uses:
 * - SysTick as the system time base
 * - PendSV to perform context switches
 * - PSP for task execution
 * - MSP for the scheduler/handler execution
 * - A TCB for each task
 */

#include <stdint.h>
#include "main.h"
#include "scheduler.h"

extern void idle_task(void);
extern void task1_handler(void);
extern void task2_handler(void);
extern void task3_handler(void);
extern void task4_handler(void);

/* Currently running task */
uint8_t current_task = 1U;

/* Global system tick counter */
static volatile uint32_t g_tick_count = 0U;

/* Task Control Blocks */
TCB_t user_tasks[MAX_TASKS];


/*
 * Initialize the scheduler stack.
 *
 * MSP is moved to the top of the scheduler stack.
 */
__attribute__((naked))
void init_scheduler_stack(uint32_t sched_top_of_stack)
{
    __asm volatile("MSR MSP, %0" : : "r"(sched_top_of_stack) : );
    __asm volatile("BX LR");
}


/*
 * Initialize task control blocks and create
 * the initial stack frame for every task.
 */
void init_tasks_stack(void)
{
    /* Initialize task states */
    for (uint32_t i = 0U; i < MAX_TASKS; i++)
    {
        user_tasks[i].current_state = TASK_READY_STATE;
    }

    /* Assign stack regions */
    user_tasks[0].psp_value = IDLE_STACK_START;
    user_tasks[1].psp_value = T1_STACK_START;
    user_tasks[2].psp_value = T2_STACK_START;
    user_tasks[3].psp_value = T3_STACK_START;
    user_tasks[4].psp_value = T4_STACK_START;

    /* Assign task entry functions */
    user_tasks[0].task_handler = idle_task;
    user_tasks[1].task_handler = task1_handler;
    user_tasks[2].task_handler = task2_handler;
    user_tasks[3].task_handler = task3_handler;
    user_tasks[4].task_handler = task4_handler;

    uint32_t *pPSP;

    /*
     * Build an initial stack frame for each task.
     *
     * When the task is selected for the first time,
     * the exception return mechanism restores this
     * frame and execution begins at the task handler.
     */
    for (uint32_t i = 0U; i < MAX_TASKS; i++)
    {
        pPSP = (uint32_t *)user_tasks[i].psp_value;

        /* Hardware-saved exception frame */

        pPSP--;
        *pPSP = 0x01000000U;   /* xPSR - Thumb state */

        pPSP--;
        *pPSP = (uint32_t)user_tasks[i].task_handler;   /* PC */

        pPSP--;
        *pPSP = 0xFFFFFFFDU;   /* LR - exception return */

        /*
         * Reserve space for the remaining registers.
         * These registers are restored by PendSV.
         */
        for (uint32_t j = 0U; j < 13U; j++)
        {
            pPSP--;
            *pPSP = 0U;
        }

        /* Store the initial PSP in the TCB */
        user_tasks[i].psp_value = (uint32_t)pPSP;
    }
}


/*
 * Return the PSP of the currently selected task.
 */
uint32_t get_psp_value(void)
{
    return user_tasks[current_task].psp_value;
}


/*
 * Save the current task's PSP into its TCB.
 */
void save_psp_value(uint32_t current_psp_value)
{
    user_tasks[current_task].psp_value = current_psp_value;
}


/*
 * Select the next READY task using round-robin scheduling.
 *
 * Task 0 is the idle task and is selected when
 * no other task is READY.
 */
void update_next_task(void)
{
    STATE_t state = TASK_BLOCKED_STATE;

    for (uint32_t i = 0U; i < MAX_TASKS; i++)
    {
        current_task++;
        current_task = current_task % MAX_TASKS;

        state = user_tasks[current_task].current_state;

        if ((state == TASK_READY_STATE) && (current_task != 0U))
        {
            break;
        }
    }

    /* No READY application task found -> run idle task */
    if (state != TASK_READY_STATE)
    {
        current_task = 0U;
    }
}


/*
 * Switch Thread mode from MSP to PSP.
 *
 * After this function, tasks execute using PSP.
 */
__attribute__((naked))
void switch_sp_to_psp(void)
{
    __asm volatile("PUSH {LR}");

    /* Get PSP of the first task */
    __asm volatile("BL get_psp_value");

    /* Load task PSP */
    __asm volatile("MSR PSP, R0");

    /* Restore LR */
    __asm volatile("POP {R1}");
    __asm volatile("MOV LR, R1");

    /*
     * CONTROL = 0x02:
     * SPSEL = 1 -> Thread mode uses PSP.
     */
    __asm volatile("MOVS R0, #2");
    __asm volatile("MSR CONTROL, R0");

    /* Synchronize CONTROL update */
    __asm volatile("ISB");

    __asm volatile("BX LR");
}


/*
 * Configure SysTick to generate periodic interrupts.
 */
void init_systick_timer(uint32_t tick_hz)
{
    uint32_t *pSRVR = (uint32_t *)0xE000E014;
    uint32_t *pSCSR = (uint32_t *)0xE000E010;

    uint32_t count_value =
        (SYSTICK_TIM_CLK / tick_hz) - 1U;

    /* Configure reload value */
    *pSRVR &= ~(0x00FFFFFFU);
    *pSRVR |= count_value;

    /*
     * SCSR:
     * Bit 0 = ENABLE
     * Bit 1 = TICKINT
     * Bit 2 = CLKSOURCE
     */
    *pSCSR |= 0x7U;
}


/*
 * Request a PendSV exception.
 *
 * PendSV is used to perform the actual
 * context switch.
 */
void schedule(void)
{
    uint32_t *pICSR = (uint32_t *)0xE000ED04;

    /* Set PendSVSET bit */
    *pICSR |= (1UL << 28);
}


/*
 * Block the current task for the requested
 * number of SysTick periods.
 */
void task_delay(uint32_t tick_count)
{
    INTERRUPT_DISABLE();

    /*
     * Task 0 is the idle task and should never
     * be blocked.
     */
    if (current_task != 0U)
    {
        /* Calculate absolute wake-up tick */
        user_tasks[current_task].block_count =
            g_tick_count + tick_count;

        /* Move task to BLOCKED state */
        user_tasks[current_task].current_state =
            TASK_BLOCKED_STATE;

        /* Request a context switch */
        schedule();
    }

    INTERRUPT_ENABLE();
}


/*
 * PendSV exception handler.
 *
 * Responsible for:
 * 1. Saving the current task context
 * 2. Saving its PSP
 * 3. Selecting the next READY task
 * 4. Restoring the next task context
 * 5. Updating PSP
 * 6. Returning to the selected task
 *
 * Cortex-M0+ does not allow PUSH/POP of R8-R11,
 * therefore R8-R11 are temporarily copied into
 * R4-R7 during context save/restore.
 */
__attribute__((naked))
void PendSV_Handler(void)
{
    __asm volatile(".syntax unified");

    /* Get current task PSP */
    __asm volatile("MRS R0, PSP");

    /*
     * Reserve 32 bytes for R4-R11.
     */
    __asm volatile("SUBS R0, R0, #32");

    /* Save R4-R7 */
    __asm volatile("STMIA R0!, {R4-R7}");

    /* Copy R8-R11 into low registers */
    __asm volatile("MOV R4, R8");
    __asm volatile("MOV R5, R9");
    __asm volatile("MOV R6, R10");
    __asm volatile("MOV R7, R11");

    /* Save R8-R11 */
    __asm volatile("STMIA R0!, {R4-R7}");

    /* Return R0 to beginning of saved context */
    __asm volatile("SUBS R0, R0, #32");

    /*
     * Preserve exception return value while
     * calling C functions.
     */
    __asm volatile("PUSH {LR}");

    /* Save current task PSP */
    __asm volatile("BL save_psp_value");

    /* Select next task */
    __asm volatile("BL update_next_task");

    /* Get next task PSP */
    __asm volatile("BL get_psp_value");

    /*
     * Skip saved R4-R7 and reach saved R8-R11.
     */
    __asm volatile("ADDS R0, R0, #16");

    /* Restore R8-R11 */
    __asm volatile("LDMIA R0!, {R4-R7}");

    __asm volatile("MOV R8, R4");
    __asm volatile("MOV R9, R5");
    __asm volatile("MOV R10, R6");
    __asm volatile("MOV R11, R7");

    /* Return to beginning of saved context */
    __asm volatile("SUBS R0, R0, #32");

    /* Restore R4-R7 */
    __asm volatile("LDMIA R0!, {R4-R7}");

    /*
     * Move PSP past the software-saved context.
     */
    __asm volatile("ADDS R0, R0, #16");

    /* Update PSP for the next task */
    __asm volatile("MSR PSP, R0");

    /* Restore exception return value */
    __asm volatile("POP {R0}");
    __asm volatile("MOV LR, R0");

    /* Return from exception */
    __asm volatile("BX LR");
}


/*
 * Increment the global system tick.
 */
void update_global_tick_count(void)
{
    g_tick_count++;
}


/*
 * Move blocked tasks back to READY state
 * when their delay expires.
 */
void unblock_tasks(void)
{
    for (uint32_t i = 1U; i < MAX_TASKS; i++)
    {
        if (user_tasks[i].current_state != TASK_READY_STATE)
        {
            /*
             * Use <= rather than == so that a task
             * is still released if the exact tick
             * was missed.
             */
            if (user_tasks[i].block_count <= g_tick_count)
            {
                user_tasks[i].current_state =
                    TASK_READY_STATE;
            }
        }
    }
}


/*
 * SysTick interrupt handler.
 *
 * SysTick provides the scheduler's time base.
 */
void SysTick_Handler(void)
{
    uint32_t *pICSR = (uint32_t *)0xE000ED04;

    /* Update system time */
    update_global_tick_count();

    /* Wake tasks whose delay has expired */
    unblock_tasks();

    /* Request a context switch */
    *pICSR |= (1UL << 28);
}

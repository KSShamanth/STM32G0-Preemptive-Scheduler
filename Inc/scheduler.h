#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <stdint.h>

/*
 * Task states used by the scheduler.
 */
typedef enum
{
    TASK_READY_STATE,
    TASK_BLOCKED_STATE
} STATE_t;

/*
 * Task Control Block (TCB).
 *
 * Each task has:
 * - Its saved Process Stack Pointer (PSP)
 * - The tick at which it should become ready
 * - Its current scheduler state
 * - Its task entry function
 */
typedef struct
{
    uint32_t psp_value;
    uint32_t block_count;
    STATE_t current_state;
    void (*task_handler)(void);
} TCB_t;


/* Scheduler stack initialization */
__attribute__((naked))
void init_scheduler_stack(uint32_t sched_top_of_stack);

/* Task stack initialization */
void init_tasks_stack(void);


/* PSP management */
uint32_t get_psp_value(void);
void save_psp_value(uint32_t current_psp_value);


/* Switch Thread mode from MSP to PSP */
__attribute__((naked))
void switch_sp_to_psp(void);


/* Task selection */
void update_next_task(void);


/* SysTick configuration */
void init_systick_timer(uint32_t tick_hz);


/* Task blocking and scheduling */
void task_delay(uint32_t tick_count);
void schedule(void);


/* System tick management */
void update_global_tick_count(void);
void unblock_tasks(void);

#endif /* SCHEDULER_H_ */

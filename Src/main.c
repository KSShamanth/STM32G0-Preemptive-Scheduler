#include <stdint.h>
#include "main.h"
#include "scheduler.h"


/* task counters */
volatile uint32_t count1 = 0U;
volatile uint32_t count2 = 0U;
volatile uint32_t count3 = 0U;
volatile uint32_t count4 = 0U;


/* task functions */
void idle_task(void);
void task1_handler(void);
void task2_handler(void);
void task3_handler(void);
void task4_handler(void);


int main(void)
{
    /* initialize scheduler */
    init_scheduler_stack(SCHED_STACK_START);
    init_tasks_stack();
    init_systick_timer(TICK_HZ);

    /* switch to PSP */
    switch_sp_to_psp();

    /* start first task */
    task1_handler();

    for (;;);
}


/* idle task */
void idle_task(void)
{
    while (1);
}


/* task 1 */
void task1_handler(void)
{
    while (1)
    {
        count1++;

        task_delay(1000);
    }
}


/* task 2 */
void task2_handler(void)
{
    while (1)
    {
        count2++;

        task_delay(500);
    }
}


/* task 3 */
void task3_handler(void)
{
    while (1)
    {
        count3++;

        task_delay(250);
    }
}


/* task 4 */
void task4_handler(void)
{
    while (1)
    {
        count4++;

        task_delay(125);
    }
}


/* hard fault handler */
void HardFault_Handler(void)
{
    while (1);
}

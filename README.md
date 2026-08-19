# STM32G0 Preemptive Task Scheduler

A bare-metal preemptive task scheduler implemented on the STM32G070RB using SysTick, PendSV, PSP, MSP, and manual context switching.

## Hardware

* NUCLEO-G070RB
* STM32G070RB
* ARM Cortex-M0+
* 16 MHz clock
* 1 kHz SysTick

## Features

* Preemptive task scheduling
* SysTick-based timing
* PendSV-based context switching
* PSP-based task execution
* Separate stack for each task
* Task Control Block (TCB)
* READY and BLOCKED states
* Task delay
* Round-robin scheduling
* Idle task
* Manual R4-R11 context save/restore

## Project Structure

```text
Inc/
├── main.h
└── scheduler.h

Src/
├── main.c
└── scheduler.c
```

## Build & Flash

1. Open STM32CubeIDE and import the project (`File → Open Projects from File System`, point it at the project root).
2. Select the `NUCLEO-G070RB` board target if prompted, or confirm the linked MCU is `STM32G070RB`.
3. Build with `Project → Build Project` (or the hammer icon).
4. Connect the Nucleo board via USB (uses the onboard ST-LINK).
5. Flash and run with `Run → Debug` or `Run → Run`.

To observe scheduling behavior, add `count1`–`count4` and `current_task` to a **Live Expressions** or **Watch** window during a debug session.

## How It Works

```text
SysTick
   ↓
Update tick
   ↓
Unblock tasks
   ↓
Request PendSV
   ↓
Save current task context
   ↓
Select next READY task
   ↓
Restore next task context
   ↓
Run next task
```

`SysTick` runs at 1 kHz and provides the scheduler time base.

`PendSV` performs the actual context switch by saving and restoring the task context.

Each task has its own stack and its PSP is stored in its TCB.

## Tasks

| Task   |        Delay | Ticks (@ 1 kHz) |
| ------ | -----------: | ---------------: |
| Task 1 |      1000 ms |              1000 |
| Task 2 |       500 ms |               500 |
| Task 3 |       250 ms |               250 |
| Task 4 |       125 ms |               125 |
| Idle   | Always ready |                 — |

The tasks increment individual counters to validate scheduling and timing.

## Validation

The scheduler was tested for:

* Task switching
* Task blocking and wake-up
* SysTick timing
* PendSV context switching
* PSP switching
* Separate task stacks
* Idle task execution

Expected task execution rates:

```text
Task 1 → ~1 execution/sec
Task 2 → ~2 executions/sec
Task 3 → ~4 executions/sec
Task 4 → ~8 executions/sec
```

### Measured results

Counter values from a ~12.5 s debug session (Live Expressions window):

| Task   | Count at ~12s | Measured rate | Expected rate |
| ------ | ------------: | -------------: | -------------: |
| Task 1 |            12 |       ~1.0/sec |         ~1/sec |
| Task 2 |            23 |       ~1.9/sec |         ~2/sec |
| Task 3 |            45 |      ~3.75/sec |         ~4/sec |
| Task 4 |            90 |       ~7.5/sec |         ~8/sec |

Measured rates track the expected 1:2:4:8 ratio closely, confirming correct round-robin scheduling and SysTick-driven timing.

> Note: `current_task` was observed to read `0` (idle) at every sampled point during this session. This is a Live Expressions polling artifact — reads aren't synchronized to task switches — not a scheduler fault, since the counter values confirm all four tasks are executing at the expected rates. A breakpoint-based watch or UART logging gives a more reliable view of `current_task` in real time.

## Semihosting / printf

`printf()` with semihosting was initially tested inside the task handlers but was removed from the final implementation.

Semihosting is debugger-dependent and adds significant overhead, making it unsuitable for validating this custom context-switching implementation.

Task counters and timing were used for scheduler validation instead. UART is recommended for embedded runtime logging.

## Limitations

* Fixed number of tasks
* Fixed stack sizes
* No priority scheduling
* No synchronization primitives
* No stack overflow detection
* Manual stack allocation

## Tools

* STM32CubeIDE
* ARM GCC
* STM32G070RB
* C

## License

MIT
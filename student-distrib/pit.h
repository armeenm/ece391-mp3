#ifndef PIT_H
#define PIT_H

#include "types.h"

#define PIT_CHANNEL_0       0x40
#define PIT_CHANNEL_1       0x41
#define PIT_CHANNEL_2       0x42
#define PIT_MODE_REGISTER   0x43

#define PIT_SET_CHANNEL_0       0x00
#define PIT_SET_CHANNEL_1       0x40
#define PIT_SET_CHANNEL_2       0x80

#define PIT_SET_ACCESS_MODE_0   0x00
#define PIT_SET_ACCESS_MODE_1   0x10
#define PIT_SET_ACCESS_MODE_2   0x20
#define PIT_SET_ACCESS_MODE_3   0x30

#define PIT_SET_MODE_TERMINAL_COUNT     0x00
#define PIT_SET_MODE_HARDWARE_ONE_SHOT  0x02
#define PIT_SET_MODE_RATE               0x04
#define PIT_SET_MODE_SQUARE_WAVE        0x06
#define PIT_SET_MODE_SOFTWARE_STROBE    0x08
#define PIT_SET_MODE_HARDWARE_STROBE    0x0A

#define PIT_SET_BCD_MODE_0  0x00
#define PIT_SET_BCD_MODE_1  0x01

#define PIT_IRQ 0x0

#define PIT_FREQ 1193182
#define SCHEDULE_TIME 10

#define TASK_NOT_RUNNING        0
#define TASK_RUNNING            1
#define TASK_INTERRUPTIBLE      2
#define TASK_UNINTERRUPTIBLE    3
#define TASK_STOPPED            4
#define TASK_ZOMBIE             5

void irqh_pit(void);
void init_pit(void);
u8 get_current_schedule(void);

#endif

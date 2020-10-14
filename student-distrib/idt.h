#ifndef IDT_H
#define IDT_H

#include "x86_desc.h"

#define EXCEPTION_CNT 32
#define RTC_IDT 0x20
#define KEYBOARD_IDT 0x21
#define SYSCALL_IDT 0x80

/* Initialize the IDT */
void init_idt(void);

#endif

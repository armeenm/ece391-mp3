#ifndef IDT_H
#define IDT_H

#define EXCEPTION_CNT 32
#define PIT_IDT 0x20
#define KEYBOARD_IDT 0x21
#define RTC_IDT 0x28
#define SYSCALL_IDT 0x80

/* Initialize the IDT */
void init_idt(void);

#endif

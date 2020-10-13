#ifndef IDT_H
#define IDT_H

#include "x86_desc.h"

#define SYSCALL_IDT_IDX 0x80

/* Initialize the IDT */
void init_idt(void);

#endif

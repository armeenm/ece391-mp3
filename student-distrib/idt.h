#ifndef IDT_H
#define IDT_H

#include "lib.h"

enum { IDT_EXC_CNT = 32, IDT_PIT = 0x20, IDT_KEYBOARD = 0x21, IDT_RTC = 0x28, IDT_SYSCALL = 0x80 };
typedef enum Dpl { DPL0 = 0, DPL3 = 3 } Dpl;
typedef enum GateType { TASK = 5, INT = 6, TRAP = 7 } GateType;
typedef union GateTypeU {
  GateType val;

  struct {
    uint32_t reserved3 : 1;
    uint32_t reserved2 : 1;
    uint32_t reserved1 : 1;
  };

} GateTypeU;

/* Initialize the IDT */
void init_idt(void);

#endif

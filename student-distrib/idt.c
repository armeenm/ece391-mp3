#include "idt.h"

void init_idt(void) {
  uint16_t i;

  /* Configure first 32 IDT entries
   * These are defined by Intel
   */
  for (i = 0; i < 32; ++i) {
    /* 32-bit trap gate type */
    idt[i].present = 1;
    idt[i].dpl = 0;       /* Ring 0 only; cannot be called from userspace */
    idt[i].reserved0 = 0; /* Should be 0 for interrupts/traps */
    idt[i].size = 1;
    idt[i].reserved1 = 1;
    idt[i].reserved2 = 1;
    idt[i].reserved3 = 1;
    idt[i].reserved4 = 0; /* Unused */
    idt[i].seg_selector = KERNEL_CS;
  }

  /* Syscalls must be callable from ring 3 */
  idt[SYSCALL_IDT_IDX].present = 1;
  idt[SYSCALL_IDT_IDX].dpl = 3;
  idt[SYSCALL_IDT_IDX].reserved0 = 0;
  idt[SYSCALL_IDT_IDX].size = 1;
  idt[SYSCALL_IDT_IDX].reserved1 = 1;
  idt[SYSCALL_IDT_IDX].reserved2 = 1;
  idt[SYSCALL_IDT_IDX].reserved3 = 0;
  idt[SYSCALL_IDT_IDX].reserved4 = 0; /* Unused */
  idt[SYSCALL_IDT_IDX].seg_selector = KERNEL_CS;
}

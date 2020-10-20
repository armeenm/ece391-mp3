#include "syscall.h"
#include "lib.h"

void irqh_syscall() {
  SyscallType type;

  /* Read the syscall type from EAX */
  asm volatile("" : "=a"(type));

  switch (type) {
  case SYSC_HALT:
    printf("HALT!\n");
    break;

  case SYSC_EXEC:
    break;

  case SYSC_READ:
    break;

  case SYSC_WRITE:
    break;

  case SYSC_OPEN:
    break;

  case SYSC_CLOSE:
    break;

  case SYSC_GETARGS:
    break;

  case SYSC_VIDMAP:
    break;

  case SYSC_SET_HANDLER:
    break;

  case SYSC_SIGRETURN:
    break;
  };

  printf("Handling syscall...\n");
}

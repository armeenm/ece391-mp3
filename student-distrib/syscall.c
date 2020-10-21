#include "syscall.h"
#include "lib.h"

void irqh_syscall() {
  SyscallType type;
  uint32_t arg1, arg2, arg3;

  /* Read the syscall type from EAX */
  asm volatile("" : "=a"(type), "=b"(arg1), "=c"(arg2), "=d"(arg3));

  printf("Handling syscall...\n");

  switch (type) {
  case SYSC_HALT:
    printf("HALTING!\n");
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

  asm volatile("" ::"a"(type), "c"(arg2), "d"(arg3));
}

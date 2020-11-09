#define ASM 1
#include "x86_desc.h"

#define USER_ESP 0x83FFFF0

.align 4

.globl uspace

uspace:
  mov 4(%esp), %eax # New EIP

  mov $USER_DS, %ecx
  mov %cx, %ds
  mov %cx, %es
  mov %cx, %fs
  mov %cx, %gs

  mov %esp, %ecx
  push $USER_DS
  push %ecx

  pushf
  pop %edx
  or $0x200, %edx
  push %edx

  push $USER_CS
  push %eax

  iret

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
  push %ecx

  mov %esp, %ecx
  push %ecx
  pushf
  push $USER_CS
  push %eax

  iret

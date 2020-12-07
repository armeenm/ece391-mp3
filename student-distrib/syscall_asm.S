#define ASM 1
#include "x86_desc.h"

.align 4

.globl uspace


/* uspace
 * Description: Jumps to userspace at the given EIP.
 * Inputs: EIP to jump to.
 * Outputs: None
 * Function: Sets up the segment registers
 *           Appropriately fills the stack with 
 *           Initializes CR0, CR3, and CR4 to enable paging.
 */
uspace:
  cli

  mov 4(%esp), %eax # New EIP

  /* Set DS, ES, FS, GS to userspace data segment value */
  mov $USER_DS, %ecx
  mov %cx, %ds
  mov %cx, %es
  mov %cx, %fs
  mov %cx, %gs

  /* Store old (kernel) ESP */
  mov %esp, %ecx
  push $USER_DS
  push %ecx

  /* Enable interrupts */
  pushf
  pop %edx
  or $0x200, %edx
  push %edx

  /* Userspace code segment */
  push $USER_CS
  /* New EIP */
  push %eax

  iret

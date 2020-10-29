#include "syscall.h"
#include "lib.h"
#include "util.h"

i32 halt(u8 UNUSED(status)) { NIMPL; }

i32 execute(u8 const* UNUSED(command)) { NIMPL; }

i32 read(i32 UNUSED(fd), void* UNUSED(buf), i32 UNUSED(nbytes)) { NIMPL; }

i32 write(i32 UNUSED(fd), void const* UNUSED(buf), i32 UNUSED(nbytes)) { NIMPL; }

i32 open(u8 const* UNUSED(filename)) { NIMPL; }

i32 close(i32 UNUSED(fd)) { NIMPL; }

i32 getargs(u8* UNUSED(buf), i32 UNUSED(nbytes)) { NIMPL; }

i32 vidmap(u8** UNUSED(screen_start)) { NIMPL; }

i32 irqh_syscall(void) {
  SyscallType type;
  u32 arg1, arg2, arg3;

  /* Read the syscall type and arguments */
  asm volatile("" : "=a"(type), "=b"(arg1), "=c"(arg2), "=d"(arg3));

  printf("Handling syscall...\n");

  switch (type) {
  case SYSC_HALT:
    return halt(arg1);

  case SYSC_EXEC:
    return execute((u8 const*)arg1);

  case SYSC_READ:
    return read(*(i32*)&arg1, (void*)arg2, *(i32*)&arg3);

  case SYSC_WRITE:
    return write(*(i32*)&arg1, (void const*)arg2, *(i32*)&arg3);

  case SYSC_OPEN:
    return open((u8 const*)arg1);

  case SYSC_CLOSE:
    return close(*(i32*)&arg1);

  case SYSC_GETARGS:
    return getargs((u8*)arg1, *(i32*)&arg1);

  case SYSC_VIDMAP:
    return vidmap((u8**)arg1);

  case SYSC_SET_HANDLER:
    NIMPL;

  case SYSC_SIGRETURN:
    NIMPL;
  };

  /* EAX, ECX, EDX: Handled in ASM linkage
   * EBX, EDI, ESI: Handled by the compiler
   */
}

#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

enum { FD_CNT = 8, ARGBUF_SIZE = 128 };

typedef enum SyscallType {
  SYSC_HALT = 1,
  SYSC_EXEC,
  SYSC_READ,
  SYSC_WRITE,
  SYSC_OPEN,
  SYSC_CLOSE,
  SYSC_GETARGS,
  SYSC_VIDMAP,
  SYSC_SET_HANDLER,
  SYSC_SIGRETURN
} SyscallType;

typedef struct FileDesc {
  u32* jumptable;
  u32 flags;
  u32 inode;
} FileDesc;

typedef struct Pcb {
  FileDesc fds[FD_CNT];
  i8 argbuf[ARGBUF_SIZE];
  u32 state;
  u32 pid;
  u32 parent_pid;
} Pcb;

i32 halt(u8 status);
i32 execute(u8 const* command);
i32 read(i32 fd, void* buf, i32 nbytes);
i32 write(i32 fd, void const* buf, i32 nbytes);
i32 open(u8 const* filename);
i32 close(i32 fd);
i32 getargs(u8* buf, i32 nbytes);
i32 vidmap(u8** screen_start);

i32 irqh_syscall(void);

#endif

#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

#define FD_IN_USE 0x00000001
#define FD_NOT_IN_USE 0x00000000

enum { FD_CNT = 8, ARGS_SIZE = 128 };

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

typedef struct FileOps {
  i32 (*open)(u8 const* filename);
  i32 (*close)(i32 fd);
  i32 (*read)(i32 fd, void* buf, i32 nbytes);
  i32 (*write)(i32 fd, void const* buf, i32 nbytes);
} FileOps;

typedef struct FileDesc {
  FileOps const* jumptable;
  u32 inode;
  u32 file_position;
  u32 flags;
} FileDesc;

typedef struct Pcb {
  FileDesc fds[FD_CNT];
  i32 argc;
  i8 argv[ARGS_SIZE];
  u32 pid;
  u32 parent_ksp;
  u32 parent_kbp;
  u32 parent_pid;
} Pcb;

/* Implemented in syscall_asm.S */
void uspace(i32 entry);

i32 halt(u8 status);
i32 execute(u8 const* command);
i32 read(i32 fd, void* buf, i32 nbytes);
i32 write(i32 fd, void const* buf, i32 nbytes);
i32 open(u8 const* filename);
i32 close(i32 fd);
i32 getargs(u8* buf, i32 nbytes);
i32 vidmap(u8** screen_start);

i32 irqh_syscall(void);

Pcb* get_current_pcb(void);
i32 read_failure(i32 fd, void* buf, i32 nbytes);
i32 write_failure(i32 fd, void const* buf, i32 nbytes);

#endif

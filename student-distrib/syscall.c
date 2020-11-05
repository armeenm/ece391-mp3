#include "syscall.h"
#include "fs.h"
#include "lib.h"
#include "util.h"
#include "x86_desc.h"

enum { ENTRY_POINT_OFFSET = 24, LOAD_ADDR = 0x8048000, MB8 = 0x800000, KB8 = 0x2000 };

u8 const elf_header[] = {0x7F, 'E', 'L', 'F'};

u8 procs = 0x80;
u8 running_pid = 0;
u32 ksp;

static Pcb* get_pcb(u8 proc);
static Pcb* get_current_pcb(void);

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

  default:
    NIMPL;
  };

  /* EAX, ECX, EDX: Handled in ASM linkage
   * EBX, EDI, ESI: Handled by the compiler
   */
}

static Pcb* get_pcb(u8 proc) { return (Pcb*)(MB8 - (proc + 1) * KB8); }

static Pcb* get_current_pcb(void) { return (Pcb*)(ksp & 0xFFFFE); }

i32 halt(u8 UNUSED(status)) { NIMPL; }

i32 execute(u8 const* const ucmd) {
  i8 const* const cmd = (i8 const*)ucmd;
  u32 entry;
  u8 header[4];
  u8 mask;
  u32 i;

  if (!cmd)
    return -1;

  /* TODO: argc, argv, stdin, stdout */

  if (file_open(ucmd))
    return -1;

  if (file_read(cmd, header, 0, sizeof(header)) != sizeof(header))
    return -1;

  for (i = 0; i < sizeof(header); ++i)
    if (header[i] != elf_header[i])
      return -1;

  for (i = 0, mask = 0x80; i < 8; ++i, mask >>= 1)
    if (!(mask & procs)) {
      procs |= mask;
      running_pid = i;
      /* Go away, this code's great */
      goto cont;
    }

  return -1;

cont:
  if (file_read(cmd, (u8*)&entry, ENTRY_POINT_OFFSET, sizeof(entry)) != sizeof(entry))
    return -1;

  //  if (file_read(cmd, (u8*)LOAD_ADDR, 0, 0) == -1)
  //    return -1;

  //  if (make_task_pgdir(running_pid))
  //    return -1;

  if (file_read(cmd, (u8*)LOAD_ADDR, 0, 0) == -1)
    return -1;

#if 0

  {
    Pcb* const pcb = get_pcb(running_pid);
    Pcb* parent;
    u32 esp, ebp;

    asm volatile("mov %%esp, %0;"
                 "mov %%ebp, %1;"
                 : "=g"(esp), "=g"(ebp));

    parent = (Pcb*)(esp & 0xFFFFE);

    for (i = 0; i < FD_CNT; ++i) {
      pcb->fds[i].jumptable = NULL;
      pcb->fds[i].flags = 0;
      pcb->fds[i].inode = 0;
    }

    pcb->pid = running_pid;
    pcb->parent_ksp = esp;
    pcb->parent_kbp = ebp;
    pcb->parent_pid = (procs == 0xC0) ? 0 : parent->pid; /* Special case 1st proc */

    /* New KSP */
    tss.esp0 = ksp = MB8 - KB8 * running_pid - 4;

    uspace(entry);
  }
#endif

  return 0;
}

i32 read(i32 UNUSED(fd), void* UNUSED(buf), i32 UNUSED(nbytes)) { NIMPL; }

i32 write(i32 UNUSED(fd), void const* UNUSED(buf), i32 UNUSED(nbytes)) { NIMPL; }

i32 open(u8 const* UNUSED(filename)) { NIMPL; }

i32 close(i32 UNUSED(fd)) { NIMPL; }

i32 getargs(u8* UNUSED(buf), i32 UNUSED(nbytes)) { NIMPL; }

i32 vidmap(u8** UNUSED(screen_start)) { NIMPL; }

#include "syscall.h"
#include "fs.h"
#include "lib.h"
#include "rtc.h"
#include "terminal_driver.h"
#include "util.h"
#include "x86_desc.h"

enum { ENTRY_POINT_OFFSET = 24, LOAD_ADDR = 0x8048000, MB8 = 0x800000, KB8 = 0x2000 };

FileOps const std_in_fops = {terminal_open, terminal_close, terminal_read, write_failure};
FileOps const std_out_fops = {terminal_open, terminal_close, read_failure, terminal_write};
FileOps const rtc_fops = {rtc_open, rtc_close, rtc_read, rtc_write};
FileOps const fs_fops = {file_open, file_close, file_read, file_write};
FileOps const dir_fops = {dir_open, dir_close, dir_read, dir_write};

u8 const elf_header[] = {0x7F, 'E', 'L', 'F'};

u8 procs = 0x0;
u8 running_pid = 0;

static Pcb* get_pcb(u8 proc);

i32 irqh_syscall(void) {
  SyscallType type;
  u32 arg1, arg2, arg3;

  /* Read the syscall type and arguments */
  asm volatile("" : "=a"(type), "=b"(arg1), "=c"(arg2), "=d"(arg3));

  switch (type) {
  case SYSC_HALT:
    return halt(arg1);

  /* Execute */
  case SYSC_EXEC:
    return execute((u8 const*)arg1);

  /* Read */
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
  }

  /* EAX, ECX, EDX: Handled in ASM linkage
   * EBX, EDI, ESI: Handled by the compiler
   */
}

/* get_pcb
 * Description: Gets the current pcb
 * Inputs: proc --
 * Outputs: none
 * Return Value: none
 * Function:
 */
static Pcb* get_pcb(u8 proc) { return (Pcb*)(MB8 - (proc + 1) * KB8); }

/* get_current_pcb
 * Description:
 * Inputs: none
 * Outputs: none
 * Return Value:
 * Function:
 */
Pcb* get_current_pcb(void) { return get_pcb(running_pid); }

/* halt
 * Description:
 * Inputs: status -- UNUSED
 * Outputs: none
 * Return Value: none
 * Function: currently unimplemented
 */
i32 halt(u8 const status) {
  u32 i;
  Pcb* const pcb = get_current_pcb();

  /* If we're the "parent process" of the OS (pid == 0, shell) don't halt it */
  /* Close all FDs for the current process */
  for (i = 0; i < FD_CNT; ++i)
    close(i);

  if (pcb->parent_pid == -1) {
    tss.esp0 = MB8 - 4;
    running_pid = 0;
  } else {
    get_pcb(pcb->parent_pid)->child_return = status;
    /* There is a parent, we need to switch contexts to the parent */
    remove_task_pgdir(pcb->pid);
    make_task_pgdir(pcb->parent_pid);

    tss.esp0 = MB8 - KB8 * pcb->parent_pid - 4;
    running_pid = pcb->parent_pid;
  }

  procs &= ~(0x80U >> pcb->pid);

  asm volatile("mov %0, %%esp;"
               "mov %1, %%ebp;"
               "mov %2, %%eax;"
               :
               : "g"(pcb->parent_ksp), "g"(pcb->parent_kbp), "g"(pcb->child_return)
               : "eax", "esp", "ebp");

  asm volatile("leave;"
               "ret;");

  return -1;
  /* Not sure if this is correct -- halt is supposed to jump to execute return */
}

/* execute
 * Description: Executes system calls
 * Inputs: ucmd -- system call to execute
 * Outputs: none
 * Return Value: if fails return -1, if success return 0
 * Function: Checks cmd validity, if valid executes a system call given as ucmd input
 */
i32 execute(u8 const* const ucmd) {
  i8 cmd[ARGS_SIZE];
  Pcb* const parent = get_current_pcb();
  DirEntry dentry;
  u32 entry;
  u8 header[4];
  u8 mask;
  u32 i, j;
  u32 argc;

  if (!cmd)
    return -1;

  // copy the input argument
  memset(cmd, 0, ARGS_SIZE);
  strcpy(cmd, (i8 const*)ucmd);
  j = strlen(cmd);
  for (i = 0, argc = 1; i<j; i++) {
    // replace all spaces with null termination, and count up the number of args
    if (cmd[i] == ' ') {
      cmd[i] = '\0';
      argc++;
    }
  }

  /* If directory entry read fails, fail */
  if (read_dentry_by_name((u8*)cmd, &dentry))
    return -1;

  /* If the data read isn't the size of the data, fail */
  if (read_data(dentry.inode_idx, 0, header, sizeof(header)) != sizeof(header))
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
  /* If bytes read from file isn't the same as the size of the file, fail */
  if (file_read_name(cmd, (u8*)&entry, ENTRY_POINT_OFFSET, sizeof(entry)) != sizeof(entry))
    return -1;

  /* If making page directory fails, fail */
  if (make_task_pgdir(running_pid))
    return -1;

  /* If file read was unsuccessful after making pgdir, fail */
  if (file_read_name(cmd, (u8*)LOAD_ADDR, 0, 0) == -1)
    return -1;

  {
    Pcb* const pcb = get_current_pcb();
    u32 esp, ebp;

    // copy the ESP and EBP for the child process to return to
    asm volatile("mov %%esp, %0;"
                 "mov %%ebp, %1;"
                 : "=g"(esp), "=g"(ebp));

    for (i = 0; i < 2; ++i) {
      pcb->fds[i].jumptable = (i == 0) ? &std_in_fops : &std_out_fops;
      pcb->fds[i].flags = FD_IN_USE;
      pcb->fds[i].inode = 0;
      pcb->fds[i].file_position = 0;
    }

    for (i = 2; i < FD_CNT; ++i) {
      pcb->fds[i].jumptable = NULL;
      pcb->fds[i].flags = FD_NOT_IN_USE;
      pcb->fds[i].inode = 0;
      pcb->fds[i].file_position = 0;
    }

    // setup argv to point to sections of the raw_argv string to seperate args
    memcpy(pcb->raw_argv, cmd, ARGS_SIZE);
    pcb->argv[0] = pcb->raw_argv;
    for (i = 1, j = 1; i<ARGS_SIZE - 1; i++) {
      if (pcb->raw_argv[i] == (i8)'\0')
        pcb->argv[j++] = &pcb->raw_argv[i+1];
    }

    pcb->argc = argc;

    for (i = 0; i<argc; i++) {
      printf("ye : %s\n", pcb->argv[i]);
    }
    pcb->pid = running_pid;
    pcb->parent_ksp = esp;
    pcb->parent_kbp = ebp;
    pcb->parent_pid = (procs == 0x80U) ? -1 : (i32)parent->pid; /* Special case 1st proc */

    /* New KSP */
    tss.esp0 = MB8 - KB8 * running_pid - 4;

    uspace(entry);

    return pcb->child_return;
  }
}

/* read
 * Description: Reads n bytes into buffer
 * Inputs: fd -- file descriptor
 *         buf -- buffer to fill with n bytes
 *         nbytes -- number of bytes
 * Outputs: none
 * Return Value: if fails return -1, if success return 0
 * Function: Checks if inputs are valid, if so then read bytes to buffer
 */
i32 read(i32 const fd, void* const buf, i32 const nbytes) {
  Pcb* const pcb = get_current_pcb();

  /* If buffer is NULL, fd is invalid value, or nbytes is invalid value, fail */
  if (!buf || fd < 0 || fd >= FD_CNT || nbytes < 0)
    return -1;

  /* If PCB is NULL, file descriptor not in use, and ... */
  if (!pcb || ((pcb->fds[fd].flags & FD_IN_USE) == FD_NOT_IN_USE) || !pcb->fds[fd].jumptable)
    return -1;

  return pcb->fds[fd].jumptable->read(fd, buf, nbytes);
}

/* write
 * Description: Writes n bytes to file descriptor
 * Inputs: fd -- file descriptor
 *         buf -- buffer to fill with n bytes
 *         nbytes -- number of bytes
 * Outputs: none
 * Return Value: if fails return -1, if success return 0
 * Function: currently unimplemented
 */
i32 write(i32 fd, void const* buf, i32 nbytes) {
  /* If buffer is NULL, fd is invalid value, or nbytes is invalid value, fail */
  if (!buf || fd < 0 || fd >= FD_CNT || nbytes < 0)
    return -1;

  Pcb* pcb = get_current_pcb();
  /* If pcb is null, file descriptor not in use, and ... */
  if (!pcb || ((pcb->fds[fd].flags & FD_IN_USE) == FD_NOT_IN_USE) || !pcb->fds[fd].jumptable)
    return -1;

  return pcb->fds[fd].jumptable->write(fd, buf, nbytes);
}

/* open
 * Description: Opens a file to read
 * Inputs: filename -- name of the file to open
 * Outputs: none
 * Return Value: if fails return -1, if success return 0
 * Function: Checks for invalid inputs, if valid then open file
 */
i32 open(u8 const* filename) {
  DirEntry dentry;
  Pcb* const pcb = get_current_pcb();
  i32 fdIndex = 0, fdReturnValue = -1;

  /* If filename is null, or empty, fail */
  if (!filename || filename[0] == '\0')
    return -1;

  /* If directory entry read fails, fail */
  if (read_dentry_by_name(filename, &dentry) == -1)
    return -1;

  /* If pcb is null, fail */
  if (!pcb)
    return -1;

  for (fdIndex = 2; fdIndex < FD_CNT; ++fdIndex) {
    if (pcb->fds[fdIndex].flags & FD_IN_USE)
      continue;

    switch (dentry.filetype) {
    case FT_RTC:
      pcb->fds[fdIndex].jumptable = &rtc_fops;
      break;

    case FT_DIR:
      pcb->fds[fdIndex].jumptable = &dir_fops;
      break;

    case FT_REG:
      pcb->fds[fdIndex].jumptable = &fs_fops;
      break;

    default:
      return -1;
    }

    if (pcb->fds[fdIndex].jumptable->open(filename) == -1) {
      pcb->fds[fdIndex].jumptable = NULL;
      return -1;
    }

    pcb->fds[fdIndex].flags = FD_IN_USE;
    pcb->fds[fdIndex].inode = (dentry.filetype == FT_REG) ? dentry.inode_idx : 0;
    pcb->fds[fdIndex].file_position = 0;
    fdReturnValue = fdIndex;
    break;
  }

  return fdReturnValue;
}

/* close
 * Description: Closes file descriptor
 * Inputs: fd -- file descriptor
 * Outputs: none
 * Return Value: if fails return -1, if success return 0
 * Function: Checks if file descriptor is valid, if valid close it
 */
i32 close(i32 fd) {
  /* If file descriptor is invalid, fail */
  if (fd < 2 || fd >= FD_CNT)
    return -1;

  Pcb* pcb = get_current_pcb();
  /* If pcb is null, file descriptor not in use, and ... */
  if (!pcb || ((pcb->fds[fd].flags & FD_IN_USE) == FD_NOT_IN_USE) || !pcb->fds[fd].jumptable)
    return -1;

  /* Close file */
  pcb->fds[fd].flags = 0;
  pcb->fds[fd].file_position = 0;
  pcb->fds[fd].inode = 0;
  pcb->fds[fd].jumptable = NULL;
  return 0;
}

/* getargs
 * Description:
 * Inputs: buf -- UNUSED
 *         nbytes -- UNUSED
 * Outputs: none
 * Return Value: if fails return -1, if success return 0
 * Function: currently unimplemented
 */
i32 getargs(u8* UNUSED(buf), i32 UNUSED(nbytes)) { NIMPL; }

/* vidmap
 * Description:
 * Inputs: screen_start -- UNUSED
 * Outputs: none
 * Return Value: if fails return -1, if success return 0
 * Function: currently unimplemented
 */
i32 vidmap(u8** UNUSED(screen_start)) { NIMPL; }

i32 read_failure(i32 UNUSED(fd), void* UNUSED(buf), i32 UNUSED(nbytes)) { return -1; }
i32 write_failure(i32 UNUSED(fd), void const* UNUSED(buf), i32 UNUSED(nbytes)) { return -1; }

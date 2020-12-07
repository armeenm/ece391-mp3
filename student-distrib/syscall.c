#include "syscall.h"
#include "fs.h"
#include "lib.h"
#include "rtc.h"
#include "terminal_driver.h"
#include "util.h"
#include "x86_desc.h"

typedef i32 (*Syscall)(u32 arg1, u32 arg2, u32 arg3);

FileOps const std_in_fops = {terminal_open, terminal_close, terminal_read, write_failure};
FileOps const std_out_fops = {terminal_open, terminal_close, read_failure, terminal_write};
FileOps const rtc_fops = {rtc_open, rtc_close, rtc_read, rtc_write};
FileOps const fs_fops = {file_open, file_close, file_read, file_write};
FileOps const dir_fops = {dir_open, dir_close, dir_read, dir_write};

u8 const elf_header[] = {0x7F, 'E', 'L', 'F'};
Syscall const syscalls[] = {
    (Syscall)halt,  (Syscall)execute, (Syscall)read,   (Syscall)write,       (Syscall)open,
    (Syscall)close, (Syscall)getargs, (Syscall)vidmap, (Syscall)set_handler, (Syscall)sigreturn};

u8 procs = 0x0;
u8 running_pid = 0;

static u8 program_exception_occured = 0;

/* irqh_syscall
 * Description: IRQ Handler for system calls
 * Inputs: type -- Type of syscall
 *         arg1 -- Argument 1
 *         arg2 -- Argument 2
 *         arg3 -- Argument 3
 * Outputs: none
 * Return Value: -1 if fails
 * Function: Uses a jump table to call function given type and passed arguments
 */
i32 irqh_syscall(void) {
  SyscallType type;
  Syscall func;
  u32 arg1, arg2, arg3;

  /* Read the syscall type and arguments */
  asm volatile("" : "=a"(type), "=b"(arg1), "=c"(arg2), "=d"(arg3));

  /* Ensure the type is within bounds */
  if (!(u32)type || (u32)type > (u32)SYSC_SIGRETURN)
    return -1;

  /* Get the function from the jump table, do NULL check */
  func = syscalls[(u32)type - 1];
  if (!func)
    return -1;

  /* Call it */
  return func(arg1, arg2, arg3);

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
Pcb* get_pcb(u8 proc) { return (Pcb*)(MB8 - (proc + 1) * KB8); }

/* get_current_pcb
 * Description: ^
 * Inputs: none
 * Outputs: none
 * Return Value: current PCB ptr
 * Function:
 */
Pcb* get_current_pcb(void) {
  cli();
  Pcb* tmp = get_pcb(running_pid);
  sti();
  return tmp;
}

/* set_program_exception
 * Description: setter for the halt function, this function set's a static boolean to indicate if a
 * exception was thrown and the proc was killed by the shell Inputs: u8 bval: boolean to enable or
 * disable this 256 load to EAX instead of halt(status) Outputs: none Return Value: none Function:
 * set program exception occured
 */
void set_program_exception(u8 val) { program_exception_occured = val; }

/* halt
 * Description: Halts a program
 * Inputs: status -- exit code of program
 * Outputs: none
 * Return Value: none
 * Function: Halts a program given a status
 */
i32 halt(u8 const status) {
  u32 i;

  cli();

  Pcb* const pcb = get_current_pcb();

  if (pcb && pcb->parent_pcb)
    pcb->parent_pcb->child_pcb = NULL;

  /* If we're the "parent process" of the OS (pid == 0, shell) don't halt it */
  /* Close all FDs for the current process */
  for (i = 0; i < FD_CNT; ++i)
    close(i);

  terminal* term = get_running_terminal();

  /* Marks pid as completed */
  procs &= ~(FIRST_PID >> pcb->pid);

  if (pcb->parent_pid == -1) {
    // if the process is a terminal, we mark it as not running, so it's id can be taken in execute
    tss.esp0 = MB8 - KB8 * (term->pid + 1) - ADDRESS_SIZE;
    terminals[current_terminal].running = 0;
    // Load KSP/KPB from last execute call
    asm volatile("mov %0, %%esp;"
                 "mov %1, %%ebp;"
                 :
                 : "g"(pcb->parent_ksp), "g"(pcb->parent_kbp)
                 : "esp", "ebp");
    sti();
    execute((u8*)"shell");
  } else {
    // if a program exception occured, we ignore the halt status and return 256 to eax
    pcb->child_return = program_exception_occured ? PROCESS_KILLED_BY_EXCEPTION : status;
    set_program_exception(0);

    /* There is a parent, we need to switch contexts to the parent */
    remove_task_pgdir(pcb->pid);
    make_task_pgdir(pcb->parent_pid);

    tss.esp0 = MB8 - KB8 * (pcb->parent_pid + 1) - ADDRESS_SIZE;
    running_pid = pcb->parent_pid;
  }
  /* Uncheck vidmap for terminal */
  term->vidmap = 0;

  sti();

  /* Moves base pointers to parent */
  asm volatile("mov %0, %%esp;"
               "mov %1, %%ebp;"
               "mov %2, %%eax;"
               "leave;"
               "ret;"
               :
               : "g"(pcb->parent_ksp), "g"(pcb->parent_kbp), "g"(pcb->child_return)
               : "eax", "esp", "ebp");

  return -1;
}

/* execute
 * Description: Executes system calls
 * Inputs: ucmd -- system call to execute
 * Outputs: none
 * Return Value: if fails return -1, if success return 0
 * Function: Checks cmd validity, if valid executes a system call given as ucmd input
 */
i32 execute(u8 const* const ucmd) {
  cli();

  i8 cmd[ARGS_SIZE];
  Pcb* const parent = get_current_pcb();
  DirEntry dentry;
  u32 entry;
  u8 header[ELF_HEADER_SIZE];
  u8 mask;
  u32 i, j, l;

  /* If our input is null, fail */
  if (!ucmd) {
    sti();
    return -1;
  }

  /* Copy the input argument neglecting leading spaces */
  memset(cmd, 0, ARGS_SIZE);
  // Remove excess spaces and copy to cmd buffer
  for (i = strnonspace((i8 const*)ucmd), l = strlen((i8 const*)ucmd), j = 0; i < l; i++, j++) {
    if (ucmd[i] == ' ') {
      cmd[j] = '\0';
      while (i + 1 < l && ucmd[i + 1] == ' ') {
        i++;
      }
      // once we run into a command with a space, grab stuff after it
      strcpy(cmd + j, (i8 const*)ucmd + i);
      break;
    } else {
      cmd[j] = ucmd[i];
    }
  }
  cmd[j] = '\0';

  /* If directory entry read fails, fail */
  if (read_dentry_by_name((u8*)cmd, &dentry)) {
    sti();
    return -1;
  }

  /* If the data read isn't the size of the data, fail */
  if (read_data(dentry.inode_idx, 0, header, sizeof(header)) != sizeof(header)) {
    sti();
    return -1;
  }

  /* If file is an invalid executable, fail */
  for (i = 0; i < sizeof(header); ++i)
    if (header[i] != elf_header[i]) {
      sti();
      return -1;
    }

  for (i = 0, mask = FIRST_PID; i < MAX_PID_COUNT; ++i, mask >>= 1)
    if (!(mask & procs)) {
      procs |= mask;
      running_pid = i;
      /* Go away, this code's great */
      goto cont;
    }

  sti();
  return -1;

cont:
  /* If bytes read from file isn't the same as the size of the file, fail */
  if (file_read_name(cmd, (u8*)&entry, ENTRY_POINT_OFFSET, sizeof(entry)) != sizeof(entry)) {
    sti();
    return -1;
  }

  /* If making page directory fails, fail */
  if (make_task_pgdir(running_pid)) {
    sti();
    return -1;
  }

  /* If file read was unsuccessful after making pgdir, fail */
  if (file_read_name(cmd, (u8*)LOAD_ADDR, 0, 0) == -1) {
    sti();
    return -1;
  }

  {
    Pcb* const pcb = get_current_pcb();

    u32 esp, ebp;

    /* Copy the ESP and EBP for the child process to return to parent */
    asm volatile("mov %%esp, %0;"
                 "mov %%ebp, %1;"
                 : "=g"(esp), "=g"(ebp));

    /* Sets first two file descriptors to stdin and stdout, and that they're in use */
    for (i = 0; i < FD_START; ++i) {
      pcb->fds[i].jumptable = (i == 0) ? &std_in_fops : &std_out_fops;
      pcb->fds[i].flags = FD_IN_USE;
      pcb->fds[i].inode = 0;
      pcb->fds[i].file_position = 0;
    }

    /* Sets the rest of the file descriptors to NULL and not in use */
    for (i = FD_START; i < FD_CNT; ++i) {
      pcb->fds[i].jumptable = NULL;
      pcb->fds[i].flags = FD_NOT_IN_USE;
      pcb->fds[i].inode = 0;
      pcb->fds[i].file_position = 0;
    }

    /* Setup argv to point to sections of the raw_argv string to seperate args */
    memcpy(pcb->raw_argv, cmd, ARGS_SIZE);
    pcb->argv[0] = pcb->raw_argv;
    // set the remaining section of the argument
    pcb->argv[1] = pcb->raw_argv + strlen(pcb->raw_argv) + 1;

    /* Set the pcb pid and it's parents ksp and kbp */
    pcb->pid = running_pid;
    pcb->parent_ksp = esp;
    pcb->parent_kbp = ebp;

    /* Create a new terminal if needed */
    terminal* term;
    if (terminals[current_terminal].running == 1) {
      term = &terminals[current_terminal];
    } else {
      term = new_terminal(running_pid);
    }
    /* Set child pcb to null and set parent pid based on the terminals pid */
    pcb->child_pcb = NULL;
    pcb->parent_pid =
        (running_pid == term->pid) ? -1 : (i32)parent->pid; /* Special case 1st proc */

    /* If the parent exists set it's child to the new pcb, otherwise the parent pcb is null */
    pcb->parent_pcb = parent;
    if (pcb->parent_pid != -1) {
      parent->child_pcb = pcb;
    } else {
      pcb->parent_pcb = NULL;
    }

    /* New KSP */
    tss.esp0 = MB8 - KB8 * (running_pid + 1) - ADDRESS_SIZE;

    sti();

    /* Enter into userspace */
    uspace(entry);

    /* After return from userspace return the appropriate value */
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
  if (!buf || fd < 0 || fd >= FD_CNT || nbytes < 0 || !pcb ||
      ((pcb->fds[fd].flags & FD_IN_USE) == FD_NOT_IN_USE) || !pcb->fds[fd].jumptable)
    return -1;

  sti();

  return pcb->fds[fd].jumptable->read(fd, buf, nbytes);
}

/* write
 * Description: Writes n bytes to file descriptor
 * Inputs: fd -- file descriptor
 *         buf -- buffer to fill with n bytes
 *         nbytes -- number of bytes
 * Outputs: none
 * Return Value: if fails return -1, if success return 0
 * Function: Checks if inputs are valid, if so then writes bytes to buffer
 */
i32 write(i32 fd, void const* buf, i32 nbytes) {
  /* If buffer is NULL, fd is invalid value, or nbytes is invalid value, fail */
  if (!buf || fd < 0 || fd >= FD_CNT || nbytes < 0)
    return -1;

  Pcb* pcb = get_current_pcb();
  /* If pcb is null, file descriptor not in use, or page table is null, fail */
  if (!pcb || ((pcb->fds[fd].flags & FD_IN_USE) == FD_NOT_IN_USE) || !pcb->fds[fd].jumptable)
    return -1;

  return pcb->fds[fd].jumptable->write(fd, buf, nbytes);
}

/* open
 * Description: Opens a file to read
 * Inputs: filename -- name of the file to open
 * Outputs: none
 * Return Value: if fails return -1, if success return 0
 * Function: Checks for invalid inputs, if valid then open filename
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

  /* Start from index 2 because 0 and 1 are already in-use by std I/O */
  for (fdIndex = 2; fdIndex < FD_CNT; ++fdIndex) {
    if (pcb->fds[fdIndex].flags & FD_IN_USE)
      continue;

    /* Check which file type we have and set appropriate file operation address */
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

    /* If we fail to open, set jumptable address to NULL */
    if (pcb->fds[fdIndex].jumptable->open(filename) == -1) {
      pcb->fds[fdIndex].jumptable = NULL;
      return -1;
    }

    /* Set file descriptor flags etc */
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
  if (fd < FD_START || fd >= FD_CNT)
    return -1;

  Pcb* pcb = get_current_pcb();
  /* If pcb is null, file descriptor not in use, or page table is null, fail */
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
 * Description: Gets arguments from the process execution and returns them
 * Inputs: buf -- buffer to write args to
 *         nbytes -- number of bytes to write to buffer (uses min(nbytes, argv_length))
 * Outputs: none
 * Return Value: if fails return -1, if success return 0
 * Function: Writes the arguments from the function call to the buffer.
 */
i32 getargs(u8* const buf, i32 const nbytes) {
  /* Gets the pcb */
  Pcb const* const pcb = get_current_pcb();
  /* Check to see if pcb and buffer are valid, also check
     that we're not pointing to an empty string*/
  if (!buf || nbytes < 0 || !pcb->argv[1] || !pcb->argv[1][0])
    return -1;
  /* Copy data into buffer */
  memcpy(buf, pcb->argv[1], MIN(strlen(pcb->argv[1]) + 1, (u32)nbytes));

  return 0;
}

/* vidmap
 * Description: Maps a pointer in user space to vga memory
 * Inputs: screen_start -- Pointer to video memory
 * Outputs: none
 * Return Value: if fails return -1, if success return 0
 * Function: Maps pointer to video memory and sets up a PTE for the virtual address
 * to a physical address
 */
i32 vidmap(u8** screen_start) {
  /* Get pcb so we can get the pid */
  Pcb* pcb = get_current_pcb();
  /* Check to see if pcb and screen_start pointer are valid, return -1 on fail
   * (Also see if screen_start is < 8MB (not in kernal space)
   */
  if (!screen_start || screen_start < (u8**)(PG_4M_START * 2) || !pcb)
    return -1;
  /* Set screen_start to 128MB + 4MB * 8 Process = 160MB */
  *screen_start = (u8*)(PG_4M_START * (ELF_LOAD_PG + NUM_PROC));
  /* Map to video memory and return condition */
  terminal* term = get_running_terminal();
  if (!term)
    return -1;
  /*
   * If the terminal is displayed set physical address to
   * video memory. Otherwise it needs to be set to the terminal video_buffer
   */
  u32 video_addr;
  if (term->id == current_terminal) {
    video_addr = (u32)VIDEO;
  } else {
    video_addr = (u32)term->vid_mem_buf;
  }
  term->vidmap = 1;
  /* Map screen start pointer to appropriate video address */
  return map_vid_mem(pcb->pid, (u32)(*screen_start), video_addr);
}

/* set_handler
 * Description: Changes the default action for a signal for a particular signal
 * Inputs: signum -- signal to change handler for
 *         handler_address -- address of user function to go to
 * Outputs: none
 * Return Value: if fails return -1, if success return 0
 * Function: Changes the default action for a signal for a particular signal
 */
i32 set_handler(u32 UNUSED(signum), void* UNUSED(handler_address)) { NIMPL; }

/* sigreturn
 * Description: Copies hardware context on the user-level stack to the processor
 * Inputs: none
 * Outputs: none
 * Return Value: if fails return -1, if success return 0
 * Function: Copies hardware context on the user-level stack to the processor
 */
i32 sigreturn(void) { NIMPL; }

void set_pid(u8 pid) { running_pid = pid; }

i32 read_failure(i32 UNUSED(fd), void* UNUSED(buf), i32 UNUSED(nbytes)) { return -1; }
i32 write_failure(i32 UNUSED(fd), void const* UNUSED(buf), i32 UNUSED(nbytes)) { return -1; }

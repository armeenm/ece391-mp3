#include "terminal_driver.h"
#include "keyboard.h"
#include "syscall.h"
#include "lib.h"
#include "x86_desc.h"
#include "pit.h"

/* terminal_read
 * Description: Read input from the terminal
 * Inputs: buf - buf to write line buf to
 *         nbytes - number of bytes to read
 * Outputs: none
 * Return Value: number of bytes read
 * Function: To read from the line buf
 */
i32 terminal_read(i32 UNUSED(fd), void* const buf, i32 const nbytes) {
  /* Get line buf and return size of bytes read */
  return get_line_buf((char*)buf, nbytes);
}

/* terminal_write
 * Description: Write input to the terminal
 * Inputs: buf - Buffer of chars to write to line buf
 *         nbytes - number of bytes to write
 * Outputs: none
 * Return Value: number of bytes written
 * Function: To write to the line buf
 */
i32 terminal_write(i32 UNUSED(fd), void const* const buf, i32 const nbytes) {
  /* Typecast buf to a char* */
  terminal* term = get_running_terminal();
  char const* const cbuf = (char const*)buf;
  i32 i, bytes_written = 0;
  // if((term = &terminals[current_terminal]) == get_running_terminal())
  //   printf("terminal pid is %d\n", term->pid);
  /* If params are invalid return -1 */
  if (nbytes <= 0 || !buf || !term)
    return -1;

  for (i = 0; i < nbytes; ++i) {
    /* Write to screen */
    terminal_putc(term->id, cbuf[i]);
    ++bytes_written;
  }

  /* Return bytes written */
  return bytes_written;
}

/* terminal_open
 * Description: Opens the terminal driver
 * Inputs: none
 * Outputs: none
 * Return Value: 0
 * Function: Opens the terminal driver and allocates any memory it needs to
 */
i32 terminal_open(const u8* UNUSED(filename)) { return 0; }

/* terminal_close
 * Description: Close the terminal driver
 * Inputs: none
 * Outputs: none
 * Return Value: 0
 * Function: Closes the terminal driver and deallocates any memory it needs to.
 */
i32 terminal_close(i32 UNUSED(fd)) { return 0; }

/* get_terminal_from_pid
 * Description: Get pointer to a terminal from a pid
 * Inputs: u32 pid -- pid to find terminal from
 * Outputs: none
 * Return Value: terminal* -- pointer to terminal from pid
 * Function: Finds a terminal from a pid, returns null if not found
 */
terminal* get_terminal_from_pid(u32 pid) {
  int i = 0;
  /* Iterate through all terminals to find pid that matches */
  for(i = 0; i < TERMINAL_NUM; i++) {
    if(terminals[i].pid == pid)
      return &terminals[i];
  }
  /* Returns NULL is terminal is not found */
  return NULL;
}


/* get_running_terminal
 * Description: Gets the terminal that the scheduler is running
 * Inputs: none
 * Outputs: none
 * Return Value: terminal* -- pointer to the running terminal
 * Function: Finds a terminal from the current pcb and iterates through it's parents
 */
terminal* get_running_terminal(void) {
  Pcb* pcb = get_current_pcb();
  terminal* term;
  /* If the current pcb is the terminal return it */
  if((term = get_terminal_from_pid(pcb->pid)))
    return term;
  /* If it is not the current termial and there is no parent there is no terminal */
  if(!(pcb->parent_pcb))
    return NULL;
  /* Iterate through the parents of the pcb and see if any of them are the terminal */
  while(pcb->parent_pcb != NULL || pcb->parent_pcb->pid != pcb->pid) {
    if((term = get_terminal_from_pid(pcb->parent_pid)))
      return term;
    /* Go to parent pcb */
    pcb = pcb->parent_pcb;
  }
  /* If no terminal is running on the current pcb return null */
  return NULL;
}

/* init_terminals
 * Description: Initialize all the terminals
 * Inputs: none
 * Outputs: Sets up terminals and their video memory
 * Return Value: none
 * Function: Initialize all the terminal data structures and sets
 * the appropriate memory locations to 0.
 */
void init_terminals(void) {
  int i;
  terminal* term;
  /* For each terminal set everything to 0 */
  for(i = 0; i < TERMINAL_NUM; i++) {
    term = &terminals[i];
    term->screen_x = 0;
    term->screen_y = 0;
    term->read_flag = 0;
    term->line_buf_index = 0;
    term->status = TASK_NOT_RUNNING;
    term->running = 0;
      
    // Setup RTC details here since each terminal owns it's own RTC config
    term->rtc.real_freq = RTC_DEFAULT_REAL_FREQ;
    term->rtc.virt_freq = RTC_DEFAULT_VIRT_FREQ;
    term->rtc.int_count = 0;
    term->rtc.flag = 0;
    /* Set the video mem buffer to be next to the physical video memory */
    term->vid_mem_buf = (u8 *)(VIDEO + (KB4 * (i + 1)));
    term->id = (u8)i;
    term->vidmap = 0;
    /* Set line buffer to 0 */
    int j;
    for(j = 0; j < LINE_BUFFER_SIZE; j++)
      term->line_buf[j] = 0;
    if(i == 0) {
      /* If it is the currrent terminal then set the screen_pos to wherever
       * bootup leaves us
       */
      term->screen_x = get_screen_x();
      term->screen_y = get_screen_y();
    }
    else {
      /* clear the video buffer for non-zero terminal ids */
      for(j = 0; j < NUM_ROWS * NUM_COLS * 2; j+=2) {
        term->vid_mem_buf[j] = ' ';
        term->vid_mem_buf[j + 1] = ATTRIB;
      }
        
    }
  }
  /* Initialize terminal 0 to be running for the PIT */
  terminals[0].running = 1;
  terminals[0].status = TASK_RUNNING;
  /* Set current terminal */
  current_terminal = 0;
  /* Start shell */
  execute((u8*)"shell");
}

/* switch_terminal
 * Description: Switches between two terminals
 * Inputs: u8 term_num -- terminal to switch to
 * Outputs: Switches video memory and changes the currrent terminal
 * Return Value: none
 * Function: Switches video memory and changes the currrent terminal
 * as well as the terminal status for the pit
 */
void switch_terminal(u8 term_num) {
  /* Ensure valid input and that it is not the current terminal */
  if(current_terminal == term_num || term_num >= TERMINAL_NUM)
    return;
  /* Critical section on code. Must not be interrupted */
  cli();
  /* Restore new terminals properties and change the current terminal */
  restore_terminal(term_num);
  current_terminal = term_num;
  terminals[term_num].status = TASK_RUNNING;
  sti();
}

/* restore_terminal
 * Description: Changes out video memory of old terminal and sets properties
 * such as screen x/y.
 * Inputs: u8 term_num -- terminal to switch to
 * Outputs: Changes screen position, video memory, and virtual memory address
 * Return Value: none
 * Function: Sets the appopriate virtual address for video, copies video memory over
 */
void restore_terminal(u8 term_num) {
  /* Check if terminal is valid */
  terminal* term;
  if(term_num >= TERMINAL_NUM)
    return;

  /* Set previous terminal screen position to current screen pos */
  terminal* prev_term = &terminals[current_terminal];
  prev_term->screen_x = get_screen_x();
  prev_term->screen_y = get_screen_y();

  /* Set screen position to next terminals screen position */
  term = &terminals[term_num];
  set_screen_xy(term->screen_x, term->screen_y);

  /* map video memory to be video memory to ensure there are no virtual addresses */
  u32 pid = get_current_pcb()->pid;
  map_vid_mem(pid, (u32)VIDEO, (u32)VIDEO);

  /* 
   * Copy video memory into prev_term buffer and copies the new terminals
   * video memory into the physical video memory
   */
  memcpy(prev_term->vid_mem_buf, (u8*)VIDEO, NUM_COLS * NUM_ROWS * 2);
  memcpy((u8*)VIDEO, term->vid_mem_buf, NUM_COLS * NUM_ROWS * 2);

  /* Until the next process in the scheduler happens write to the previous vid buf */
  map_vid_mem(pid, (u32)VIDEO, (u32)prev_term->vid_mem_buf);
}

/* new_terminal
 * Description: Makes a terminal availible for use and sets its pid
 * Inputs: u8 pid -- pid to set to terminal
 * Outputs: none
 * Return Value: terminal* -- running terminal created by new_terminal
 * Function: Finds and returns the first availible terminal
 */
terminal* new_terminal(u8 pid) {
  int i;
  /* Iterate through all terminals */
  for(i = 0; i < TERMINAL_NUM; i++) {
    if(terminals[i].running)
      continue;
    /* If terminal is not running then it is availible. Set it's pid and running */
    terminals[i].pid = pid;
    terminals[i].running = 1;
    return &terminals[i];
  }
  /* If there are no availible terminals return null */
  return NULL;
}

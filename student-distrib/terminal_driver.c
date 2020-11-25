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
  terminal* term = get_current_terminal();
  char const* const cbuf = (char const*)buf;
  i32 i, bytes_written = 0;
  // if((term = &terminals[current_terminal]) == get_current_terminal())
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


terminal* get_terminal_from_pid(u32 pid) {
  int i = 0;
  for(i = 0; i < TERMINAL_NUM; i++) {
    if(terminals[i].pid == pid)
      return &terminals[i];
  }
  return NULL;
}

terminal* get_current_terminal(void) {
  Pcb* pcb = get_current_pcb();
  terminal* term;
  if((term = get_terminal_from_pid(pcb->pid)))
    return term;
  if(!(pcb->parent_pcb))
    return NULL;
  while(pcb->parent_pcb != NULL || pcb->parent_pcb->pid != pcb->pid) {
    if((term = get_terminal_from_pid(pcb->parent_pid)))
      return term;
    pcb = pcb->parent_pcb;
  }
  return NULL;
}

void init_terminals(void) {
  int i;
  terminal* term;
  for(i = 0; i < TERMINAL_NUM; i++) {
    term = &terminals[i];
    term->screen_x = 0;
    term->screen_y = 0;
    term->read_flag = 0;
    term->line_buf_index = 0;
    term->status = TASK_NOT_RUNNING;
    term->vid_mem_buf = (u8 *)(KB4 * ((VIDEO/KB4) + i + 1));
    term->id = (u8)i;
    int j;
    for(j = 0; j < LINE_BUFFER_SIZE; j++)
      term->line_buf[j] = 0;
    if(i == 0) {
      term->screen_x = get_screen_x();
      term->screen_y = get_screen_y();
    }
    else {
      for(j = 0; j < NUM_ROWS * NUM_COLS * 2; j+=2) {
        term->vid_mem_buf[j] = ' ';
        term->vid_mem_buf[j + 1] = ATTRIB;
      }
        
    }
  }
  terminals[0].running = 1;
  terminals[0].status = TASK_RUNNING;
  current_terminal = 0;
  execute((u8*)"shell");
}

void switch_terminal(u8 term_num) {
  u32 esp, ebp;
  if(current_terminal == term_num || term_num >= TERMINAL_NUM)
    return;

  cli();
  restore_terminal(term_num);
  current_terminal = term_num;
  terminals[term_num].status = TASK_RUNNING;
  sti();
}


void restore_terminal(u8 term_num) {
  terminal term;
  if(term_num >= TERMINAL_NUM)
    return;

  terminal* prev_term = &terminals[current_terminal];
  prev_term->screen_x = get_screen_x();
  prev_term->screen_y = get_screen_y();

  term = terminals[term_num];
  set_screen_xy(term.screen_x, term.screen_y);

  u32 pid = get_current_pcb()->pid;
  map_vid_mem(pid, (u32)VIDEO, (u32)VIDEO);

  memcpy(prev_term->vid_mem_buf, (u8*)VIDEO, NUM_COLS * NUM_ROWS * 2);
  memcpy((u8*)VIDEO, term.vid_mem_buf, NUM_COLS * NUM_ROWS * 2);

  map_vid_mem(pid, (u32)VIDEO, (u32)VIDEO);

  /* Map term_num to physical memory and current_terminal to virtual memory */


/* TODO:
 * What do I do if the current terminal is not running??
 * Also need to pay attention to flushing tlb with map_vid_mem
 */

  /* Maps the address of term_num to VIDEO */
  // IFF term is running
  // vidmap(&(term.vid_mem_buf));

  /* maps video for the "current" terminal to address of its buffer */
  // map_vid_mem(prev_term->pid, (u32)VIDEO, &(prev_term->vid_mem_buf));

  // if(term.running) {
  //   Pcb* terminal_pcb = get_pcb(term.pid);
  //   while(terminal_pcb->child_pcb) {
  //     terminal_pcb = terminal_pcb->child_pcb;
  //   }
  //   map_vid_mem(terminal_pcb->pid, (u32)VIDEO, (u32)VIDEO);
  // }
    
  
}


terminal* new_terminal(u8 pid) {
  int i;
  for(i = 0; i < TERMINAL_NUM; i++) {
    if(terminals[i].running)
      continue;
    terminals[i].pid = pid;
    return &terminals[i];
  }
  return NULL;
}
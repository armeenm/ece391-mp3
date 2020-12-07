#include "pit.h"
#include "debug.h"
#include "i8259.h"
#include "syscall.h"
#include "terminal_driver.h"
#include "x86_desc.h"

u8 current_schedule, schedule_counter;

void scheduler_vidmap(u8 num_term, u32 pid);

/* init_pit
 * Description: Initialize the PIT
 * Inputs: none
 * Outputs: none
 * Return Value: none
 * Function: Sets the PIT frequency so that it interrupts at a consistant rate
 * Reference: https://wiki.osdev.org/PIT#Mode_0_.E2.80.93_Interrupt_On_Terminal_Count
 */
void init_pit(void) {

  /* Get reload value (1193182 / reload_value HZ) */
  u16 frequency = (PIT_FREQ * SCHEDULE_TIME / 1000);

  /* Ensure that schedule time is indeed within [10, 50]ms */
  ASSERT(SCHEDULE_TIME >= 10 && SCHEDULE_TIME <= 50);
  /* Enable irq for the pit */
  enable_irq(PIT_IRQ);

  /* Mask high and low bits of frequency */
  u8 low = (u8)(frequency & 0x00FF);
  u8 high = (u8)(frequency >> 8);

  /* Set PIT Modes, written to the pit register */
  outb(PIT_SET_CHANNEL_0 | PIT_SET_ACCESS_MODE_3 | PIT_SET_MODE_RATE | PIT_SET_BCD_MODE_0,
       PIT_MODE_REGISTER);
  /* Write data to the pit */
  outb(low, PIT_CHANNEL_0);
  outb(high, PIT_CHANNEL_0);

  /* Initialize schedule */
  current_schedule = 0;
  schedule_counter = 0;
}

/* irqh_pit
 * Description: pit interrupt handler -- Executes next program in the schedule
 * Inputs: none
 * Outputs: none
 * Return Value: none
 * Function: Iterates through scheduler to find next program to run
 */
void irqh_pit(void) {
  u32 esp, ebp;
  /* Iterates through the terminals until a running one is found */
  do {
    schedule_counter++;
    schedule_counter %= TERMINAL_NUM;
  } while (terminals[schedule_counter].status != TASK_RUNNING);

  /* If this schedule is the current schedule we can continue to run it */
  if (schedule_counter == current_schedule) {
    send_eoi(PIT_IRQ);
    return;
  }
  /* Set the current schedule to the schedule_counter */
  current_schedule = schedule_counter;

  Pcb* prev_pcb = get_current_pcb();

  /* Save the ESP and EBP so this process is still reachable */
  asm volatile("mov %%esp, %0;"
               "mov %%ebp, %1;"
               : "=g"(esp), "=g"(ebp));

  /* Save esp and ebp to the ksp and kbp */
  prev_pcb->ksp = esp;
  prev_pcb->kbp = ebp;

  if (terminals[current_schedule].running == 1) {
    /* If the terminal is running get the next pcb */
    Pcb* next_pcb = get_pcb(terminals[current_schedule].pid);
    /* find the lowest child pcb */
    while (next_pcb->child_pcb) {
      next_pcb = next_pcb->child_pcb;
    }
    /* Setup the TSS to switch to the next pid and set the running pid*/
    tss.esp0 = MB8 - KB8 * (next_pcb->pid + 1) - ADDRESS_SIZE;
    set_pid(next_pcb->pid);

    if (terminals[current_schedule].vidmap)
      scheduler_vidmap(current_schedule, next_pcb->pid);

    /* If the terminal is the current one map video memory to the physical address.
     * Otherwise it should not be displayed and set it to the address of the buffer */
    if (current_schedule == current_terminal) {
      map_vid_mem(next_pcb->pid, (u32)VIDEO, (u32)VIDEO);
    } else {
      map_vid_mem(next_pcb->pid, (u32)VIDEO, (u32)(terminals[current_schedule].vid_mem_buf));
    }

    /* Flush the tlb and end the interrupt */
    flush_tlb();
    send_eoi(PIT_IRQ);

    /* Switch to the next program in the scheduler to run */
    asm volatile("mov %0, %%esp;"
                 "mov %1, %%ebp;"
                 "leave;"
                 "ret;"
                 :
                 : "g"(next_pcb->ksp), "g"(next_pcb->kbp)
                 : "esp", "ebp");
  } else {
    /* If the terminal is not running end the interrupt and start the shell */
    send_eoi(PIT_IRQ);
    execute((u8*)"shell");
  }
}

/* get_current_schedule
 * Description: Gets the current schedule
 * Inputs: none
 * Outputs: none
 * Return Value: current_schedule
 * Function: Returns the current schedule
 */
u8 get_current_schedule(void) { return current_schedule; }

void scheduler_vidmap(u8 num_term, u32 pid) {
  /* Make sure vidmap goes to virtual memorry in background */
  u8* screen_start = (u8*)(PG_4M_START * (ELF_LOAD_PG + NUM_PROC));
  /*
   * If the terminal is displayed set physical address to
   * video memory. Otherwise it needs to be set to the terminal video_buffer
   */
  terminal* term = &terminals[num_term];
  u32 video_addr;
  if (term->id == current_terminal) {
    video_addr = (u32)VIDEO;
  } else {
    video_addr = (u32)term->vid_mem_buf;
  }
  /* Map screen start pointer to appropriate video address */
  map_vid_mem(pid, (u32)(screen_start), video_addr);
}

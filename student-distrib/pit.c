#include "pit.h"
#include "i8259.h"
#include "debug.h"
#include "terminal_driver.h"
#include "syscall.h"
#include "x86_desc.h"

u8 current_schedule, schedule_counter;

/* https://wiki.osdev.org/PIT#Mode_0_.E2.80.93_Interrupt_On_Terminal_Count */
void init_pit(void) {
    
    /* Get reload value (1193182 / reload_value HZ) */
    u16 frequency =  (PIT_FREQ * SCHEDULE_TIME / 1000);

    ASSERT(SCHEDULE_TIME >= 10 && SCHEDULE_TIME <= 50);
    enable_irq(PIT_IRQ);
    u8 low = (u8)(frequency & 0x00FF);
    u8 high = (u8)(frequency >> 8);

    outb(PIT_SET_CHANNEL_0 | PIT_SET_ACCESS_MODE_3 | PIT_SET_MODE_RATE | PIT_SET_BCD_MODE_0, PIT_MODE_REGISTER);
    outb(low,PIT_CHANNEL_0);
    outb(high,PIT_CHANNEL_0);

    current_schedule = 0;
}


void irqh_pit(void) {
    u32 esp, ebp;
    do {
        schedule_counter++;
        schedule_counter %= TERMINAL_NUM;
    } while(terminals[schedule_counter].status != TASK_RUNNING);

    if(schedule_counter == current_schedule) {
         send_eoi(PIT_IRQ);
         return;
    }
        

    current_schedule = schedule_counter;

   
    /* TODO SWITCH TASKS HERE:*/

    Pcb* prev_pcb = get_current_pcb();
   /* Copy the ESP and EBP for the child process to return to parent */
    asm volatile("mov %%esp, %0;"
                 "mov %%ebp, %1;"
                 : "=g"(esp), "=g"(ebp));
    prev_pcb->ksp = esp;
    prev_pcb->kbp = ebp;
  if(terminals[current_schedule].running == 1)
  {
    Pcb* next_pcb = get_pcb(terminals[current_schedule].pid);
    while(next_pcb->child_pcb) {
      next_pcb = next_pcb->child_pcb;
    }
    tss.esp0 = MB8 - KB8 * (next_pcb->pid + 1) - ADDRESS_SIZE;

    // terminal* prev_term = get_terminal_from_pid(prev_pcb->pid);
    // if(&terminals[current_terminal] == prev_term) {
    //   map_vid_mem(prev_pcb->pid, (u32)VIDEO, (u32)VIDEO);
    // } else {
    //   map_vid_mem(prev_pcb->pid, (u32)VIDEO, (u32)(prev_term->vid_mem_buf));
    // }

    set_pid(next_pcb->pid);
  
     if(current_schedule == current_terminal) {
       map_vid_mem(next_pcb->pid, (u32)VIDEO, (u32)VIDEO);
     } else {
       map_vid_mem(next_pcb->pid, (u32)VIDEO, (u32)(terminals[current_schedule].vid_mem_buf));
     }

    
    flush_tlb();
    send_eoi(PIT_IRQ);
    asm volatile("mov %0, %%esp;"
                "mov %1, %%ebp;"
          "leave;"
          "ret;"
                :
                : "g"(next_pcb->ksp), "g"(next_pcb->kbp)
                : "esp", "ebp");
  }
  else {
    send_eoi(PIT_IRQ);
    execute((u8*)"shell");
  }

}

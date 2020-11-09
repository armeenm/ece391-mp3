/* i8259.h - Defines used in interactions with the 8259 interrupt
 * controller
 */

#ifndef I8259_H
#define I8259_H

#include "types.h"

enum {
  /* Ports that each PIC sits on */
  MASTER_8259_PORT = 0x20,
  MASTER_8259_DATA_PORT,
  SLAVE_8259_PORT = 0xA0,
  SLAVE_8259_DATA_PORT,

  /* Initialization control words to init each PIC.
   * See the Intel manuals for details on the meaning
   * of each word */
  ICW1 = 0x11,
  ICW2_MASTER = 0x20,
  ICW2_SLAVE = 0x28,
  ICW3_MASTER = 0x04,
  ICW3_SLAVE = 0x02,
  ICW4 = 0x01,

  SLAVE_IRQ = 0x2,

  /* End-of-interrupt byte.  This gets OR'd with
   * the interrupt number and sent out to the PIC
   * to declare the interrupt finished */
  EOI = 0x60
};

/* Initialize both PICs */
void init_i8259(void);
/* Enable (unmask) the specified IRQ */
void enable_irq(u32 irq_num);
/* Disable (mask) the specified IRQ */
void disable_irq(u32 irq_num);
/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(u32 irq_num);

#endif /* I8259_H */

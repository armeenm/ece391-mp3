/* i8259.c - Functions to interact with the 8259 interrupt controller
 */

#include "i8259.h"
#include "debug.h"
#include "lib.h"
#include "util.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = 0xFF; /* IRQs 0-7  */
uint8_t slave_mask = 0xFF;  /* IRQs 8-15 */

/* Initialize the 8259 PIC
 * See 8259 datasheet for more information
 */
void i8259_init(void) {
  /* ICW1: Start init process */
  outb(ICW1, MASTER_8259_PORT);
  outb(ICW1, SLAVE_8259_PORT);

  /* ICW2: Vector offsets */
  outb(ICW2_MASTER, MASTER_8259_DATA_PORT);
  outb(ICW2_SLAVE, SLAVE_8259_DATA_PORT);

  /* ICW3: Slavery */
  outb(ICW3_MASTER, MASTER_8259_DATA_PORT);
  outb(ICW3_SLAVE, SLAVE_8259_DATA_PORT);

  /* ICW4: Misc. Config
   * - Use 8086/8088 mode
   */
  outb(ICW4, MASTER_8259_DATA_PORT);
  outb(ICW4, SLAVE_8259_DATA_PORT);

  /* Enable IRQ2 */
  enable_irq(ICW3_SLAVE);
}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t const irq_num) {
  ASSERT(irq_num < 16);

  /* Check if IRQ on master or slave */
  if (irq_num < 8) {
    master_mask &= ~(1 << irq_num);
    outb(master_mask, MASTER_8259_DATA_PORT);
  } else {
    slave_mask &= ~(1 << (irq_num - 8));
    outb(slave_mask, SLAVE_8259_DATA_PORT);
  }
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t const irq_num) {
  ASSERT(irq_num < 16);

  /* Check if IRQ on master or slave */
  if (irq_num < 8) {
    master_mask |= 1 << irq_num;
    outb(master_mask, MASTER_8259_DATA_PORT);
  } else {
    slave_mask |= 1 << (irq_num - 8);
    outb(slave_mask, SLAVE_8259_DATA_PORT);
  }
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t const irq_num) {
  ASSERT(irq_num < 16);

  if (irq_num < 8)
    outb(irq_num | EOI, MASTER_8259_PORT);
  else {
    outb((irq_num - 8) | EOI, SLAVE_8259_PORT);

    /* TODO: This may be wrong */
    outb(EOI + ICW3_SLAVE, MASTER_8259_PORT);
  }
}

#include "keyboard.h"
#include "i8259.h"
#include "lib.h"

void init_keyboard(void) { enable_irq(KEYBOARD_IRQ); }

void keyboard_irqh(void) {
  disable_irq(KEYBOARD_IRQ);
  send_eoi(KEYBOARD_IRQ);
  sti();

  printf("Testing...\n");

  enable_irq(KEYBOARD_IRQ);
}

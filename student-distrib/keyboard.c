#include "keyboard.h"
#include "i8259.h"
#include "lib.h"

void init_keyboard(void) { enable_irq(KEYBOARD_IRQ); }

void keyboard_irqh(void) {
  cli();
  disable_irq(KEYBOARD_IRQ);
  send_eoi(KEYBOARD_IRQ);
  sti();

  while (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_OUTBUF_FULL) {
    printf("Keyboard: %d\n", inb(KEYBOARD_DATA_PORT));
  }

  enable_irq(KEYBOARD_IRQ);
}

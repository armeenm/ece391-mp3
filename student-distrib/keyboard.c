#include "keyboard.h"
#include "i8259.h"
#include "lib.h"

void init_keyboard(void) { enable_irq(KEYBOARD_IRQ); }

void irqh_keyboard(void) {
  send_eoi(KEYBOARD_IRQ);
  sti();

  printf("Keyboard event\n");

  if (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_OUTBUF_FULL)
    inb(KEYBOARD_DATA_PORT);

  /* NOTE: Do NOT printf() after the inb(KEYBOARD_DATA_PORT) call!
   * It causes odd processor exceptions...
   */
}

KeyDiff keycode_scs1(SCSet1 const scancode) {
  if (scancode >= SCS1_RELEASED_ESC) {
    KeyDiff const kd = {.pressed = 0, .keycode = scancode - SCS1_RELEASED_ESC + 1};
    return kd;
  } else {
    KeyDiff const kd = {.pressed = 1, .keycode = scancode};
    return kd;
  }
}

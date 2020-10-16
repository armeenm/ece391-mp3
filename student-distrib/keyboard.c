#include "keyboard.h"
#include "i8259.h"
#include "lib.h"

char keycodes[SCS1_PRESSED_F12] = {
    [SCS1_PRESSED_1] = '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 10, '\t'};

void init_keyboard(void) { enable_irq(KEYBOARD_IRQ); }

void irqh_keyboard(void) {
  send_eoi(KEYBOARD_IRQ);
  sti();

  if (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_OUTBUF_FULL)
    handle_keypress(inb(KEYBOARD_DATA_PORT));
}

void handle_keypress(SCSet1 const scancode) {
  if (scancode < SCS1_RELEASED_ESC) {
    char const disp = keycodes[scancode];

    if (disp)
      putc(disp);
  }
}

#include "keyboard.h"
#include "i8259.h"

void init_keyboard(void) { enable_irq(KEYBOARD_IRQ); }

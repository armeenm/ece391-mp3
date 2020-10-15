#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEYBOARD_IRQ 0x1

void init_keyboard(void);
void keyboard_irqh(void);

#endif

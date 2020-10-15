#ifndef KEYBOARD_H
#define KEYBOARD_H

/* Intel 8042 PS/2 Controller */
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

#define KEYBOARD_OUTBUF_FULL 0x1
#define KEYBOARD_INBUF_FULL 0x2
#define KEYBOARD_SYSFLAG 0x4
#define KEYBOARD_CMDSEL 0x8
#define KEYBOARD_TIMEOUT_ERR 0x40
#define KEYBOARD_PARITY_ERR 0x80

#define KEYBOARD_IRQ 0x1

void init_keyboard(void);
void irqh_keyboard(void);

#endif

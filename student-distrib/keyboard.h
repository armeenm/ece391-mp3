#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "keycodes.h"
#include "lib.h"
#include "scancodes.h"
#include "util.h"

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

#define CHAR_BUFFER_SIZE 128
/* pressed = 1 if pressed, 0 if released */
typedef struct KeyDiff {
  uint8_t keycode;
  uint8_t pressed;
} KeyDiff;

static char const keycodes[SCS1_PRESSED_F12] = {[KEY_1] = '1',
                                                '2',
                                                '3',
                                                '4',
                                                '5',
                                                '6',
                                                '7',
                                                '8',
                                                '9',
                                                '0',
                                                '-',
                                                '=',
                                                10,
                                                '\t',
                                                'q',
                                                'w',
                                                'e',
                                                'r',
                                                't',
                                                'y',
                                                'u',
                                                'i',
                                                'o',
                                                'p',
                                                '[',
                                                ']',
                                                '\n',
                                                [KEY_A] = 'a',
                                                's',
                                                'd',
                                                'f',
                                                'g',
                                                'h',
                                                'j',
                                                'k',
                                                'l',
                                                ';',
                                                '\'',
                                                '`',
                                                [KEY_BACKSLASH] = '\\',
                                                'z',
                                                'x',
                                                'c',
                                                'v',
                                                'b',
                                                'n',
                                                'm',
                                                ',',
                                                '.',
                                                '/',
                                                [KEY_KPASTERISK] = '*',
                                                [KEY_SPACE] = ' ',};

void init_keyboard(void);
void irqh_keyboard(void);
void handle_keypress(SCSet1 scancode);
char handle_disp(char disp);
#endif

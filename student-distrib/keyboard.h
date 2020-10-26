#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "keycodes.h"
#include "lib.h"
#include "scancodes.h"
#include "util.h"

/* Intel 8042 PS/2 Controller */
enum {
  KEYBOARD_DATA_PORT = 0x60,
  KEYBOARD_STATUS_PORT = 0x64,

  KEYBOARD_OUTBUF_FULL = 1,
  KEYBOARD_INBUF_FULL = 1 << 1,
  KEYBOARD_SYSFLAG = 1 << 2,
  KEYBOARD_CMDSEL = 1 << 3,
  KEYBOARD_TIMEOUT_ERR = 1 << 6,
  KEYBOARD_PARITY_ERR = 1 << 7,

  KEYBOARD_IRQ = 0x1
};
#define SCS1_UPPERCASE_OFFSET 0x20
#define SCS1_KEYPRESS_RELEASE_OFFSET 0x80
#define LINE_BUFFER_SIZE 128
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
                                                ' ',
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

/* Declare helper functions for keyboard */
void init_keyboard(void);
void irqh_keyboard(void);
void handle_keypress(SCSet1 scancode);
char handle_disp(char disp);
void clear_line_buffer();
int32_t get_line_buffer(char * buffer, int32_t nbytes);
#endif

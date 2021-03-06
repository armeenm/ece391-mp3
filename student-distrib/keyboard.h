#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "keycodes.h"
#include "lib.h"
#include "scancodes.h"
#include "util.h"

i32 terminal_to_switch_to;

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

  KEYBOARD_IRQ = 0x1,

  SCS1_UPPERCASE_OFFSET = 0x20,
  SCS1_KEYPRESS_RELEASE_OFFSET = 0x80,
  LINE_BUFFER_SIZE = 128
};

static char const keycodes[SCS1_PRESSED_F12] = {
    [KEY_1] = '1',
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
    [KEY_SPACE] = ' ',
};

/* Declare helper functions for keyboard */
void init_keyboard(void);
void irqh_keyboard(void);
i32 contains_newline(i8 const* buf, i32 size);
void handle_keypress(SCSet1 scancode);
char handle_disp(char disp);
void clear_line_buf(void);
i32 get_line_buf(i8* buf, i32 nbytes);
i32 capslock_pressed(void);
i32 shift_pressed(void);
i32 is_func_key(void);
i32 alt_pressed(void);

#endif

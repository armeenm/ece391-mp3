#ifndef SCANCODES_H
#define SCANCODES_H

/* By default, the keyboard appears to use Set 1
 * TODO: Add multi-byte scancode support
 */
typedef enum SCSet1 {
  SCS1_PRESSED_ESC = 0x1,
  SCS1_PRESSED_1,
  SCS1_PRESSED_2,
  SCS1_PRESSED_3,
  SCS1_PRESSED_4,
  SCS1_PRESSED_5,
  SCS1_PRESSED_6,
  SCS1_PRESSED_7,
  SCS1_PRESSED_8,
  SCS1_PRESSED_9,
  SCS1_PRESSED_0,
  SCS1_PRESSED_MINUS,
  SCS1_PRESSED_EQUAL,
  SCS1_PRESSED_BACKSPACE,
  SCS1_PRESSED_TAB,
  SCS1_PRESSED_Q,
  SCS1_PRESSED_W,
  SCS1_PRESSED_E,
  SCS1_PRESSED_R,
  SCS1_PRESSED_T,
  SCS1_PRESSED_Y,
  SCS1_PRESSED_U,
  SCS1_PRESSED_I,
  SCS1_PRESSED_O,
  SCS1_PRESSED_P,
  SCS1_PRESSED_LEFTBRACE,
  SCS1_PRESSED_RIGHTBRACE,
  SCS1_PRESSED_ENTER,
  SCS1_PRESSED_LEFTCTRL,
  SCS1_PRESSED_A,
  SCS1_PRESSED_S,
  SCS1_PRESSED_D,
  SCS1_PRESSED_F,
  SCS1_PRESSED_G,
  SCS1_PRESSED_H,
  SCS1_PRESSED_J,
  SCS1_PRESSED_K,
  SCS1_PRESSED_L,
  SCS1_PRESSED_SEMICOLON,
  SCS1_PRESSED_APOSTROPHE,
  SCS1_PRESSED_GRAVE,
  SCS1_PRESSED_LEFTSHIFT,
  SCS1_PRESSED_BACKSLASH,
  SCS1_PRESSED_Z,
  SCS1_PRESSED_X,
  SCS1_PRESSED_C,
  SCS1_PRESSED_V,
  SCS1_PRESSED_B,
  SCS1_PRESSED_N,
  SCS1_PRESSED_M,
  SCS1_PRESSED_COMMA,
  SCS1_PRESSED_DOT,
  SCS1_PRESSED_SLASH,
  SCS1_PRESSED_RIGHTSHIFT,
  SCS1_PRESSED_KPASTERISK,
  SCS1_PRESSED_LEFTALT,
  SCS1_PRESSED_SPACE,
  SCS1_PRESSED_CAPSLOCK,
  SCS1_PRESSED_F1,
  SCS1_PRESSED_F2,
  SCS1_PRESSED_F3,
  SCS1_PRESSED_F4,
  SCS1_PRESSED_F5,
  SCS1_PRESSED_F6,
  SCS1_PRESSED_F7,
  SCS1_PRESSED_F8,
  SCS1_PRESSED_F9,
  SCS1_PRESSED_F10,
  SCS1_PRESSED_NUMLOCK,
  SCS1_PRESSED_SCROLLOCK,
  SCS1_PRESSED_KP7,
  SCS1_PRESSED_KP8,
  SCS1_PRESSED_KP9,
  SCS1_PRESSED_KPMINUS,
  SCS1_PRESSED_KP4,
  SCS1_PRESSED_KP5,
  SCS1_PRESSED_KP6,
  SCS1_PRESSED_KPPLUS,
  SCS1_PRESSED_KP1,
  SCS1_PRESSED_KP2,
  SCS1_PRESSED_KP3,
  SCS1_PRESSED_KP0,
  SCS1_PRESSED_KPDOT,
  SCS1_PRESSED_F11 = 0x57,
  SCS1_PRESSED_F12,

  SCS1_RELEASED_ESC = 0x81,
  SCS1_RELEASED_1,
  SCS1_RELEASED_2,
  SCS1_RELEASED_3,
  SCS1_RELEASED_4,
  SCS1_RELEASED_5,
  SCS1_RELEASED_6,
  SCS1_RELEASED_7,
  SCS1_RELEASED_8,
  SCS1_RELEASED_9,
  SCS1_RELEASED_0,
  SCS1_RELEASED_MINUS,
  SCS1_RELEASED_EQUAL,
  SCS1_RELEASED_BACKSPACE,
  SCS1_RELEASED_TAB,
  SCS1_RELEASED_Q,
  SCS1_RELEASED_W,
  SCS1_RELEASED_E,
  SCS1_RELEASED_R,
  SCS1_RELEASED_T,
  SCS1_RELEASED_Y,
  SCS1_RELEASED_U,
  SCS1_RELEASED_I,
  SCS1_RELEASED_O,
  SCS1_RELEASED_P,
  SCS1_RELEASED_LEFTBRACE,
  SCS1_RELEASED_RIGHTBRACE,
  SCS1_RELEASED_ENTER,
  SCS1_RELEASED_LEFTCTRL,
  SCS1_RELEASED_A,
  SCS1_RELEASED_S,
  SCS1_RELEASED_D,
  SCS1_RELEASED_F,
  SCS1_RELEASED_G,
  SCS1_RELEASED_H,
  SCS1_RELEASED_J,
  SCS1_RELEASED_K,
  SCS1_RELEASED_L,
  SCS1_RELEASED_SEMICOLON,
  SCS1_RELEASED_APOSTROPHE,
  SCS1_RELEASED_GRAVE,
  SCS1_RELEASED_LEFTSHIFT,
  SCS1_RELEASED_BACKSLASH,
  SCS1_RELEASED_Z,
  SCS1_RELEASED_X,
  SCS1_RELEASED_C,
  SCS1_RELEASED_V,
  SCS1_RELEASED_B,
  SCS1_RELEASED_N,
  SCS1_RELEASED_M,
  SCS1_RELEASED_COMMA,
  SCS1_RELEASED_DOT,
  SCS1_RELEASED_SLASH,
  SCS1_RELEASED_RIGHTSHIFT,
  SCS1_RELEASED_KPASTERISK,
  SCS1_RELEASED_LEFTALT,
  SCS1_RELEASED_SPACE,
  SCS1_RELEASED_CAPSLOCK,
  SCS1_RELEASED_F1,
  SCS1_RELEASED_F2,
  SCS1_RELEASED_F3,
  SCS1_RELEASED_F4,
  SCS1_RELEASED_F5,
  SCS1_RELEASED_F6,
  SCS1_RELEASED_F7,
  SCS1_RELEASED_F8,
  SCS1_RELEASED_F9,
  SCS1_RELEASED_F10,
  SCS1_RELEASED_NUMLOCK,
  SCS1_RELEASED_SCROLLOCK,
  SCS1_RELEASED_KP7,
  SCS1_RELEASED_KP8,
  SCS1_RELEASED_KP9,
  SCS1_RELEASED_KPMINUS,
  SCS1_RELEASED_KP4,
  SCS1_RELEASED_KP5,
  SCS1_RELEASED_KP6,
  SCS1_RELEASED_KPPLUS,
  SCS1_RELEASED_KP1,
  SCS1_RELEASED_KP2,
  SCS1_RELEASED_KP3,
  SCS1_RELEASED_KP0,
  SCS1_RELEASED_KPDOT,
  SCS1_RELEASED_F11 = 0xD7,
  SCS1_RELEASED_F12,

  SCS1_MULTIBYTE = 0xE0,
  SCS1_PAUSE = 0xE1
} SCSet1;

#endif
